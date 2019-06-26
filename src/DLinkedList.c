#include <stdio.h>
#include "DLinkedList.h"

/*
 * return dl's first Node
 */
LinkedList* linkedList_init()
{
    LinkedList* new_likedlist_ptr = (LinkedList*) malloc(sizeof(LinkedList));
    new_likedlist_ptr->remainder_size = MAX_CACHE_SIZE;
    new_likedlist_ptr->header = NULL;

    return new_likedlist_ptr;
}

/**
 * params
 * url : 
 * object :
 * size :
 * return node pointer
 */
Node* node_init(char* url, char* object, int size)
{
    if (size > MAX_OBJECT_SIZE) return NULL;

    Node* new_node_ptr = (Node*) malloc(sizeof(Node));
    bzero(new_node_ptr->url, MAX_BUF_SIZE);
    strcpy(new_node_ptr->url, url);
    bzero(new_node_ptr->object, MAX_OBJECT_SIZE);
    memcpy(new_node_ptr->object, object, size);
    new_node_ptr->object_size= size;
    time_t current_time;
    time(&current_time);
    new_node_ptr->timestemp=current_time;
    new_node_ptr->next = NULL;

    return new_node_ptr;
}

/**
 * params
 * single_linkedlist : linked list pointer
 * return deleted node pointer
 */
Node* delete(LinkedList *single_linkedlist)
{
    // write your code..
    if (single_linkedlist->header != NULL)
    {
        Node* tmp = single_linkedlist->header;
        single_linkedlist->remainder_size += tmp->object_size;
        single_linkedlist->header = tmp->next;

        return tmp;
    }
    else return NULL; 
}

/**
 * params
 * single_linkedlist : linked list pointer
 * node : a node pointer to add in single_linkedlist
 */
void add(LinkedList *single_linkedlist, Node *node)
{
    if (node == NULL) return;

    while (single_linkedlist->remainder_size < node->object_size) delete(single_linkedlist);

    if (single_linkedlist->header!=NULL)
    {
        Node* tmp = single_linkedlist->header;
        while((tmp->next)!=NULL) tmp = tmp->next;
        tmp->next = node;
        single_linkedlist->remainder_size -= node->object_size;
    }
    else single_linkedlist->header = node;
}

/**
 * params
 * single_linkedlist : linked list pointer
 * url : 
 */
Node* search(LinkedList *single_linkedlist, char* url)
{
    Node* cur_node = single_linkedlist->header, *prev_node;

    if (cur_node == NULL) return NULL;
    while (strcmp(cur_node->url, url) != 0 && cur_node != NULL)
    {
        prev_node = cur_node;
        cur_node = cur_node->next;
    }

    if (strcmp(cur_node->url, url) == 0) return cur_node;
    Node* tmp = single_linkedlist->header->next;
    if (tmp == NULL) return NULL;
    while(strcmp(tmp->url, url)!=0)
    {
        cur_node = cur_node->next;
        tmp = tmp->next;
        if (tmp == NULL) return NULL;
    }
    cur_node->next = tmp->next;
    tmp->next = NULL;
    single_linkedlist->remainder_size += tmp->object_size;
    add(single_linkedlist, tmp);

    return tmp;
}