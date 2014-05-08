/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		Exec_Clnt.c - A simple TCP client program.
--
--	PROGRAM:			TCP Client
--						gcc -Wall -ggdb -o clnt tcp_clnt.c -lpthread
--
--	FUNCTIONS:			Berkeley Socket API
--
--	DATE:				January 23, 2001
--
--
--	REVISIONS:			(Date and Description)
--						January 2005
--						* Modified the read loop to use fgets.
--						* While loop is based on the buffer length
--                      * Connects to the server variable number of seconds.
--                      * Gets the time it took to send and receive from the
--                        server.
--                      * Monitors number of connections made
--                      * Gets total bytes of data sent
--						* Forks and executes child processes to connect to server
--
--
--	PROGRAMMERS:		Jivanjot Brar & Shan Bains
--
--	NOTES:
--	The program will establish a TCP connection to a user specifed server.
-- 	The server can be specified using a fully qualified domain name or and
--	IP address. After the connection has been established the user will be
-- 	prompted for date. The date string is then sent to the server and the
-- 	response (echo) back from the server is displayed.
---------------------------------------------------------------------------------------*/

#include "common.h"
#include <semaphore.h>

const char client_msg[BUFLEN] =
        "012345678901234567890123456789012345678901234567890123456789012\n";
const char request[BUFLEN] = "request\n";
const char quit[BUFLEN] = "quit\n";

sem_t mutex;
bool running = true;
data *d;
FILE *fp;

/**
 * main
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char **argv) {
    int n, bytes_to_read, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char *bp, rbuf[BUFLEN], **pptr, *host, *data_file, str[16];
    double t1, t2;
    struct sigaction act;

    // initialize data struct 
    d = malloc(sizeof (data));
    d->numOfRequests = 0;
    d->dataSent = 0;
    d->time = 0;

    host = argv[1];
    port = atoi(argv[2]);
    d->interval = atoi(argv[3]);

    // set up the signal handler to close the server socket when CTRL-c is received
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGALRM, &act, NULL) == -1)) {
        perror("Failed to set AIGALRM handler");
        exit(EXIT_FAILURE);
    }
    if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGINT, &act, NULL) == -1)) {
        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&mutex, 1, 1) < 0) {
        perror("sem_init(): semaphore initialization failed");
        exit(EXIT_FAILURE);
    }

    data_file = malloc(sizeof (char *));
    sprintf(data_file, "./%s.csv", argv[4]);
    if ((fp = fopen(data_file, "a+")) == NULL) {
        SystemFatal("fopen(): Unable to open the file");
    }

    // Create the socket
    if ((d->sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot create socket");
        exit(1);
    }
    bzero((char *) &server, sizeof (struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((hp = gethostbyname(host)) == NULL) {
        fprintf(stderr, "Unknown server address\n");
        exit(1);
    }
    bcopy(hp->h_addr, (char *) &server.sin_addr, hp->h_length);

    // Connecting to the server
    if (connect(d->sd, (struct sockaddr *) &server, sizeof (server)) == -1) {
        fprintf(stderr, "Can't connect to server\n");
        perror("connect");
        exit(1);
    }
    // connect to server and increment counter
    printf("Connected:    Server Name: %s\n", hp->h_name);
    d->numOfRequests++;
    pptr = hp->h_addr_list;
    printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof (str)));
    printf("> Transmit: ");
    //fprintf(stdout, "%s\n", client_msg);
    fprintf(stdout, "%s\n", request);
    // send data to server through socket
    gettimeofday(&d->start, NULL);

    /* setting an alarm to shut down the server */
    alarm(d->interval);

    //send(sd, d->sendBuff, BUFLEN, 0);
    //write(sd, d->sendBuff, BUFLEN);

    while (running) {
        /* send a request */
        write(d->sd, request, BUFLEN);

        d->dataSent += BUFLEN;

        // Receive data back from server
        printf("> Receive: ");
        bp = rbuf;
        bytes_to_read = BUFLEN;

        // client makes repeated calls to recv until no more data is expected to arrive.
        n = 0;
        while ((n = recv(d->sd, bp, bytes_to_read, 0)) < BUFLEN) {
            bp += n;
            bytes_to_read -= n;
        }
        printf("%s\n", rbuf);
        usleep(250000);
    }
    printf("> Transmit: ");
    fprintf(stdout, "%s\n", quit);
    /* send quit signal */
    write(d->sd, quit, BUFLEN);

    // get end time
    gettimeofday(&d->end, NULL);

    // calculate time difference from start to finish
    t1 = d->start.tv_sec + (d->start.tv_usec / 1000000.0);
    t2 = d->end.tv_sec + (d->end.tv_usec / 1000000.0);

    // store time
    d->time = t2 - t1;

    print_client_data(d);

    fflush(stdout);
    close(d->sd);
    free(d);

    return EXIT_SUCCESS;
}

/**
 * print_client_data
 * 
 * print client information
 * 
 * @param d
 */
void print_client_data(data *d) {
    pid_t pid;
    sem_wait(&mutex);
    pid = getpid();
    fprintf(fp, "%d,%d,%lf\n", (int) pid, d->dataSent, d->time);
    sem_post(&mutex);
    /*fprintf(fp, "Client Process ID: %d\n", (int) pid);
    fprintf(fp, "Number of Requests: %d\n", d->numOfRequests);
    fprintf(fp, "Data Sent: %d \n", d->dataSent);
    fprintf(fp, "Time: %lf \n", d->time);
    fprintf(fp, "************************************************************\n\n");*/
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

/**
 * signal_Handler
 * 
 * Handles signals that occur during program execution.
 * 
 * @param signo The Signal Received
 */
void signal_handler(int signo) {
    switch (signo) {
        case SIGALRM:
            fprintf(stderr, "\nReceived SIGALRM signal\n");
            running = false;
            break;
        case SIGINT:
            fprintf(stderr, "\nReceived SIGINT signal\n");
            exit(EXIT_FAILURE);
            break;
    }
}
