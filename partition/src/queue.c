#include "queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Creates a new queue node with the given data. Must be holding the queue's
 * lock.
 *
 * @param data data to place in node
 * @returns pointer to the queue node, `NULL` if error with global `errno` set
 */
static struct queue_node *create_node(void *data, unsigned int data_size) {
    if (!data || data_size == 0) {
        errno = EINVAL;
        return NULL;
    }

    struct queue_node *node = malloc(sizeof(struct queue_node));
    if (node == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    node->entry.data = malloc(data_size);
    if (node->entry.data == NULL) {
        free(node);
        errno = ENOMEM;
        return NULL;
    }
    memcpy(node->entry.data, data, data_size);
    node->entry.size = data_size;
    time(&node->entry.timestamp);

    node->next = NULL;
    return node;
}

int queue_init(struct queue *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;
    return 0;
}

int queue_push(struct queue *queue, void *data, unsigned int data_size) {
    if (queue == NULL || data == NULL || data_size == 0) {
        errno = EINVAL;
        return -1;
    }

    struct queue_node *node = create_node(data, data_size);
    if (node == NULL) {
        return -1;
    }

    if (queue->head == NULL) {
        queue->head = node;
        queue->tail = node;
    } else if (queue->head == queue->tail) {
        queue->head->next = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    return 0;
}

int queue_pop(struct queue *queue, struct queue_entry *out_entry) {
    if (queue == NULL || out_entry == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (queue->head == NULL) {
        errno = ENODATA;
        return -1;
    }

    struct queue_node *node = queue->head;

    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }

    *out_entry = node->entry;
    free(node);
    return 0;
}

int queue_peek(struct queue *queue, struct queue_entry *out_entry) {
    if (queue == NULL || out_entry == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (queue->head == NULL) {
        errno = ENODATA;
        return -1;
    }

    *out_entry = queue->head->entry;
    return 0;
}

int queue_destroy(struct queue *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct queue_node *curr = queue->head;

    while (curr != NULL) {
        struct queue_node *temp = curr->next;
        free(curr->entry.data);
        free(curr);
        curr = temp;
    }

    queue->head = NULL;
    queue->tail = NULL;
    return 0;
}
