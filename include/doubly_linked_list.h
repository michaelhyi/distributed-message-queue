#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

struct doubly_linked_list {
    struct doubly_linked_list_node *head;
    struct doubly_linked_list_node *tail;
};

struct doubly_linked_list_node {
    void *data;
    struct doubly_linked_list_node *prev;
    struct doubly_linked_list_node *next;
};

/**
 * Initializes a doubly linked list.
 * 
 * @param list list to initialize
 * @returns 0 if success, -1 if error with global errno set
 */
int doubly_linked_list_init(struct doubly_linked_list *list);

/**
 * Adds data to a doubly linked list.
 * 
 * @param list list to update
 * @param data data to add
 * @param data_size size of data
 * @returns 0 if success, -1 if error with global errno set
 */
int doubly_linked_list_add(struct doubly_linked_list *list, void *data, unsigned int data_size);

/**
 * Removes a node from a doubly linked list. Behavior is undefined if `node`
 * does not exist in `list`.
 * 
 * @param list list to update
 * @param node node to remove
 * @returns 0 if success, -1 if error with global errno set
 */
int doubly_linked_list_remove(struct doubly_linked_list *list, struct doubly_linked_list_node *node);

/**
 * Destroys a doubly linked list.
 * 
 * @param list list to destroy 
 * @returns 0 if success, -1 if error with global errno set
 */
int doubly_linked_list_destroy(struct doubly_linked_list *list);

#endif
