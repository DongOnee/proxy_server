/* 
   A simple server in the internet domain using TCP
   Usage:./server port (E.g. ./server 10000 )
*/
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

void parse_reqm(char*, char*, char*, char*, char*, char*, char*);
void *t_function(void*);
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int sockfd; // descriptors rturn from socket and accept system calls
int portno; // port number
struct sockaddr_in proxy_addr, cli_addr, end_addr;

struct sigaction act_new;
struct sigaction act_old;

void sigint_handler(int signo) {
    bzero((char *) &proxy_addr, sizeof(struct sockaddr_in));
    close(sockfd);
    exit(1);
}

void closeall() {
    printf("\nbye~\n");
    close(sockfd);
}

LinkedList* g_heap;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lmutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) 
{
    // error detect
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // init new signal act
    act_new.sa_handler = sigint_handler;
    sigemptyset(&act_new.sa_mask);
    sigaction(SIGINT, &act_new, &act_old);

    // for multi-thread
    pthread_t p_thread[100];

    socklen_t clilen;
    /*sockaddr_in: Structure Containing an Internet Address*/

    // cacheing memory init
    g_heap = slinit();

    /*Create a new socket
    AF_INET: Address Domain is Internet 
    SOCK_STREAM: Socket Type is STREAM Socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");


    /* SET PROXY ADDRESS */
    bzero((char *) &proxy_addr, sizeof(struct sockaddr_in));
    portno = atoi(argv[1]); //atoi converts from String to Integer
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
    proxy_addr.sin_port = htons(portno); //convert from host to network byte order
 
    if (bind(sockfd, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0) //Bind the socket to the server address
        error("ERROR on binding");
 
    if (listen(sockfd,10) == -1) // Listen for socket connections. Backlog queue (connections to wait) is 5
        error("ERROR on listen");
    int i =0;
    clilen = sizeof(cli_addr);
    while (1) {
        /*accept function: 
        1) Block until a new connection is established
        2) the new socket descriptor will be used for subsequent communication with the newly connected client.
        */
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        if(pthread_create(&p_thread[i], NULL, (*t_function), (void*)&newsockfd) <0 ) {
            perror("thread create error");
        }
        pthread_join(p_thread[i], NULL);
        i++;
        i=i%100;
    }
    close(sockfd);
    return 0; 
}



void parse_reqm(char* reqm, char* mathod, char* URL, char* URL_obj, char* port, char* vhttp, char* host) {
    int reqlen = strlen(reqm);
    char url_tmp[MAX_BUF_SIZE];
    char host_tmp[MAX_BUF_SIZE];

    char reqm_tmp[reqlen];
    strcpy(reqm_tmp, reqm);
    char* line_parse = strtok(reqm_tmp, "\r\n");
    sscanf(line_parse, "%s %s %s", mathod, url_tmp, vhttp);
    line_parse = strtok(NULL, "\r\n");
    sscanf(line_parse, "Host: %s", host_tmp);
    strcpy(URL, url_tmp);

    char* ptr_url = strtok(url_tmp, "/");
    ptr_url = strtok(NULL, "/");
    if ((ptr_url = strtok(NULL, "/"))==NULL) strcat(URL_obj, "/");
    else {
        strcat(URL_obj, "/");
        strcat(URL_obj, ptr_url);
        while((ptr_url = strtok(NULL, "/"))!=NULL) {
            strcat(URL_obj, "/");
            strcat(URL_obj, ptr_url);
        }
    }

    int s_flag=0;
    for (int m=0; m<strlen(host_tmp); m++) if (host_tmp[m]==':') s_flag=1;
    if (s_flag) {
        char* port_ptr = strtok(host_tmp, ":");
        strcpy(host, port_ptr);
        port_ptr = strtok(NULL, ":");
        strcpy(port, port_ptr);
    } else {
        strcpy(host, host_tmp);
        strcpy(port, "80");
    }
}

void *t_function(void* newsockfd) {
    char* requestname = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* mathod = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* URL = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* vhttp = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* host = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* port = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* req_message = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* respon_message = (char*) malloc(sizeof(char)*MAX_OBJECT_SIZE);
    bzero((char*)requestname, MAX_BUF_SIZE);
    bzero((char*)mathod, MAX_BUF_SIZE);
    bzero((char*)URL, MAX_BUF_SIZE);
    bzero((char*)vhttp, MAX_BUF_SIZE);
    bzero((char*)host, MAX_BUF_SIZE);
    bzero((char*)port, MAX_BUF_SIZE);
    bzero((char*)req_message, MAX_BUF_SIZE);
    bzero((char*)respon_message, MAX_OBJECT_SIZE);
    int reqlen=0;
    int reslen=0;
    int endportno=0;

    // check sever connection
    printf("server got connection from client : %s\n", inet_ntoa(cli_addr.sin_addr));

    // read section~~~~ we can see http reqeust message using buffer.
    reqlen = read(*(int*)newsockfd, req_message, MAX_BUF_SIZE); //Read is a block function. It will read at most 1024 bytes
    if (reqlen < 0) error("ERROR reading from socket"); // error
    else if (reqlen==0) { // 가끔 버퍼에 아무것도 채워지지 않는 경우가 발생한다. 이 문제가 발생하여도 서버가 다운되지 않도록 조치 하였다.
        printf("error~~\nreqlen=0\nrestart!!\n\n\n");
        close(*(int*)newsockfd);
        return;
    }
    printf("====================1st request message====================\n%s\n====================================end====================\n\n", req_message);

    /***********PARSING DATA***********/
    parse_reqm(req_message, mathod, URL, requestname, port, vhttp, host);
    if (strcmp(mathod, "GET")!=0) { 
        printf("NO GET...\n");
        close(*(int*)newsockfd);
        return;
    }

    //hit or miss
    Node *searched;
    int n;
    struct hostent *server; //conaatains tons of information, including the server's IP address
    // mutex lock.............
    pthread_mutex_lock(&mutex);
    searched = search(g_heap, URL);
    pthread_mutex_unlock(&mutex);
    if (searched != NULL) {
        printf("================\n");
        printf("==HITTTTTTTT!!==\n");
        printf("================\n\n");
        printf("====================searched->object====================\n%s\n====================================end====================\n\n", searched->object);
        // read respons message
        int obsize=searched->object_size;
        reslen=searched->object_size;
        int total=0;
        char* temp_m = searched->object;
        while (0 < obsize) {
            if (obsize>=MAX_BUF_SIZE) n=send(*(int*)newsockfd, temp_m, MAX_BUF_SIZE, 0);
            else n=send(*(int*)newsockfd, temp_m, obsize+1, 0);
            obsize -= n;
            temp_m +=n;
        }
    } else {
        printf("================\n");
        printf("==MISSSSSSSS!!==\n");
        printf("================\n\n");

        char tmpm[reqlen+1];
        bzero((char*)tmpm, reqlen+1);
        strcpy(tmpm, req_message);
        char* line_parse = strtok(tmpm, "\r\n");
        line_parse = strtok(NULL, "\r\n");

        bzero((char*)req_message, MAX_BUF_SIZE);
        sprintf(req_message, "%s %s %s\r\nHost: %s\r\n", mathod, requestname, vhttp, host);
        while((line_parse=strtok(NULL,"\n"))!=NULL) {
            if (strcmp(line_parse, "Proxy-Connection: keep-alive\r")==0 || strcmp(line_parse, "Connection: keep-alive\r")==0) strcat(req_message, "Connection: close\r");
            else strcat(req_message, line_parse);
            strcat(req_message, "\n");
        }
        printf("====================2st request message====================\n%s\n====================================end====================\n\n", req_message);

        endportno = atoi(port);
        server = gethostbyname(host); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }

        int endsockfd = socket(AF_INET, SOCK_STREAM, 0); //create a new socket

        bzero((char *) &end_addr, sizeof(struct sockaddr_in));

        end_addr.sin_family = AF_INET; //initialize server's address
        bcopy((char *)server->h_addr_list, (char *)&end_addr.sin_addr.s_addr, server->h_length);
        end_addr.sin_port = htons(endportno);

        // connect
        if (connect(endsockfd,(struct sockaddr *)&end_addr,sizeof(end_addr)) < 0) //establish a connection to the server
            error("ERROR connecting");
        printf("server got connection from end server : %s\n", inet_ntoa(end_addr.sin_addr));

        // write req_message to server
        n=send(endsockfd, req_message, strlen(req_message)+1, 0);
        if (n < 0) error("ERROR writing to socket");

        // read respons message
        char* temp_m = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
        char* ptr_res = respon_message;
        do {
            bzero(temp_m, MAX_BUF_SIZE);
            n=recv(endsockfd, temp_m, MAX_BUF_SIZE,0);
            reslen += n;
            if(n>0) {
                send(*(int*)newsockfd, temp_m, n, 0);
                if( reslen > MAX_OBJECT_SIZE) bzero((char*)respon_message, MAX_OBJECT_SIZE);
                else {
                    memcpy(ptr_res, temp_m, n);
                    ptr_res +=n;
                }
            }
        } while(n>0);
        free(temp_m);

        /**********read respon_message**********/
        printf("==================== respon_message ====================\n%s\n====================================end====================\n\n", respon_message);
        if ((strncmp(respon_message, "HTTP/1.1 200 OK", 13)==0)&& reslen < MAX_OBJECT_SIZE) {
            pthread_mutex_lock(&mutex);
            add(g_heap, nodeinit(URL, respon_message, reslen+1));
            pthread_mutex_unlock(&mutex);
        }
    }

    // logging 
    time_t current_time;
    time(&current_time);
    char* timeptr = strtok(ctime(&current_time), "\n");
    char* logs = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    bzero(logs, MAX_BUF_SIZE);
    strcat(logs, timeptr);
    strcat(logs, " EST: ");
    strcat(logs, inet_ntoa(end_addr.sin_addr));
    strcat(logs, " ");
    strcat(logs, URL);
    strcat(logs, " ");
    char mlen[10];
    sprintf(mlen, "%d", reslen);
    strcat(logs, mlen);
    strcat(logs, "\n");

    pthread_mutex_lock(&lmutex);
    int fd;
    if ((fd = open("./proxy.log",O_CREAT|O_WRONLY|O_APPEND, 0644))<0) error("logsfile error");
    write(fd, logs, strlen(logs));
    pthread_mutex_unlock(&lmutex);

    // free(new_req_message);
    close(*(int*)newsockfd);
    free(logs);

    free(requestname);
    free(mathod);
    free(URL);
    free(vhttp);
    free(host);
    free(port);
    free(req_message);
    free(respon_message);
}

