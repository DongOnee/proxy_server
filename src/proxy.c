#include "proxy.h"

#define DEBUG_FLAG 0x1
#define MAX_THREAD 0x20

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void debug(char *msg, ...)
{
    #ifdef DEBUG_FLAG
        printf(msg);
    #endif
}

int main(int argc, char *argv[]) 
{
    // error detect
    if (argc < 2) error("ERROR, no port provided\n");
    proxy_port = atoi(argv[1]);

    // cacheing memory init
    data_cache.remainder_size = MAX_CACHE_SIZE;

    /**
     * Create a new socket
     * AF_INET: Address Domain is Internet 
     * SOCK_STREAM: Socket Type is STREAM Socket
     */
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket < 0) error("ERROR, opening proxy socket");

    /**
     * For "avoid binding error : Address already in use."
     * int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
     */
    int val = 1;
    if (setsockopt(proxy_socket, SOL_SOCKET, SO_REUSEADDR, (void *) &val, sizeof(val)) < 0) error("ERROR, setsockopt function error");

    /* SET PROXY ADDRESS */
    bzero(&proxy_addr, sizeof(struct sockaddr_in));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port); //convert from host to network byte order
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
 
    //Bind the socket to the server address
    if ( bind(proxy_socket, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0 )  error("ERROR, on binding");
 
    // Listen for socket connections. Backlog queue (connections to wait) is 5
    if ( listen(proxy_socket, 5) == -1 ) error("ERROR, on listen");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // for multi-thread
    pthread_t thid[MAX_THREAD];
    int client_socket[MAX_THREAD];
    int idx_th = 0, status, state;

    // client info
    struct sockaddr_in client_addr[MAX_THREAD];
    
    fd_set reads, cpy_reads;
    int fd_num;
    struct timeval timeout;
	FD_ZERO(&reads);
    
// printf("using socket file descriptor : %d\n", proxy_socket);
    while (1)
    {
    // pthread_mutex_lock(&mutex_t);
    
    // if ((status = pthread_create(&thid[idx_th], NULL, func_th, (void*) &proxy_socket)) < 0)
    // {
    //     perror("thread create error");
    //     exit(0);
    // }

    // pthread_cond_wait(&cond_t, &mutex_t);
    // pthread_mutex_unlock(&mutex_t);

    // pthread_detach(thid[idx_th]);

    // idx_th++;
    // idx_th %= MAX_THREAD;

        ////////////////////////////////////////////////////////////
        /**
         * accept function
         * 1) Block until a new connection is established
         * 2) the new socket descriptor will be used for subsequent communication with the newly connected client.
        */

    // FD_SET(proxy_socket, &reads);
    // timeout.tv_sec = 2;
    // timeout.tv_usec = 0;

    // if ( (fd_num = select(proxy_socket+1, &reads, 0, 0, &timeout)) == -1) error("ERROR on select()");
    // else if (fd_num == 0)
    // {
    //     printf("fd_num ERROR\n");
    //     continue;
    // }
    // else if (FD_ISSET(proxy_socket, &reads))
    // {
    //     socklen_t client_length = sizeof(struct sockaddr_in);
        
    //     client_socket[idx_th] = accept(proxy_socket, (struct sockaddr *) &client_addr[idx_th], &client_length);
    //     if ( client_socket[idx_th] < 0 ) error("ERROR on accept");

    //     // check sever connection
    //     printf( " - server got connection from client : %s\n"
    //             " - using socket file descriptor      : %d\n", inet_ntoa(client_addr[idx_th].sin_addr), client_socket[idx_th]);

    //     if ((status = pthread_create(&thid[idx_th], NULL, accept_operation, (void*) &client_socket[idx_th])) < 0)
    //         error("Thread Create Error");

    //     pthread_detach(thid[idx_th]);
    // }

        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////


        pthread_mutex_lock(&mutex_t);
        socklen_t client_length = sizeof(struct sockaddr_in);
        
        client_socket[idx_th] = accept(proxy_socket, (struct sockaddr *) &client_addr[idx_th], &client_length);
        if ( client_socket[idx_th] < 0 ) error("ERROR on accept");

        // check sever connection
        printf( " - server got connection from client : %s\n"
                " - using socket file descriptor      : %d\n", inet_ntoa(client_addr[idx_th].sin_addr), client_socket[idx_th]);
        
        if ((status = pthread_create(&thid[idx_th], NULL, accept_operation, (void*) &client_socket[idx_th])) < 0)
            error("Thread Create Error");
        pthread_cond_wait(&cond_t, &mutex_t);
        printf("condition wait\n");
        pthread_mutex_unlock(&mutex_t);

        pthread_detach(thid[idx_th]);


        // pthread_join(thid[idx_th], (void**)&state);
        // printf("\t\tstate:%d\n", state);
        
        idx_th = (idx_th+1) % MAX_THREAD;
        sleep(1);
    }

    close(proxy_socket);
    return 0;
}


void parse_reqm(char *message, struct request_msg *saved)
{
    char tmp_message[MAX_REQ_MSG+1];
    char tmp_URL[MAX_BUF_SIZE];
    char tmp_host[MAX_BUF_SIZE];
    char tmp_object_path[MAX_BUF_SIZE];
    char option_name[64];
    char option_value[MAX_BUF_SIZE];
    
    char* ptr_object;
    char* ptr_port;
    char* line_parser;
    char* host_parser;
    int isHost = 0;
    
    strcpy(tmp_message, message);

    line_parser = strtok(tmp_message, "\r\n");
    sscanf(line_parser, "%s %s %s", saved->_method, tmp_URL, saved->_vhttp);
    strcpy(saved->_url, tmp_URL);
    host_parser = strtok(NULL, "\r\n");

    line_parser = strtok(tmp_URL, "//");
    line_parser = strtok(NULL, "//");
    if ( (ptr_port = strchr(line_parser, ':')) == NULL) 
    {
        strcpy(saved->_port, "80");
        strcpy(tmp_host, line_parser);
    }
    else 
    {
        strcpy(saved->_port, ptr_port+1);
        *ptr_port = '\0';
        strcpy(tmp_host, line_parser);
        *ptr_port = ':';
    }
    // printf("port : %s\n", saved->_port);
    
    ptr_object = strtok(NULL, "/");
    if (ptr_object == NULL) tmp_object_path[0] = '/';
    else
    {
        while (ptr_object != NULL)
        {
            strcat(tmp_object_path, "/");
            strcat(tmp_object_path, ptr_object);
            ptr_object = strtok(NULL, "/");
        }
    }
    strcpy(saved->_url_object, tmp_object_path);
    // printf("path : %s\n", saved->_url_object);

    while (host_parser) 
    {
        sscanf(line_parser, "%s %s", option_name, option_value);

        if (strcmp(option_name, "Host:")==0||strcmp(option_name, "host:")==0)
        {
            strcpy(saved->_host, option_value);
            isHost = 1;
            break;
        }
        host_parser = strtok(NULL, "\r\n");
    }
    if (!isHost) strcpy(saved->_host, tmp_host);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
    
// while ((line_parser = strtok(NULL, "\r\n")) != NULL) sscanf(line_parser, "Host: %s", tmp_host);

// while ((line_parser = strtok(NULL, "\r\n")) != NULL) 
// {
//     sscanf(line_parser, "%s %s", option_name, option_value);

//     if (strcmp(option_name, "Host:")==0||strcmp(option_name, "host:")==0)
//     {
//         strcpy(tmp_host, option_value);
//         isHost = 1;
//         break;
//     }
// }

// // build URL of requested Object
// char* url_parser = strtok(tmp_URL, "/");
// url_parser = strtok(NULL, "/");
// if ( (url_parser = strtok(NULL, "/")) == NULL ) strcat(saved->_url_object, "/");
// else {
//     strcat(saved->_url_object, "/");
//     strcat(saved->_url_object, url_parser);
//     while((url_parser = strtok(NULL, "/"))!=NULL)
//     {
//         strcat(saved->_url_object, "/");
//         strcat(saved->_url_object, url_parser);
//     }
// }

// // build host URL and port
// char *port_parser = strpbrk(tmp_host, ":");
// if (port_parser == NULL)
// {
//     strcpy(saved->_port, "80");
//     strcpy(saved->_host, tmp_host);
// }
// else
// {
//     strcpy(saved->_port, port_parser+1);
//     strcpy(saved->_host, strtok(tmp_host, ":"));
// }
}


void *accept_operation(void *_client_sock)
{
    pthread_mutex_lock(&fmutex);
    int client_sock = *(int*)_client_sock;
    
    Node *searched_node;
    time_t current_time;
    struct request_msg saved;
    struct sockaddr_in host_addr;

    char request_message[MAX_REQ_MSG+1];
    char tmp_req_msg[MAX_REQ_MSG+1];
    char respon_message[MAX_OBJECT_SIZE+1];
    char receive_buffer[MAX_BUF_SIZE+1];
    char option_name[64];
    char option_value[MAX_BUF_SIZE];
    char logs[MAX_BUF_SIZE];
    int length_req_msg=0;
    int length_res_msg=0;

    bzero(&host_addr, sizeof(host_addr));
    bzero(request_message, MAX_REQ_MSG+1);
    bzero(tmp_req_msg, MAX_REQ_MSG+1);
    bzero(respon_message, MAX_OBJECT_SIZE+1);
    bzero(receive_buffer, MAX_BUF_SIZE+1);
    bzero(logs, MAX_BUF_SIZE);

    // read section~~~~ we can see http reqeust message using buffer.
    length_req_msg = read(client_sock, (void*) request_message, MAX_REQ_MSG); //Read is a block function. 
    if (length_req_msg <= 0)
    {
        fprintf(stderr, "ERROR, read Client Sock fail\n");
        close(client_sock);
        pthread_exit(0);
    }

    pthread_cond_signal(&cond_t);
    pthread_mutex_unlock(&fmutex);

    debug(  "\n===========  1st  R E Q U E S T  M E S S A G E  ===========\n%s"
            "\n========================  E  N  D  ========================\n\n", request_message);

    /* P A R S I N G  D A T A */
    parse_reqm(request_message, &saved);
    if (strcmp(saved._method, "GET") != 0)
    { 
        fprintf(stderr, "NO GET METHOD\n");
        close(client_sock);
        pthread_exit(0);
    }

    // smutex lock.............
    pthread_mutex_lock(&smutex);
    searched_node = search(&data_cache, saved._url);
    pthread_mutex_unlock(&smutex);

    if (searched_node != NULL)
    {
        printf( "\n.______________."
                "\n|              |"
                "\n|  H  I  T  !  |"
                "\n|______________|\n\n");

        debug(  "\n=============  S E A R C H E D   O B J E C T  =============\n%s"
                "\n========================  E  N  D  ========================\n\n", searched_node->object);

        length_res_msg = searched_node->object_size;
        char* temp_m = searched_node->object;

        // read respons message
        int remain_send = searched_node->object_size;
        char* ptr_send = searched_node->object;
        while (0 < remain_send)
        {
            int size_send = write(client_sock, ptr_send, remain_send);
            if (size_send == -1) break;

            remain_send -= size_send;
            ptr_send += size_send;
        }
    } 
    else
    {
        printf( "\n.______________."
                "\n|              |"
                "\n|  M  I  S  S  |"
                "\n|______________|\n\n");

        strcpy(tmp_req_msg, request_message);
        char* line_parser = strtok(tmp_req_msg, "\r\n");
        
        bzero((char*)request_message, MAX_REQ_MSG+1);
        sprintf(request_message, "%s %s %s\r\n", saved._method, saved._url_object, saved._vhttp);
        // sprintf(request_message, "%s\r\n", line_parser);
        while ( (line_parser = strtok(NULL, "\r\n")) != NULL )
        {
            sscanf(line_parser, "%s %s", option_name, option_value);

            if (strcmp(option_name, "Connect:")==0||
                strcmp(option_name, "connect:")==0||
                strcmp(option_name, "Proxy-Connection:")==0) strcat(request_message, "Connect: close");
            else strcat(request_message, line_parser);
            strcat(request_message, "\r\n");
        }
        
            // line_parser = strtok(NULL, "\r\n");

        // rebuild request message
            // bzero((char*)request_message, MAX_REQ_MSG);
            // sprintf(request_message, "%s %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", saved._method, saved._url, saved._vhttp, saved._host);
        // while( (line_parser = strtok(NULL,"\n")) != NULL )
        // {
        //     if (strcmp(line_parser, "Proxy-Connection: keep-alive\r") == 0 || strcmp(line_parser, "Connection: keep-alive\r")== 0)
        //         strcat(request_message, "Connection: close\r");
        //     else strcat(request_message, line_parser);
        //     strcat(request_message, "\n");
        // }

        strcat(request_message, "\r\n");
    
        debug(  "\n============ 2nd  R E Q U E S T  M E S S A G E ============\n%s"
                "\n========================  E  N  D  =========================\n\n", request_message);

        // re-setting socket option
        int host_port = atoi(saved._port);
        int host_socket;
        struct hostent *host_server = gethostbyname(saved._host); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
        if (host_server == NULL)
        { 
            fprintf(stderr, "ERROR, no such host\n");
            close(client_sock);
            pthread_exit(0);
        }

        //create a new socket
        pthread_mutex_lock(&mmutex);
        if ((host_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        { 
            fprintf(stderr, "ERROR opening host socket\n");
            close(client_sock);
            pthread_mutex_unlock(&mmutex);
            pthread_exit(0);
        }
        pthread_mutex_unlock(&mmutex);

        host_addr.sin_family = AF_INET;
        host_addr.sin_port = htons(host_port);
        host_addr.sin_addr.s_addr = *((unsigned long *)host_server->h_addr_list[0]);
        // host_addr.sin_addr.s_addr = inet_addr((struct in_addr*)server->h_addr_list[0]);
        // bcopy((char*) server->h_addr_list[0], (char*) &host_addr.sin_addr.s_addr, server->h_length);

        // connect
        if (connect(host_socket, (struct sockaddr *) &host_addr, sizeof(host_addr)) < 0) //establish a connection to the server
        { 
            fprintf(stderr, "ERROR connecting\n");
            close(client_sock);
            pthread_exit(0);
        }
        printf(" - server got connection from host server : %s\n", inet_ntoa(host_addr.sin_addr));

        // write request_message to server
        // if (send(host_socket, request_message, strlen(request_message)+1, 0) < 0) error("ERROR sending to socket");
        if (write(host_socket, request_message, strlen(request_message)+1) < 0)
        { 
            fprintf(stderr, "ERROR writing to host socket\n");
            close(client_sock);
            pthread_exit(0);
        }

        // read respons message
        int over_object_size_flag = 1;
        int receive_size;


        while( (receive_size = read(host_socket, receive_buffer, MAX_BUF_SIZE)) > 0)
        {
            printf("%s\n", receive_buffer);
            length_res_msg += receive_size;
            write(client_sock, receive_buffer, receive_size);
            if (length_res_msg > MAX_OBJECT_SIZE && over_object_size_flag)
            {
                bzero(respon_message, MAX_OBJECT_SIZE+1);
                over_object_size_flag = 0;
            }
            else if (over_object_size_flag) strcat(respon_message, receive_buffer);
            bzero(receive_buffer, MAX_BUF_SIZE+1);
        }

        close(host_socket);

        debug(  "============  R E S P O N S E   M E S S A G E  ============\n%s\n"
                "========================  E  N  D  ========================\n\n", respon_message);

        if ( strncmp(respon_message, "HTTP/1.1 200 OK", 13)==0 && over_object_size_flag)
        {
            pthread_mutex_lock(&lmutex);
            add(&data_cache, node_init(saved._url, respon_message, length_res_msg+1));
            pthread_mutex_unlock(&lmutex);
        }
    }

    // logging 
    time(&current_time);
    char* timeptr = strtok(ctime(&current_time), "\n");
    sscanf(logs, "%s EST: %s %s %d\n", timeptr, inet_ntoa(host_addr.sin_addr), saved._url, &length_res_msg);

    pthread_mutex_lock(&rmutex);
    int log_fd;
    if ( (log_fd = open("./proxy.log", O_CREAT|O_WRONLY|O_APPEND, 0644)) < 0 ) error("logsfile error");
    write(log_fd, logs, strlen(logs));
    close(log_fd);
    pthread_mutex_unlock(&rmutex);

// char* timeptr = strtok(ctime(&current_time), "\n");
// strcat(logs, " EST: ");
// strcat(logs, inet_ntoa(host_addr.sin_addr));
// strcat(logs, " ");
// strcat(logs, saved._url);
// strcat(logs, " ");
// char mlen[10];
// sprintf(mlen, "%d", length_res_msg);
// strcat(logs, mlen);
// strcat(logs, "\n");

// pthread_mutex_lock(&rmutex);
// int log_fd;
// if ( (log_fd = open("./proxy.log", O_CREAT|O_WRONLY|O_APPEND, 0644)) < 0 ) error("logsfile error");
// write(log_fd, logs, strlen(logs));
// close(log_fd);
// pthread_mutex_unlock(&rmutex);

    // free(logs);

    close(client_sock);

    pthread_exit(0);
}