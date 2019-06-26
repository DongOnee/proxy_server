#include "DLinkedList.h"

/*
 * return dl's first Node
 */
DLinkedList* linkedList_init()
{
    DLinkedList* dlinkedlist_ptr = (DLinkedList*) malloc(sizeof(DLinkedList));
    dlinkedlist_ptr->remainder_size = MAX_CACHE_SIZE;
    dlinkedlist_ptr->header = NULL;
    dlinkedlist_ptr->tail = NULL;

    return dlinkedlist_ptr;
}

/**
 * params
 * dlinkedlist_ptr : linked list pointer
 * return deleted node pointer (header node)
 */
Node* delete(DLinkedList *dlinkedlist_ptr)
{
    // write your code..
    Node *ret_ptr = dlinkedlist_ptr->header;
    if (ret_ptr == NULL) return NULL;
    else if (ret_ptr == dlinkedlist_ptr->tail)
    {
        dlinkedlist_ptr->header = NULL;
        dlinkedlist_ptr->tail = NULL;
    }
    else
    {
        ret_ptr->next->prev = NULL;
        dlinkedlist_ptr->header = ret_ptr->next;
    }
    dlinkedlist_ptr->remainder_size += ret_ptr->object_size;

    return ret_ptr;
}

/**
 * params
 * dlinkedlist_ptr : linked list pointer
 * node : a node pointer to add in dlinkedlist_ptr, size of node < MAX_OBJECT_SIZE
 */
void add(DLinkedList *dlinkedlist_ptr, Node *node)
{
    if (node == NULL) return;

    while (dlinkedlist_ptr->remainder_size < node->object_size) delete(dlinkedlist_ptr);

    if (dlinkedlist_ptr->tail == NULL) dlinkedlist_ptr->header = node;
    else
    {
        node->prev = dlinkedlist_ptr->tail;
        dlinkedlist_ptr->tail->next = node;
    }
    dlinkedlist_ptr->tail = node;
    dlinkedlist_ptr->remainder_size -= node->object_size;
}

/**
 * params
 * dlinkedlist_ptr : linked list pointer
 * url : 
 */
Node* search(DLinkedList *dlinkedlist_ptr, char* url)
{
    Node* cur_node = dlinkedlist_ptr->header;
    while (strcmp(cur_node->url, url) != 0 && cur_node != NULL) cur_node = cur_node->next;

    if (cur_node == NULL) return NULL;
    else if (strcmp(cur_node->url, url) == 0)
    {
        cur_node->prev->next = cur_node->next;
        cur_node->next->prev - cur_node->prev;
        dlinkedlist_ptr->tail->next = cur_node;
        cur_node->prev = dlinkedlist_ptr->tail;
        dlinkedlist_ptr->tail = cur_node;
        cur_node->next = NULL;
    }
    return cur_node;
}