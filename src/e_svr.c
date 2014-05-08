/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		epoll_svr.c -   A simple echo server using the epoll API
--
--	PROGRAM:			epolls
--						gcc -Wall -ggdb -o epolls epoll_svr.c
--
--	FUNCTIONS:			Berkeley Socket API
--
--	DATE:				February 19, 2014
--
--	REVISIONS:			(Date and Description)
--
--	DESIGNERS:			Design based on various code snippets found on C10K links
--						Modified and improved: Aman Abdulla - February 2008
--                      Adapted to suit the requirements of the assignment: Jivanjot Brar
 *						& Shan Bains.
--
--	PROGRAMMERS:		Jivanjot S Brar & Shan Bains
--
--	NOTES:
--	The program will accept TCP connections from client machines.
-- 	The program will read data from the client socket and simply echo it back.
--	Design is a simple, single-threaded server using non-blocking, edge-triggered
--	I/O to handle simultaneous inbound connections.
--	Test with accompanying client application: epoll_clnt.c
---------------------------------------------------------------------------------------*/

#include "common.h"

const char client_msg[BUFLEN] =
        "012345678901234567890123456789012345678901234567890123456789012\n";

// GLOBALS
pthread_t master_thread;
bool running = true;
server *serv;

/**
 * main
 *
 * Parses the user commandline input and creates and waits for the
 * server thread.
 *
 * @param argc number of command line arguments
 * @param argv the command line arguments
 * @return EXIT_SUCCES after successful completion.
 */
int main(int argc, char **argv) {
    int ret;
    struct sigaction act;
    server *s;

    act.sa_handler = signal_Handler;
    act.sa_flags = 0;

    if ((sigaction(SIGINT, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGINT handler\n");
    }
    if ((sigaction(SIGPIPE, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGPIPE handler\n");
    }
    if ((sigaction(SIGSEGV, &act, NULL) == -1)) {
        SystemFatal("Failed to set SIGPIPE handler\n");
    }

    /* allocate memory for server data */
    s = server_new();
    serv = s;

    switch (argc) {
        case 1:
            s->port = SERVER_TCP_PORT; // Use the default port
            break;
        case 2:
            s->port = atoi(argv[1]); // Get user specified port
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    ret = pthread_create(&master_thread, NULL, client_manager, s);
    if (ret != 0)
        fprintf(stderr, "Unable to create client management thread\n");
    pthread_join(master_thread, NULL);

    /* clean up */
    free(s);

    return EXIT_SUCCESS;
}

/**
 * client_manager
 *
 * Thread function to manage client connections.
 *
 * @param data Thread data for the function
 */
void* client_manager(void *data) {
    /*int nready;
    client *c = NULL;
    node *n = NULL;*/
    server *s = (server *) data;

    server_init(s);

    s->epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (s->epoll_fd < 0)
        SystemFatal("epoll_create() Failed\n");

    /* Add the server socket to the epoll event loop  */
    s->event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    s->event.data.fd = s->listen_sd;
    if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, s->listen_sd, &s->event) == -1)
        SystemFatal("epoll_ctl() error\n");


    for (; running;) {
        pthread_mutex_lock(&s->dataLock);
        s->num_fds = epoll_wait(s->epoll_fd, s->events, EPOLL_QUEUE_LEN, -1);
        pthread_mutex_unlock(&s->dataLock);

        if (s->num_fds < 0)
            SystemFatal("epoll_wait(): Error\n");

        read_from_socket(s);
    }

    fprintf(stdout, "Exiting the client manager\n");
    close(s->listen_sd);
    free(s);
    pthread_exit(NULL);
}

/**
 * read_from_socket
 *
 * Handles the activities that take place on the sockets monitored by
 * epoll. The activity includes new connections or data transfer from
 * existing connections.
 *
 * @param s The server data containing the epoll events to handle.
 */
void read_from_socket(server *s) {
    int i;
    client *c = NULL;
    node *it = NULL;

    for (i = 0; i < s->num_fds; i++) {
        /* Error check */
        if (s->events[i].events & (EPOLLHUP | EPOLLERR)) {
            fprintf(stderr, "epoll(): EPOLLERR\n");
            close(s->events[i].data.fd);
            continue;
        }
        assert(s->events[i].events & EPOLLIN);

        /* Server is receiving a connection request */
        if (s->events[i].data.fd == s->listen_sd) {
            while (true) {
                /* create new client data */
                c = client_new();
                c->sa_len = sizeof (c->sa);

                fprintf(stdout, "client connection\n");
                c->fd = accept(s->listen_sd, (struct sockaddr *) &c->sa, &c->sa_len);
                if (c->fd < 0) {
                    if ((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK)) {
                        /* all incoming connections have been processed */
                        break;
                    } else {
                        SystemFatal("accept(): Error");
                        break;
                    }
                }
                fprintf(stdout, "Received connection from (%s, %d)\n",
                        inet_ntoa(c->sa.sin_addr),
                        ntohs(c->sa.sin_port));

                /* make the server Socket non-blocking */
                if (fcntl(c->fd, F_SETFL, O_NONBLOCK | fcntl(c->fd, F_GETFL, 0)) == -1)
                    SystemFatal("fcntl(): Server Non-Block Failed\n");

                pthread_mutex_lock(&s->dataLock);

                /* update counters */
                s->n_clients++;
                s->n_max_connected++;

                /* add the new socket descriptor to the epoll loop */
                s->event.data.fd = c->fd;
                if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, c->fd, &s->event) == -1)
                    SystemFatal("epoll_ctl() error");

                /* add the client data to the linked list */
                s->e_client_list = llist_append(s->e_client_list, (void *) c);

                fprintf(stdout, "Added client to list, new size: %d\n",
                        llist_length(s->e_client_list));

                pthread_mutex_unlock(&s->dataLock);
            }

            continue;
        } else {
            /* go through existing client connections. */
            it = s->e_client_list->link;
            while (it != NULL) {
                c = (client *) it->data;
                if (c->fd == s->events[i].data.fd)
                    break;
                it = it->next;
            }

            //process_client_data(c, s);
            process_client_req(c, s);

            if (c->quit) {
                pthread_mutex_trylock(&s->dataLock);
                s->n_clients--;
                s->e_client_list = llist_remove(s->e_client_list, (void *) c,
                        client_compare);
                fprintf(stderr, "[%5d]Removed client from list, new size: %d\n",
                        c->fd, llist_length(s->e_client_list));
                close(c->fd);
                c = NULL;
                free(c);
                pthread_mutex_unlock(&s->dataLock);
            }
        }
    }
}

/**
 * process_client_data
 *
 * Processes the current client command that triggered an event
 *
 * @param c the client data containing the file descriptor to write to.
 */
void process_client_data(client *c, server *s) {
    ssize_t r, w;
    int bytes_to_read = BUFLEN;
    char error[100], recvBuff[BUFLEN], sendBuff[BUFLEN];
    //char *recv;
    //recv = malloc(BUFLEN * sizeof (char));
    while ((r = read(c->fd, recvBuff, bytes_to_read)) > 0) {
        //recv += r;
        recvBuff[r] = 0;
        s->n_max_bytes_received += r;
        bytes_to_read -= r;
    }

    strcpy(sendBuff, recvBuff);

    if ((w = write(c->fd, sendBuff, BUFLEN)) != BUFLEN) {
        strcpy(error, "Client Write Error - ");
        sscanf(error, "%d != %d\n", (int *) strlen(recvBuff), (int *) w);
        SystemFatal(error);
    }
    c->quit = true;
    return;
    //n = readline (c->fd, recv, BUFLEN);
}

/**
 * process_client_req
 *
 * process the client request and responses with data, or starts the closing
 * procedure
 *
 * @param c client information
 * @param s server information
 */
void process_client_req(client *c, server *s) {
    ssize_t r, w;
    char *recv;
    //int bytes_to_read = BUFLEN;
    char error[100];

    recv = malloc(BUFLEN * sizeof (char));

    while ((r = read(c->fd, recv, BUFLEN)) > 0) {
        s->n_max_bytes_received += r;
    }

    if (strcmp(recv, "request\n") == 0) {
        if ((w = write(c->fd, client_msg, BUFLEN)) != BUFLEN) {
            strcpy(error, "write(): Client Write Error - ");
            sscanf(error, "%d != %d\n", (int *) strlen(recv), (int *) w);
            SystemFatal(error);
        }
    } else if (strcmp(recv, "quit\n") == 0) {
        c->quit = true;
        return;
    }
}

/**
 * client_new
 *
 * create a client struct and initialize default variables.
 *
 * @return c returns the client struct
 */
client * client_new(void) {
    client *c = malloc(sizeof (client));
    c->sa_len = sizeof (c->sa);
    c->quit = false;
    return c;
}

/**
 * server_new
 *
 * Creates a server structure and initializes the variables
 *
 * @return s Returns the server structure
 */
server * server_new(void) {
    server *s = malloc(sizeof (server));
    if (s == NULL)
        fprintf(stderr, "Server Malloc() Failed\n");

    s->pid = getpid();
    s->n_clients = 0;
    s->n_max_connected = 0;
    s->n_max_bytes_received = 0;

    s->e_client_list = llist_new();

    /* create the mutexes for controlling access to thread data */
    if (pthread_mutex_init(&s->dataLock, NULL) != 0) {
        free(s);
        return NULL;
    }

    return s;
}

/**
 * server_init
 *
 * Creates a socket and puts the socket into a listening mode
 *
 * @param s server structure variable.
 */
void server_init(server * s) {
    const int on = 1;
    struct sockaddr_in servaddr;

    /* create TCP socket to listen for client connections */
    fprintf(stderr, "> Creating TCP socket\n");
    s->listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->listen_sd < 0)
        SystemFatal("Socket Creation Failed\n");

    /* set the socket to allow re-bind to same port without wait issues */
    setsockopt(s->listen_sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (int));

    /* make the server Socket non-blocking */
    if (fcntl(s->listen_sd, F_SETFL, O_NONBLOCK | fcntl(s->listen_sd, F_GETFL, 0)) == -1)
        SystemFatal("fcntl(): Server Non-Block Failed\n");

    bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(s->port);

    if (bind(s->listen_sd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
        SystemFatal("Failed to bind socket\n");

    /* setup the socket for listening to incoming connection  */
    fprintf(stderr, "> Waiting for Connections\n");
    if (listen(s->listen_sd, LISTENQ) < 0)
        SystemFatal("Unable to listen on socket \n");

    /* right now the listening socket is the max */
    s->maxfd = s->listen_sd;
}

/**
 * print_server_data
 *
 * ....
 *
 * @param s
 */
void print_server_data(server * s) {
    fprintf(stdout, "\n\n[===========================================]\n");
    fprintf(stdout, "[ Total Clients Connected: %d\n", s->n_max_connected);
    fprintf(stdout, "[ Total Bytes Received: %d\n", s->n_max_bytes_received);
    fprintf(stdout, "[ Total Active Clients: %d\n", s->n_clients);
    fprintf(stdout, "[===========================================]\n\n");
}

/**
 * signal_Handler
 *
 * Handles signals that occur during program execution.
 *
 * @param signo The Signal Received
 */
void signal_Handler(int signo) {
    switch (signo) {
        case SIGINT:
            fprintf(stderr, "\nReceived SIGINT signal\n");
            running = false;
            close(serv->listen_sd);
            print_server_data(serv);
            exit(EXIT_FAILURE);
            break;

        case SIGSEGV:
            fprintf(stderr, "\nReceived SIGSEGV signal\n");
            close(serv->listen_sd);
            print_server_data(serv);
            exit(EXIT_FAILURE);
            break;

        case SIGPIPE:
            fprintf(stderr, "\nReceived SIGPIPE signal\n");
            writeResult = -1;
            break;

        default:
            fprintf(stderr, "\nUn handled signal %s\n", strsignal(signo));
            exit(EXIT_FAILURE);
            break;
    }
}

/**
 * SystemFatal
 *
 * Displays a perror message and exits the program.
 *
 * @param message takes in a string message
 */
void SystemFatal(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}
