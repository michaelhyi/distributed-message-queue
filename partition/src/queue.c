#include "queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Creates a new queue node with the given entry. Deep copies the entry. Must be
 * holding the queue's lock.
 *
 * @param entry entry to copy in node
 * @returns pointer to the queue node, `NULL` if error with global `errno` set
 */
static struct queue_node *create_node(const struct queue_entry *entry) {
    if (entry == NULL || entry->data == NULL || entry->size == 0) {
        errno = EINVAL;
        return NULL;
    }

    struct queue_node *node = malloc(sizeof(struct queue_node));
    if (node == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    node->entry.data = malloc(entry->size);
    if (node->entry.data == NULL) {
        free(node);
        errno = ENOMEM;
        return NULL;
    }
    memcpy(node->entry.data, entry->data, entry->size);
    node->entry.size = entry->size;
    node->entry.timestamp = entry->timestamp;
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

int queue_push(struct queue *queue, const struct queue_entry *entry) {
    if (queue == NULL || entry == NULL || entry->data == NULL ||
        entry->size == 0) {
        errno = EINVAL;
        return -1;
    }

    struct queue_node *node = create_node(entry);
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

int queue_pop(struct queue *queue, struct queue_entry *entry) {
    if (queue == NULL || entry == NULL) {
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

    *entry = node->entry;
    free(node);
    return 0;
}

int queue_peek(struct queue *queue, struct queue_entry *entry) {
    if (queue == NULL || entry == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (queue->head == NULL) {
        errno = ENODATA;
        return -1;
    }

    *entry = queue->head->entry;
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
