/* 
 * File:   common.h
 * Author: root
 *
 * Created on February 15, 2014, 2:05 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>



#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	1024		//Buffer length
#define LISTENQ	5
#define EPOLL_QUEUE_LEN 256
#define VAL(str) #str
#define TOSTRING(str) VAL(str)

    // GLOBALS
    ssize_t writeResult;
    const char sendbuf[BUFLEN];

    // llist.c 
    typedef struct _llist llist;
    typedef struct _node node;

    struct _llist {
        node *link;
    };

    struct _node {
        node *next;
        void *data;
    };

    // s_svr.c
    typedef struct _client client;
    typedef struct _server server;

    struct _client {
        int fd;
        int n_bytes_received;
        pthread_t tid;
        struct sockaddr_in sa;
        socklen_t sa_len;
        bool quit;
    };

    struct _server {
        int listen_sd;
        int maxfd;
        int port;
        int n_clients;
        int n_max_connected;
        int n_max_bytes_received;
        pid_t pid;
        bool running;
        pthread_mutex_t dataLock;

        /* for epoll on client connections */
        int epoll_fd;
        int num_fds;
        struct epoll_event events[EPOLL_QUEUE_LEN];
        struct epoll_event event;
        llist *e_client_list;

        /* for select on client connections*/
        fd_set allset;
        llist *client_list;
        //int clients[FD_SETSIZE];
        //client *clientConn[FD_SETSIZE];
    };

    // tcp_clnt.c
    typedef struct _data data;

    struct _data {
        int numOfRequests;
        int dataSent;
        int interval;
        int sd;
        double time;
        char sendBuff[BUFLEN];
        struct timeval start;
        struct timeval end;
    };

    // FUNCTION PROTOTYPES tcp_clnt.c
    void connect_to_server(data *, char *, int);
    void print_client_data(data *);
    void signal_handler(int);

    // FUNCTION PROTOTYPES llist.c
    bool client_compare(const void *, const void *);
    llist* llist_new(void);
    void llist_free(llist *l, void (*free_func)(void*));
    llist* llist_append(llist *l, void *data);
    llist* llist_remove(llist *l, void *data, bool (*compare_func)(const void*, const void*));
    int llist_length(llist *l);
    bool llist_is_empty(llist *l);

    // FUNCTION PROTOTYPES s_svr.c & e_svr.c
    void SystemFatal(const char*);
    void signal_Handler(int);
    server* server_new(void);
    void server_init(server *);
    void* client_manager(void *);
    void read_from_socket(server *);
    client* client_new(void);
    void process_client_data(client *, server *);
    void process_client_req(client *, server *);
    void print_server_data(server *);


#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

