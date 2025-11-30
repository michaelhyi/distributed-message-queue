#include "queue.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Creates a new queue node with the given data. Must be holding the queue's
 * lock.
 * 
 * @param data data to place in node
 * @returns pointer to the queue node. `NULL` if error
 */
static struct queue_node *create_node(char *data, unsigned int data_size) {
    if (!data || data_size == 0) {
        return NULL;
    }

    struct queue_node *node = malloc(sizeof(struct queue_node));
    if (node == NULL) {
        return NULL;
    }

    node->data = malloc(data_size);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }
    memcpy(node->data, data, data_size);

    time((time_t *) &node->timestamp);
    node->next = NULL;
    return node;
}

int queue_init(struct queue *queue) {
    if (queue == NULL) {
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;

    pthread_mutex_init(&queue->lock, NULL);
    return 0;
}

int queue_push(struct queue *queue, char *data, unsigned int data_size) {
    if (queue == NULL || data == NULL || data_size == 0) {
        return -1;
    }

    pthread_mutex_lock(&queue->lock);
    struct queue_node *node = create_node(data, data_size);

    if (node == NULL) {
        pthread_mutex_unlock(&queue->lock);
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

    pthread_mutex_unlock(&queue->lock);
    return 0;
}

char *queue_pop(struct queue *queue) {
    if (queue == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&queue->lock);
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->lock);
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
    pthread_mutex_unlock(&queue->lock);
    return data;
}

void queue_destroy(struct queue *queue) {
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->lock);
    struct queue_node *curr = queue->head; 

    while (curr != NULL) {
        struct queue_node *temp = curr->next;
        free(curr->data);
        free(curr);
        curr = temp;
    }

    pthread_mutex_unlock(&queue->lock);
    pthread_mutex_destroy(&queue->lock);
}
