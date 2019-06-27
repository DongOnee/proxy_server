/* 
   A simple server in the internet domain using TCP
   Usage:./proxy port (E.g. ./proxy 10000 )
*/

#include "proxy.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void sigint_handler(int signo)
{
    bzero((char *) &proxy_addr, sizeof(struct sockaddr_in));
    close(sockfd);
    exit(1);
}

void closeall()
{
    printf("\nbye~\n");
    close(sockfd);
}

int main(int argc, char *argv[]) 
{
    // error detect
    if (argc < 2) error("ERROR, no port provided\n");

    // SIGINT 를 이용해서 종료하였을때 socket 을 닫아주기 위해
    act_new.sa_handler = sigint_handler;
    sigemptyset(&act_new.sa_mask);
    sigaction(SIGINT, &act_new, &act_old);

    // cacheing memory init
    g_heap = linkedList_init();

    /**
     * Create a new socket
     * AF_INET: Address Domain is Internet 
     * SOCK_STREAM: Socket Type is STREAM Socket
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* SET PROXY ADDRESS */
    bzero((char *) &proxy_addr, sizeof(struct sockaddr_in));
    portno = atoi(argv[1]); //atoi converts from String to Integer
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
    proxy_addr.sin_port = htons(portno); //convert from host to network byte order
 
    //Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0)  error("ERROR on binding");
 
    // Listen for socket connections. Backlog queue (connections to wait) is 5
    if (listen(sockfd, 10) == -1) error("ERROR on listen");

    // for multi-thread
    pthread_t p_thread[100];

    int i =0;
    socklen_t clilen = sizeof(cli_addr);
    while (1)
    {
        /**
         * accept function
         * 1) Block until a new connection is established
         * 2) the new socket descriptor will be used for subsequent communication with the newly connected client.
        */
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        if(pthread_create(&p_thread[i], NULL, (void*) t_function, (void*)&newsockfd) <0 ) perror("thread create error");
        pthread_join(p_thread[i], NULL);
        i = (i+1) % 100;
    }

    close(sockfd);
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


void assign_request_msg(struct request_msg* saved)
{
    saved->_method = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_url = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_url_object = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_port = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_vhttp = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    saved->_host = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);

    bzero((char*)saved->_method, MAX_BUF_SIZE);
    bzero((char*)saved->_url, MAX_BUF_SIZE);
    bzero((char*)saved->_url_object, MAX_BUF_SIZE);
    bzero((char*)saved->_port, MAX_BUF_SIZE);
    bzero((char*)saved->_vhttp, MAX_BUF_SIZE);
    bzero((char*)saved->_host, MAX_BUF_SIZE);
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


void t_function(void* newsockfd)
{
    /*
    char* requestname = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* mathod = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* URL = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* vhttp = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* host = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char* port = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    bzero((char*)requestname, MAX_BUF_SIZE);
    bzero((char*)mathod, MAX_BUF_SIZE);
    bzero((char*)URL, MAX_BUF_SIZE);
    bzero((char*)vhttp, MAX_BUF_SIZE);
    bzero((char*)host, MAX_BUF_SIZE);
    bzero((char*)port, MAX_BUF_SIZE);
 */
    struct request_msg saved;
    assign_request_msg(&saved);
    
    char *req_message = (char*) malloc(sizeof(char)*MAX_BUF_SIZE);
    char *respon_message = (char*) malloc(sizeof(char)*MAX_OBJECT_SIZE);
    bzero((char*)req_message, MAX_BUF_SIZE);
    bzero((char*)respon_message, MAX_OBJECT_SIZE);
    int reqlen=0;
    int reslen=0;

    // check sever connection
    printf("server got connection from client : %s\n", inet_ntoa(cli_addr.sin_addr));

    // read section~~~~ we can see http reqeust message using buffer.
    reqlen = read(*(int*)newsockfd, req_message, MAX_BUF_SIZE); //Read is a block function. It will read at most 1024 bytes
    if (reqlen < 0) error("ERROR reading from socket"); // error
    else if (reqlen==0) // 가끔 버퍼에 아무것도 채워지지 않는 경우가 발생한다. 이 문제가 발생하여도 서버가 다운되지 않도록 조치 하였다.
    {
        printf("error~~\nreqlen=0\nrestart!!\n\n\n");
        close(*(int*)newsockfd);
        return;
    }
    printf("====================1st request message====================\n%s\n====================================end====================\n\n", req_message);

    /***********PARSING DATA***********/
    parse_reqm(req_message, &saved);
    if (strcmp(saved._method, "GET") != 0)
    { 
        printf("NO GET...\n");
        close(*(int*)newsockfd);
        return;
    }

    //hit or miss
    Node *searched;
    // mutex lock.............
    pthread_mutex_lock(&mutex);
    searched = search(g_heap, saved._url);
    pthread_mutex_unlock(&mutex);
    if (searched != NULL)
    {
        printf("================\n");
        printf("==HITTTTTTTT!!==\n");
        printf("================\n\n");
        printf("====================searched->object====================\n%s\n====================================end====================\n\n", searched->object);

        reslen = searched->object_size;
        char* temp_m = searched->object;

        // read respons message
        int remain_send = searched->object_size;
        char* ptr_send = searched->object;
        while (0 < remain_send)
        {
            int size_send = send(*(int*) newsockfd, ptr_send, remain_send, 0);
            if (size_send == -1) break;

            remain_send -= size_send;
            ptr_send += size_send;
        }
    } 
    else
    {
        printf("================\n");
        printf("==MISSSSSSSS!!==\n");
        printf("================\n\n");

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
        printf("====================2st request message====================\n%s\n====================================end====================\n\n", req_message);

        // re-setting socket option
        int end_server_portno = atoi(saved._port);
        struct hostent *server; //conaatains tons of information, including the server's IP address
        server = gethostbyname(saved._host); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }

        int ens_server_socket = socket(AF_INET, SOCK_STREAM, 0); //create a new socket
        bzero((char*) &end_addr, sizeof(struct sockaddr_in));
        end_addr.sin_family = AF_INET; //initialize server's address
        bcopy((char*)server->h_addr_list, (char*)&end_addr.sin_addr.s_addr, server->h_length);
        end_addr.sin_port = htons(end_server_portno);

        // connect
        if (connect(ens_server_socket,(struct sockaddr *)&end_addr,sizeof(end_addr)) < 0) //establish a connection to the server
            error("ERROR connecting");
        printf("server got connection from end server : %s\n", inet_ntoa(end_addr.sin_addr));

        // write req_message to server
        int size_send = send(ens_server_socket, req_message, strlen(req_message)+1, 0);
        if (size_send < 0) error("ERROR writing to socket");

        // read respons message
        char receive_buffer[MAX_BUF_SIZE];
        char *response_message_ptr = respon_message;
        int over_object_size_flag = 1;
        while(1)
        {
            int receive_size = recv(ens_server_socket, receive_buffer, MAX_BUF_SIZE, 0);
            if (receive_size == -1) break;
            reslen += receive_size;
            send(*(int*) newsockfd, receive_buffer, receive_size, 0);
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

        /**********read respon_message**********/
        printf("==================== respon_message ====================\n%s\n====================================end====================\n\n", respon_message);
        if ((strncmp(respon_message, "HTTP/1.1 200 OK", 13)==0) && reslen < MAX_OBJECT_SIZE)
        {
            pthread_mutex_lock(&mutex);
            add(g_heap, node_init(saved._url, respon_message, reslen+1));
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
    strcat(logs, saved._url);
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

    free_request_msg(&saved);
    free(req_message);
    free(respon_message);
}

