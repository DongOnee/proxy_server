#if !defined(PROXY_H)
#define PROXY_H
#define _XOPEN_SOURCE

#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#include "DLinkedList.h"

struct sigaction act_new;
struct sigaction act_old;

int proxy_socket_fd; // descriptors rturn from socket and accept system calls
int proxy_port; // port number
struct sockaddr_in proxy_addr;

struct request_msg
{
    char *_method;
    char *_url;
    char *_url_object;
    char *_port;
    char *_vhttp;
    char *_host;
};

void parse_reqm(char*, struct request_msg*);
void* accept_operation(void*);
void assign_request_msg(struct request_msg*);
void free_request_msg(struct request_msg*);

void closeall();

DLinkedList* data_cache;
pthread_cond_t cond_t = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_t = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kmutex = PTHREAD_MUTEX_INITIALIZER;

#endif // PROXY_H
