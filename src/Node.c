#include "Node.h"

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
