#ifndef NODE_H
#define NODE_H

#include <time.h>

#define MAX_BUF_SIZE 1024 // 1kb
#define MAX_OBJECT_SIZE 524288 //512kb

typedef struct Node {
    time_t timestemp;
    char url[MAX_BUF_SIZE]; // identifier
    char object[MAX_OBJECT_SIZE]; // data
    int object_size;
    struct Node *prev;
    struct Node *next;
} Node;

Node* node_init(char* , char* , int);

#endif // NODE_H
