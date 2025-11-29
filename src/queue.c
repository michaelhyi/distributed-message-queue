#include "queue.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Creates a new queue node with the given data.
 * 
 * @param data data to place in node
 * @returns pointer to the queue node. `NULL` if error
 */
struct queue_node *create_node(char *data) {
    if (!data) {
        return NULL;
    }

    struct queue_node *node = malloc(sizeof(struct queue_node));
    if (node == NULL) {
        return NULL;
    }

    // TODO: what if data contains nul term due to encrpytion
    unsigned int data_size = strnlen(data, INT_MAX);
    node->data = malloc(data_size);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }
    strlcpy(node->data, data, data_size);

    time((long *) &node->timestamp);
    node->next = NULL;
    return node;
}

int queue_init(struct queue *queue) {
    if (queue == NULL) {
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;

    return 0;
}

int queue_push(struct queue *queue, char *data) {
    if (queue == NULL || data == NULL) {
        return -1;
    }

    struct queue_node *node = create_node(data);
    
    if (!queue->head) {
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

char *queue_pop(struct queue *queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    struct queue_node *node = queue->head;

    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }

    char *data = node->data;
    free(node);
    return data;
}
