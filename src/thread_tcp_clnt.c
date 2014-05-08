/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		thread_tcp_clnt.c - A simple TCP client program.
--
--	PROGRAM:		TCP Client
--				gcc -Wall -ggdb -o clnt tcp_clnt.c -lpthread
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			January 23, 2001
--
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				* Modified the read loop to use fgets.
--				* While loop is based on the buffer length
--                              * Connects to the server variable number of times.
--                              * Gets the time it took to send and receive from the
--                                server.
--                              * Monitors number of connections made
--                              * Gets total bytes of data sent
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
#include <sys/wait.h>

#define EXEC 1500
#define MAXLINE 255

const char client_msg[MAXLINE] =
        "012345678901234567890123456789012345678901234567890123456789012\n";
FILE *fp;
char *data_file;
sem_t mutex;

/**
 * main
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char **argv) {
    int i = 0;
    int num;
    int port;
    char *host;
    data *d;
    int pid[EXEC], status;

    // initialize data struct
    d = malloc(sizeof (data));
    d->numOfRequests = 0;
    d->dataSent = 0;
    d->time = 0;

    switch (argc) {
        case 2:
            host = argv[1]; // Host Name
            port = SERVER_TCP_PORT; // Default Port
            strcpy(d->sendBuff, client_msg); // Default Text
            num = 50; // Default client execution
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]); // User Specified Port
            num = 50; // Default client execution
            strcpy(d->sendBuff, client_msg); // Default Text
            break;
        case 4:
            host = argv[1];
            port = atoi(argv[2]); // User Specified Port
            num = atoi(argv[3]); // User Specified client execution
            strcpy(d->sendBuff, client_msg); // Use Specified Text
            break;
        case 5:
            host = argv[1];
            port = atoi(argv[2]); // User Specified Port
            num = atoi(argv[3]); // User Specified client execution
            strcpy(d->sendBuff, argv[4]); // Use Specified Text
            break;
        default:
            fprintf(stderr, "USAGE: %s HOST [PORT] [EXECUTION] [INPUT] \n", argv[0]);
            exit(1);
    }

    if (sem_init(&mutex, 1, 1) < 0) {
        perror("sem_init(): semaphore initialization failed");
        exit(EXIT_FAILURE);
    }

    data_file = malloc(sizeof (char *));
    sprintf(data_file, "./thread_clnt.csv");
    if ((fp = fopen(data_file, "a+")) == NULL) {
        perror("fopen(): Unable to open the file");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "ProcessID,Time(Seconds),Data(Bytes)\n");
    fclose(fp);


    // run the server 10 times
    for (i = 0; i < EXEC; i++) {
        pid[i] = fork();

        if (pid[i] < 0) { // ERROR
            perror("fork(): failed");
            exit(EXIT_FAILURE);
        }

        if (pid[i] == 0) { // CHILD
            connect_to_server(d, host, port);
            break;
        }
    }

    if (pid[i] > 0) {
        for (i = 0; i < EXEC; i++) {
            waitpid(pid[i], &status, 0);
        }

        fprintf(stderr, "\n   -------------------------------------------------------------\n");
        fprintf(stderr, "   ############    Main(): Child Processes Ended    ############\n");
        fprintf(stderr, "   -------------------------------------------------------------\n\n");

        free(data_file);
    }
    return EXIT_SUCCESS;
}

/**
 * connect_to_server
 * 
 * @param d
 * @param host
 * @param port
 */
void connect_to_server(data *d, char * host, int port) {
    int n, bytes_to_read;
    int sd;
    struct hostent *hp;
    struct sockaddr_in server;
    char *bp, rbuf[MAXLINE], **pptr;
    char str[16];
    double t1, t2;
    pid_t pid;


    // Create the socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
    if (connect(sd, (struct sockaddr *) &server, sizeof (server)) == -1) {
        fprintf(stderr, "Can't connect to server\n");
        perror("connect");
        exit(1);
    }
    // connect to server and increment counter
    printf("Connected:    Server Name: %s\n", hp->h_name);
    d->numOfRequests++;
    pptr = hp->h_addr_list;
    printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof (str)));
    printf("> Transmit:\n");
    fprintf(stdout, "%s\n", client_msg);
    // send data to server through socket
    gettimeofday(&d->start, NULL);
    send(sd, d->sendBuff, MAXLINE, 0);
    d->dataSent += MAXLINE;

    // Receive data back from server
    printf("> Receive:\n");
    bp = rbuf;
    bytes_to_read = MAXLINE;

    // client makes repeated calls to recv until no more data is expected to arrive.
    n = 0;
    while ((n = recv(sd, bp, bytes_to_read, 0)) < MAXLINE) {
        bp += n;
        bytes_to_read -= n;
    }
    printf("%s\n", rbuf);
    // get end time
    gettimeofday(&d->end, NULL);

    // calculate time difference from start to finish
    t1 = d->start.tv_sec + (d->start.tv_usec / 1000000.0);
    t2 = d->end.tv_sec + (d->end.tv_usec / 1000000.0);

    // store time
    d->time = t2 - t1;

    if ((fp = fopen(data_file, "a+")) == NULL) {
        perror("fopen(): Unable to open the file");
    }
    sem_wait(&mutex);
    pid = getpid();
    fprintf(fp, "%d,%d,%lf\n", (int) pid, d->dataSent, d->time);
    fclose(fp);
    sem_post(&mutex);

    //print client information
    //printf("Number of Requests: %d\n", d->numOfRequests);
    //printf("Data Sent: %d \n", d->dataSent);
    //printf("Time: %lf \n", d->time);

    fflush(stdout);
    close(sd);
}
