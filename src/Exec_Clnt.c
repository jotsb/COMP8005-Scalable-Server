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
#include <sys/wait.h>

#define EXEC 10

int random_num(int, int);

// Global
FILE *fp;

/**
 * main
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char **argv) {
    int i = 0;
    int pid[EXEC], status;
    struct sigaction act;
    char *data_file, *host, *interval, *port, *file;

    switch (argc) {
        case 2:
            host = argv[1];
            port = TOSTRING(SERVER_TCP_PORT);
            file = malloc(sizeof (char *));
            sprintf(file, "./data_file.csv");
            break;
        case 3:
            host = argv[1];
            port = argv[2];
            file = malloc(sizeof (char *));
            sprintf(file, "./data_file.csv");
            break;
        case 4:
            host = argv[1];
            port = argv[2];
            file = argv[3];
            break;
        default:
            fprintf(stderr, "USAGE: %s HOST [PORT] [SERVER NAME] \n", argv[0]);
            exit(1);
    }
    interval = malloc(sizeof (char *));
    data_file = malloc(sizeof (char *));
    sprintf(data_file, "./%s.csv", file);

    if ((fp = fopen(data_file, "a+")) == NULL) {
        perror("fopen(): Unable to open the file");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "ProcessID,Time(Seconds),Data(Bytes)\n");

    free(data_file);
    fclose(fp);

    // set up the signal handler to close the server socket when CTRL-c is received
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGINT, &act, NULL) == -1)) {
        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }
    if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGSEGV, &act, NULL) == -1)) {
        perror("Failed to set AIGALRM handler");
        exit(EXIT_FAILURE);
    }

    // run the server 10 times
    //for (i = 0; i < EXEC; i++) {
	while(1) {
        sprintf(interval, "%d", random_num(1, 10));
        printf("%s", interval);
        pid[i] = fork();
        if (pid[i] < 0) {
            perror("Fork(): Failed");
            exit(EXIT_FAILURE);
        }
        //Child
        if (pid[i] == 0) {
            //connect_to_server(d, host, port);
            execl("tcp_clnt", "tcp_clnt", host, port, interval, file, NULL);
            break;
        }
        usleep(150000);
    }

    // Parent waits for processes to end
    for (i = 0; i < EXEC; i++) {
        waitpid(pid[i], &status, 0);
    }

    fprintf(stderr, "\n   -------------------------------------------------------------\n");
    fprintf(stderr, "   ############    Main(): Child Processes Ended    ############\n");
    fprintf(stderr, "   -------------------------------------------------------------\n\n");

    return EXIT_SUCCESS;
}

int random_num(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
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
        case SIGINT:
            fprintf(stderr, "\nReceived SIGINT signal\n");
            exit(EXIT_FAILURE);
            break;

        case SIGSEGV:
            fprintf(stderr, "\nReceived SIGSEGV signal\n");
            fclose(fp);
            exit(EXIT_FAILURE);
            break;

        default:
            fprintf(stderr, "\nUn handled signal %s\n", strsignal(signo));
            fclose(fp);
            exit(EXIT_FAILURE);
            break;
    }
}
