/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		thread_svr.c - A traditional multithreaded sever. 
--
--	PROGRAM:		Thread Sever
--				gcc -Wall -ggdb -o t_svr thread_svr.c
--                              ./t_svr <port> 
--
--	FUNCTIONS:		Berkeley Socket API
--
--	DATE:			February 5, 2014
--
--	REVISIONS:		(Date and Description)
--				January 2005
--				Modified the read loop to use fgets.
--				While loop is based on the buffer length 
--
--
--	PROGRAMMERS:		Jivanjot Brar & Shan Bains
--
--
--  	File Location:          /B-Tech/COMP8005/Assignments/Assignment2-Scalable-Servers 
--
--	NOTES:
--	This program will start a multithreaded server that can handle multiple connections
--	from differnt computers. 
---------------------------------------------------------------------------------------*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_TCP_PORT 7000 	//Default port
#define BUF_LENGTH	255	//Buffer length off the socket

int createSocket();
int listenForClients(int);
int checkConnection();
void* recieveFromClient(void *);
static void SystemFatal(const char*) ;

// struct to hold client info
typedef struct
{
	char* 	ip;
	int	socket;
}clientInfo;

// struct to hold host info
typedef struct
{
	char*	status;
	char* 	ip;
	int	numOfConnections;
	int 	numOfBytesSent;
}hostInfo;

// globals
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
hostInfo* host;
int totalHosts;
int activeConnections;

int main(int argc, char **argv) 
{
    totalHosts = 0;
    int port = 0;
    
        switch(argc)
	{
		case 1:
			port = SERVER_TCP_PORT;	// use default port
		break;
		case 2:
			port = atoi(argv[1]);	// get user specified port
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}
	
	host = malloc(10000 * sizeof(hostInfo)); // 10000 hosts max 
	
	
	if(checkConnection() == -1)
	{
		perror("Server Exited - Error occurred");
		exit(1);
	}

	// if server is closing, free all allocated memory 
	free(host);
	return EXIT_SUCCESS;
}

/* 
 * Function to check if server encountered an error, if so exit and clear 
 * allocated memory 
 */ 
int checkConnection()
{
	int socket;

	// create socket
	socket = createSocket();
	if(socket == -1)
	{
		perror("Socket Error!");
		return(-1);
	}
	
	// run listenForClients function and exit if error occurred. 
	if(listenForClients(socket) == -1)
	{
		perror("Client Connection Error!");
		return(-1);
	}
	return 0;
}

/*
 * Function to create socket, set options and bind name to socket
 */ 
int createSocket()
{
	int sd, port, arg;
	struct	sockaddr_in server;

	//Get port from global default
	port = SERVER_TCP_PORT;

	// Create a stream socket
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Can't create a socket");
		return(-1);
	}
	
	arg = 1;
	// REUSEADDR to reuse port if something goes wrong
	if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
		SystemFatal("setsockopt");
	}

	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	//bind name to socket
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		perror("Can't bind name to socket");
		return(-1);
	}
	
	//return socket
	return sd;
}


/*
 * Function to listen for clients, accept clients and store connection information
 */
int listenForClients(int sd)
{
	int	new_sd;
	socklen_t client_len;
	struct sockaddr_in client;

	//listen for clients
	listen(sd, 5);
	while(1)
	{
		pthread_t clientThread;
		clientInfo *cl = malloc(sizeof(clientInfo));
		
		client_len = sizeof(client);
		
		// accept new connections on socket
		if ((new_sd = accept (sd, (struct sockaddr *)&client, &client_len)) == -1)
		{
			fprintf(stderr, "Can't accept client\n");
			return -1;
		}

		cl->ip = inet_ntoa(client.sin_addr);
		cl->socket = new_sd;
		//increment number of active connections
		activeConnections++;
		//create a new thread for each connection
		pthread_create(&clientThread, NULL, recieveFromClient, (void*)cl);

	}
	close(sd);
	return 1;
}

/*
 * Function to recieve data from connected client, each connection is handled 
 * on a different thread. Client information is stored, mutex is locked, data
 * is recieved and set back to client. After data is recieved information is stored
 * in the appropriate structures. 
 */
void* recieveFromClient(void *client) 
{
	char		*bp, buf[BUF_LENGTH];
	char* 		clientAddress;
	int		n, bytes_to_read, socket, arrayPos;
        clientInfo 	*cl = (clientInfo *)client; 

	socket 		= cl->socket;
	clientAddress 	= cl->ip;
	printf("Current Active Hosts: %d\n", ++activeConnections);
	
		arrayPos = totalHosts;
		host[arrayPos].status = "true";
		host[arrayPos].ip = clientAddress;
		host[arrayPos].numOfConnections = 0;
		host[arrayPos].numOfBytesSent = 0;
		
	// mutex to lock 
	pthread_mutex_lock(&mutex);
	//increment total host connections
	totalHosts++;
	
	bp = buf;
	bytes_to_read = BUF_LENGTH;
	while ((n = recv (socket, bp, bytes_to_read, 0)) < BUF_LENGTH)
	{
		bp += n;
		bytes_to_read -= n;
	}

	host[arrayPos].numOfConnections++;
	host[arrayPos].numOfBytesSent += n;
        printf ("Total Host Connections: %d\n", totalHosts);
	printf ("Client IP: %s\n", host[arrayPos].ip);
	printf ("Number of Connections by Client: %d\n", host[arrayPos].numOfConnections);
	printf ("Number of Bytes Recieved: %d\n", host[arrayPos].numOfBytesSent);
	// send data on socket
	send (socket, buf, BUF_LENGTH, 0);
	
	close (socket);
	free(client);

	// after sending data decrement total number of active hosts
	activeConnections--;
	host[arrayPos].status = "false";

	// mutex unlock
	pthread_mutex_unlock(&mutex);
	
	return NULL;
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message) 
{
    perror (message);
    exit (EXIT_FAILURE);
}
