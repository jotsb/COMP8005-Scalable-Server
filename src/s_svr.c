/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		s_svr.c -   A simple multiplexed echo server using TCP
--
--	PROGRAM:			Select Server
--						gcc -Wall -ggdb -o s_svr s_svr.c -lpthread
--
--	FUNCTIONS:			Berkeley Socket API
--
--	DATE:				February 5, 2014
--
--
--	DESIGNERS:			Based on Richard Stevens Example, p165-166
--						Modified & redesigned: Aman Abdulla: February 16, 2001
--                      Later Adapted and modified to suit the requirements of an
--                      assignment by: Jivanjot Brar & Shan Bains
--
--	PROGRAMMERS:		Jivanjot Brar & Shan Bains
--
--  File Location:      /root/Dropbox/1\)\ School/B-Tech/COMP8005/Assignments/Assignment2-Scalable-Servers
--
--	NOTES:
--	The program will accept TCP connections from multiple client machines.
-- 	The program will read data from each client socket and simply echo it back.
--      http://beej.us/guide/bgipc/output/html/multipage/signals.html
--      http://www.chemie.fu-berlin.de/chemnet/use/info/libc/libc_21.html
---------------------------------------------------------------------------------------*/

#include "common.h"

const char client_msg[BUFLEN] =
"012345678901234567890123456789012345678901234567890123456789012\n";

// GLOBALS
pthread_t master_manager;
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
int main(int argc, char** argv) {

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
	/* creates a new server variable and initialize the structure. */
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

	ret = pthread_create(&master_manager, NULL, client_manager, s);
	if (ret != 0)
		SystemFatal("Unable to create client management thread\n");

	pthread_join(master_manager, NULL);

	free(s);

	return (EXIT_SUCCESS);
}

/**
 * server_new
 *
 * Creates a server structure and initializes the variables
 *
 * @return s Returns the server structure
 */
server* server_new(void) {
	//int i;
	server *s = malloc(sizeof (server));
	if (s == NULL)
		SystemFatal("Malloc() Failed \n");

	s->pid = getpid();
	s->n_clients = 0;
	s->n_max_connected = 0;
	s->n_max_bytes_received = 0;

	s->client_list = llist_new();

	/*for (i = 0; i <= FD_SETSIZE; i++) {
		s->clients[i] = -1;
		}*/

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
void server_init(server *s) {
	const int on = 1;
	struct sockaddr_in servaddr;

	/* create TCP socket to listen for client connections */
	fprintf(stderr, "Creating TCP socket\n");
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
	if (listen(s->listen_sd, LISTENQ) < 0)
		SystemFatal("Unable to listen on socket \n");

	/* right now the listening socket is the max */
	s->maxfd = s->listen_sd;

}

/**
 * client_new
 *
 * create a client struct and initialize default variables.
 *
 * @return c returns the client struct
 */
client* client_new(void) {
	client *c = malloc(sizeof (client));
	c->sa_len = sizeof (c->sa);
	c->quit = false;
	return c;
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
	//printf("%s\n", recvBuff);
	strcpy(sendBuff, recvBuff);
	if ((w = write(c->fd, sendBuff, BUFLEN)) != BUFLEN) {
		strcpy(error, "Client Write Error - ");
		sscanf(error, "%d != %d\n", (int *)strlen(recvBuff), (int *)w);
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
			sscanf(error, "%d != %d\n", (int *)strlen(recv), (int *)w);
			SystemFatal(error);
		}
	}
	else if (strcmp(recv, "quit\n") == 0) {
		c->quit = true;
		return;
	}
}

/**
 * client_manager
 *
 * Thread function to manage client connections.
 *
 * @param data Thread data for the function
 */
void* client_manager(void *data) {
	int nready;
	client *c = NULL;
	node *n = NULL;
	server *s = (server *)data;

	/* set up as a tcp server */
	server_init(s);

	while (running) {
		pthread_mutex_lock(&s->dataLock);

		FD_ZERO(&s->allset);
		FD_SET(s->listen_sd, &s->allset);

		/* loop through all possible socket connections and add
		 * them to fd_set */
		for (n = s->client_list->link; n != NULL; n = n->next) {
			c = (client *)n->data;
			FD_SET(c->fd, &s->allset);
			if (c->fd > s->maxfd)
				s->maxfd = c->fd;
		}

		/* Monitor sockets for any activity of new connections or data transfer */
		nready = select(s->maxfd + 1, &s->allset, NULL, NULL, NULL);

		pthread_mutex_unlock(&s->dataLock);

		if (nready < 0) {
			/* Something interrupted the call, cause EINTR, continue */
			if (errno == EINTR)
				continue;
			else
				SystemFatal("Select() Error\n");
		}
		else if (nready == 0) {
			/* nothing to read, continue */
			continue;
		}
		else {
			/* There is an activity on one or more sockets. */
			read_from_socket(s);
		}
	}

	fprintf(stdout, "Exiting the client manager\n");
	close(s->listen_sd);
	free(s);
	pthread_exit(NULL);
}

/**
 * read_from_client
 *
 * Handles the activities that take place on the sockets monitored by
 * select. The activity includes new connections or data transfer from
 * existing connections.
 *
 * @param s Server struct, contains information about the server.
 */
void read_from_socket(server *s) {
	//int ret, maxi;
	client *c = NULL;
	node *n = NULL;

	//maxi = 0;

	/* Check if a client is trying to connect */
	pthread_mutex_trylock(&s->dataLock);
	if (FD_ISSET(s->listen_sd, &s->allset)) {

		/* get the client data ready */
		c = client_new();

		/* blocking call waiting for connections */
		c->fd = accept(s->listen_sd, (struct sockaddr *) &c->sa, &c->sa_len);
		/* make the client Socket non-blocking */
		if (fcntl(c->fd, F_SETFL, O_NONBLOCK | fcntl(c->fd, F_GETFL, 0)) == -1)
			SystemFatal("fcntl(): Client Non-Block Failed\n");

		fprintf(stdout, "Received connection from (%s, %d)\n",
			inet_ntoa(c->sa.sin_addr),
			ntohs(c->sa.sin_port));
		s->n_clients++;
		s->n_max_connected++;
		/*s->n_max_connected = (s->n_clients > s->n_max_connected) ?
				s->n_clients : s->n_max_connected;*/
		s->client_list = llist_append(s->client_list, (void *)c);
		fprintf(stdout, "Added client to list, new size: %d\n",
			llist_length(s->client_list));
		/* add the client to the list */
		/*for (i = 0; i < FD_SETSIZE; i++) {
			if (s->clientConn[i] == NULL) {
			s->clientConn[i] = c;
			//break;
			}
			if (s->clients[i] < 0) {
			s->clients[i] = c->fd;
			break;
			}
			if (i == FD_SETSIZE) {
			SystemFatal("To Many Clients\n");
			}
			}*/
		/* add the socket connections to the fd_set */
		/*FD_SET(c->fd, &s->allset);
		s->maxfd = (c->fd > s->maxfd) ?
		c->fd : s->maxfd;
		maxi = (i > maxi) ? i : maxi;*/
		c = NULL;
	}
	pthread_mutex_unlock(&s->dataLock);

	for (n = s->client_list->link; n != NULL; n = n->next) {

		/* check if the client cause an event */
		pthread_mutex_trylock(&s->dataLock);
		c = (client *)n->data;
		if (FD_ISSET(c->fd, &s->allset)) {
			process_client_req(c, s);
			//process_client_data(c, s);
		}
		pthread_mutex_unlock(&s->dataLock);

		//pthread_mutex_trylock(&s->dataLock);  TRY THIS LATER
		if (c->quit) {
			pthread_mutex_trylock(&s->dataLock);
			s->n_clients--;
			s->client_list = llist_remove(s->client_list, (void *)c, client_compare);
			fprintf(stderr, "[%5d]Removed client from list, new size: %d\n",
				c->fd, llist_length(s->client_list));
			close(c->fd);
			free(c);
			c = NULL;
			pthread_mutex_unlock(&s->dataLock);
		}
		//pthread_mutex_unlock(&s->dataLock);  TRY THIS LATER
	}

	/* go through the available connections */
	/*for(n = 0; n <= maxi; n++) {
		pthread_mutex_trylock(&s->dataLock);
		if(FD_ISSET(s->clients[n], &s->allset)){
		process_client_data();
		}
		pthread_mutex_unlock(&s->dataLock);
		}*/
	//free(c);
}

void print_server_data(server *s) {
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