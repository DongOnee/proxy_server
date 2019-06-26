#include <time.h>

#define MAX_BUF_SIZE 1024 // 1kb
#define MAX_OBJECT_SIZE 524288 //512kb
#define MAX_CACHE_SIZE 5242880 //5mb

typedef struct Node {
    time_t timestemp;
    char url[MAX_BUF_SIZE]; // identifier
    char object[MAX_OBJECT_SIZE]; // data
    int object_size;
    struct Node *next;
} Node;

typedef struct LinkedList {
    int remainder_size;
    struct Node *header;
} LinkedList;

LinkedList* linkedList_init();
Node* node_init(char* , char* , int);
Node* delete(LinkedList*);
void add(LinkedList*, Node*);
Node* search(LinkedList*, char*);
