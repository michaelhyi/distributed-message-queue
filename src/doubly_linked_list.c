#include "doubly_linked_list.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int doubly_linked_list_init(struct doubly_linked_list *list) {
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }

    list->head = NULL;
    list->tail = NULL;
    return 0;
}

int doubly_linked_list_add(struct doubly_linked_list *list, void *data, unsigned int data_size) {
    if (list == NULL || data == NULL || data_size == 0) {
        errno = EINVAL;
        return -1;
    }

    struct doubly_linked_list_node *node = malloc(sizeof(struct doubly_linked_list_node));
    if (node == NULL) {
        errno = ENOMEM;
        return -1;
    }

    node->data = malloc(data_size);
    if (node->data == NULL) {
        free(node);
        errno = ENOMEM;
        return -1;
    }
    memcpy(node->data, data, data_size);

    node->prev = NULL;
    node->next = NULL;

    if (list->head == NULL) { // empty list
        list->head = node;
        list->tail = node;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }

    return 0;
}

int doubly_linked_list_remove(struct doubly_linked_list *list, struct doubly_linked_list_node *node) {
    if (list == NULL || node == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (list->head == NULL) { // empty list
        errno = EINVAL;
        return -1;
    } else if (list->head == list->tail) { // list size = 1
        list->head = NULL;
        list->tail = NULL;
    } else if (node == list->head) {
        list->head = list->head->next;
    } else if (node == list->tail) {
        list->tail = list->tail->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    free(node->data);
    free(node);
    return 0;
}

int doubly_linked_list_destroy(struct doubly_linked_list *list) {
    if (list == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct doubly_linked_list_node *curr = list->head;

    while (curr != NULL) {
        struct doubly_linked_list_node *temp = curr->next;
        free(curr->data);
        free(curr);
        curr = temp;
    }

    list->head = NULL;
    list->tail = NULL;
    return 0;
}
