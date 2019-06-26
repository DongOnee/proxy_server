#if !defined(DLINKEDLIST_H)
#define DLINKEDLIST_H

#include <stdio.h>
#include "Node.h"

#define MAX_CACHE_SIZE 5242880 //5mb

typedef struct DLinkedList {
    int remainder_size;
    struct Node *header;
    struct Node *tail;
} DLinkedList;

DLinkedList* linkedList_init();
Node* delete(DLinkedList*);
void add(DLinkedList*, Node*);
Node* search(DLinkedList*, char*);

#endif // DLINKEDLIST_H
