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

#include <sys/time.h>
#include <sys/select.h>

#include "DLinkedList.h"

struct sigaction act_new;
struct sigaction act_old;

struct sockaddr_in proxy_addr;
int proxy_socket; // descriptors rturn from socket and accept system calls
int proxy_port; // port number

struct request_msg
{
    char _method[MAX_BUF_SIZE];
    char _url[MAX_BUF_SIZE];
    char _url_object[MAX_BUF_SIZE];
    char _port[MAX_BUF_SIZE];
    char _vhttp[MAX_BUF_SIZE];
    char _host[MAX_BUF_SIZE];
};

void parse_reqm(char*, struct request_msg*);
void *accept_operation(void*);
void *func_th(void*);

// void assign_request_msg(struct request_msg*);
// void free_request_msg(struct request_msg*);

void closeall();

DLinkedList data_cache;
pthread_cond_t cond_t = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_t = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t fmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t smutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mmutex = PTHREAD_MUTEX_INITIALIZER;

#endif // PROXY_H
