/* 
   A simple server in the internet domain using TCP
   Usage:./proxy port (E.g. ./proxy 10000 )
*/

#include "proxy.h"

#define DEBUG_FLAG 0

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void debug(char *msg, ...)
{
    if (DEBUG_FLAG) printf(msg);
}

void closeall()
{
    printf("\nbye~\n");
    close(proxy_socket_fd);
}

int main(int argc, char *argv[]) 
{
    // error detect
    if (argc < 2) error("ERROR, no port provided\n");

    // cacheing memory init
    data_cache = linkedList_init();

    /**
     * Create a new socket
     * AF_INET: Address Domain is Internet 
     * SOCK_STREAM: Socket Type is STREAM Socket
     */
    proxy_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (proxy_socket_fd < 0) error("ERROR opening socket");

    // For avoid binding error : Address already in use.
    int val = 1;
    if (setsockopt(proxy_socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(val)) < 0)
    {
        perror("setsockopt");
        close(proxy_socket_fd);
        return -1;
    }

    /* SET PROXY ADDRESS */
    bzero(&proxy_addr, sizeof(struct sockaddr_in));
    proxy_port = atoi(argv[1]); //atoi converts from String to Integer

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
    proxy_addr.sin_port = htons(proxy_port); //convert from host to network byte order
 
    //Bind the socket to the server address
    if (bind(proxy_socket_fd, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0)  error("ERROR on binding");
 
    // Listen for socket connections. Backlog queue (connections to wait) is 5
    if (listen(proxy_socket_fd, 10) == -1) error("ERROR on listen");

    // for multi-thread
    pthread_t p_thread[15];
    int accept_fd[15];
    int thid[15];
    int i =0;
    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);
    
    while (1)
    {
        /**
         * accept function
         * 1) Block until a new connection is established
         * 2) the new socket descriptor will be used for subsequent communication with the newly connected client.
        */
        accept_fd[i] = accept(proxy_socket_fd, (struct sockaddr *) &client_addr, &clilen);
        if ( accept_fd[i] < 0 ) error("ERROR on accept");

        // check sever connection
        printf("server got connection from client : %s\nusing socket file descriptor : %d\n", inet_ntoa(client_addr.sin_addr), accept_fd[i]);
        pthread_mutex_lock(&mutex_t);
        thid[i] = pthread_create(p_thread+i, NULL, accept_operation, (void*) &accept_fd[i]);
        pthread_cond_wait(&cond_t, &mutex_t);
        pthread_mutex_unlock(&mutex_t);
        
        if ( thid[i] < 0 ) perror("thread create error");
        else 
        {
            pthread_detach(p_thread[i]);
            i = (i+1) % 15;
        }
        // pthread_join( p_thread[i], NULL );
    }

    close(proxy_socket_fd);
    return 0;
}


void parse_reqm(char *message, struct request_msg *saved)
{
    int reqlen = strlen(message);
    char url_tmp[MAX_BUF_SIZE];
    char host_tmp[MAX_BUF_SIZE];

    char message_tmp[reqlen+1];
    strcpy(message_tmp, message);

    char* line_parse = strtok(message_tmp, "\r\n");
    sscanf(line_parse, "%s %s %s", saved->_method, url_tmp, saved->_vhttp);
    
    line_parse = strtok(NULL, "\r\n");
    sscanf(line_parse, "Host: %s", host_tmp);
    strcpy(saved->_url, url_tmp);

    // build ULR of requested Object
    char* ptr_url = strtok(url_tmp, "/");
    ptr_url = strtok(NULL, "/");
    if ( (ptr_url = strtok(NULL, "/")) == NULL ) strcat(saved->_url_object, "/");
    else {
        strcat(saved->_url_object, "/");
        strcat(saved->_url_object, ptr_url);
        while((ptr_url = strtok(NULL, "/"))!=NULL)
        {
            strcat(saved->_url_object, "/");
            strcat(saved->_url_object, ptr_url);
        }
    }

    // build host URL and port
    char *port_ptr2 = strpbrk(host_tmp, ":");
    if (port_ptr2 == NULL)
    {
        strcpy(saved->_port, "80");
        strcpy(saved->_host, host_tmp);
    }
    else
    {
        strcpy(saved->_port, port_ptr2+1);
        strcpy(saved->_host, strtok(host_tmp, ":"));
    }
}


void assign_request_msg(struct request_msg *saved)
{
    saved->_method = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_url = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_url_object = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_port = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_vhttp = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_host = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);

    bzero(saved->_method, MAX_BUF_SIZE);
    bzero(saved->_url, MAX_BUF_SIZE);
    bzero(saved->_url_object, MAX_BUF_SIZE);
    bzero(saved->_port, MAX_BUF_SIZE);
    bzero(saved->_vhttp, MAX_BUF_SIZE);
    bzero(saved->_host, MAX_BUF_SIZE);
}

void free_request_msg(struct request_msg* saved)
{
    free(saved->_method);
    free(saved->_url);
    free(saved->_url_object);
    free(saved->_port);
    free(saved->_vhttp);
    free(saved->_host);
}


void *accept_operation(void *_socket_fd)
{
    pthread_mutex_lock(&mutex_t);

    int socket_fd = *(int*)_socket_fd;
    struct request_msg saved;
    assign_request_msg(&saved);
    struct sockaddr_in end_addr;

    char req_message[MAX_BUF_SIZE+1];
    char respon_message[MAX_OBJECT_SIZE+1];
    int reqlen=0;
    int reslen=0;

    pthread_cond_signal(&cond_t);
    printf("accept_operation start socket_fd:%d\n", socket_fd);
    pthread_mutex_unlock(&mutex_t);

    // read section~~~~ we can see http reqeust message using buffer.
    reqlen = recv(socket_fd, (void*) req_message, MAX_BUF_SIZE, 0); //Read is a block function. It will read at most 1024 bytes
    if (reqlen < 0) error("ERROR reading from socket"); // error
    else if (reqlen == 0) // 가끔 버퍼에 아무것도 채워지지 않는 경우가 발생한다. 이 문제가 발생하여도 서버가 다운되지 않도록 조치 하였다.
    {
        printf("received message's length is zero\n");
        close(socket_fd);
        return NULL;
    }
    debug("====================1st request message====================\n%s\n====================================end====================\n\n", req_message);

    /***********PARSING DATA***********/
    parse_reqm(req_message, &saved);
    if (strcmp(saved._method, "GET") != 0)
    { 
        printf("NO GET METHOD\n");
        close(socket_fd);
        return NULL;
    }

    //hit or miss
    Node *searched;
    // mutex lock.............
    pthread_mutex_lock(&mutex);
    searched = search(data_cache, saved._url);
    pthread_mutex_unlock(&mutex);

    if (searched != NULL)
    {
        printf("================\n==HITTTTTTTT!!==\n================\n\n");

        debug("====================searched->object====================\n%s\n====================================end====================\n\n", searched->object);

        reslen = searched->object_size;
        char* temp_m = searched->object;

        // read respons message
        int remain_send = searched->object_size;
        char* ptr_send = searched->object;
        while (0 < remain_send)
        {
            int size_send = send(socket_fd, ptr_send, remain_send, 0);
            if (size_send == -1) break;

            remain_send -= size_send;
            ptr_send += size_send;
        }
    } 
    else
    {
        printf("================\n==MISSSSSSSS!!==\n================\n\n");

        char cp_request_message[reqlen+1];
        bzero((char*)cp_request_message, reqlen+1);
        strcpy(cp_request_message, req_message);
        
        char* line_parse = strtok(cp_request_message, "\r\n");
        line_parse = strtok(NULL, "\r\n");

        // rebuild request message
        bzero((char*)req_message, MAX_BUF_SIZE);
        sprintf(req_message, "%s %s %s\r\nHost: %s\r\n", saved._method, saved._url_object, saved._vhttp, saved._host);
        while( (line_parse = strtok(NULL,"\n")) != NULL )
        {
            if (strcmp(line_parse, "Proxy-Connection: keep-alive\r") == 0 || strcmp(line_parse, "Connection: keep-alive\r")== 0)
                strcat(req_message, "Connection: close\r");
            else strcat(req_message, line_parse);
            strcat(req_message, "\n");
        }
        debug("====================2st request message====================\n%s\n====================================end====================\n\n", req_message);

        // re-setting socket option
        int end_server_portno = atoi(saved._port);
        struct hostent *server = gethostbyname(saved._host); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }

        // end-server ip print
        // for ( int ndx = 0; NULL != server->h_addr_list[ndx]; ndx++)
        //     printf( "%s\n", inet_ntoa( *(struct in_addr*) server->h_addr_list[ndx]));

        int end_server_socket = socket(PF_INET, SOCK_STREAM, 0); //create a new socket
        bzero(&end_addr, sizeof(struct sockaddr_in));
        end_addr.sin_family = AF_INET; //initialize server's address
        end_addr.sin_port = htons(end_server_portno);

        
        // end_addr.sin_addr.s_addr = inet_addr( *(struct in_addr*)server->h_addr_list);
        bcopy((char*) server->h_addr_list[0], (char*) &end_addr.sin_addr.s_addr, server->h_length);

        // connect
        if (connect(end_server_socket, (struct sockaddr *) &end_addr, sizeof(end_addr)) < 0) //establish a connection to the server
            error("ERROR connecting");
        printf("server got connection from end server : %s\n", inet_ntoa(end_addr.sin_addr));

        // write req_message to server
        int size_send = send(end_server_socket, req_message, strlen(req_message)+1, 0);
        if (size_send < 0) error("ERROR writing to socket");

        // read respons message
        char receive_buffer[MAX_BUF_SIZE];
        char *response_message_ptr = respon_message;
        int over_object_size_flag = 1;

        while(1)
        {
            int receive_size = recv(end_server_socket, receive_buffer, MAX_BUF_SIZE, 0);
            if (receive_size <= 0) break;
            reslen += receive_size;
            send(socket_fd, receive_buffer, receive_size, 0);
            if (reslen > MAX_OBJECT_SIZE && over_object_size_flag)
            {
                bzero((char*) respon_message, MAX_OBJECT_SIZE);
                over_object_size_flag = 0;
            }
            else
            {
                memcpy(response_message_ptr, receive_buffer, receive_size);
                response_message_ptr += receive_size;
            }
        }

        close(end_server_socket);
        debug("==================== respon_message ====================\n%s\n====================================end====================\n\n", respon_message);
        if ((strncmp(respon_message, "HTTP/1.1 200 OK", 13)==0) && reslen < MAX_OBJECT_SIZE)
        {
            pthread_mutex_lock(&lmutex);
            add(data_cache, node_init(saved._url, respon_message, reslen+1));
            pthread_mutex_unlock(&lmutex);
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
    strcat(logs, saved._url);
    strcat(logs, " ");
    char mlen[10];
    sprintf(mlen, "%d", reslen);
    strcat(logs, mlen);
    strcat(logs, "\n");

    pthread_mutex_lock(&rmutex);
    int fd;
    if ((fd = open("./proxy.log", O_CREAT|O_WRONLY|O_APPEND, 0644))<0) error("logsfile error");
    write(fd, logs, strlen(logs));
    close(fd);
    pthread_mutex_unlock(&rmutex);

    free(logs);

    close(socket_fd);
    free_request_msg(&saved);
    return NULL;
}

