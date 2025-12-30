#include "queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void queue_init(struct queue *queue) {
    if (!queue) {
        return;
    }

    queue->head = NULL;
    queue->tail = NULL;
}

void queue_destroy(struct queue *queue) {
    if (!queue) {
        return;
    }

    struct queue_node *curr = queue->head;
    while (curr) {
        struct queue_node *temp = curr->next;
        free(curr->entry.data);
        free(curr);
        curr = temp;
    }

    queue->head = NULL;
    queue->tail = NULL;
}

/**
 * Creates a new queue node with the given entry. Deep copies the entry.
 *
 * @param entry entry to copy in node
 * @returns pointer to the queue node, `NULL` if error with global `errno` set
 * @throws `ENOMEM` malloc failure
 */
static struct queue_node *create_node(const struct queue_entry *entry) {
    struct queue_node *node = malloc(sizeof(struct queue_node));
    if (!node) {
        errno = ENOMEM;
        return NULL;
    }

    node->entry.data = malloc(entry->size);
    if (!node->entry.data) {
        free(node);
        errno = ENOMEM;
        return NULL;
    }
    memcpy(node->entry.data, entry->data, entry->size);
    node->entry.size = entry->size;
    node->entry.id = entry->id;
    node->next = NULL;

    return node;
}

int queue_push(struct queue *queue, const struct queue_entry *entry) {
    if (!queue || !entry || !entry->data || !entry->size) {
        errno = EINVAL;
        return -1;
    }

    struct queue_node *node = create_node(entry);
    if (!node) {
        return -1;
    }

    if (!queue->head) { // empty
        queue->head = node;
        queue->tail = node;
    } else if (queue->head == queue->tail) { // queue has 1 element
        queue->head->next = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    return 0;
}

struct queue_entry *queue_pop(struct queue *queue) {
    if (!queue) {
        errno = EINVAL;
        return NULL;
    }

    if (!queue->head) { // empty
        errno = ENODATA;
        return NULL;
    }

    struct queue_node *node = queue->head;

    if (queue->head == queue->tail) { // queue has 1 element
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = queue->head->next;
    }

    return &node->entry;
}

int queue_peek_id(struct queue *queue) {
    if (!queue) {
        errno = EINVAL;
        return -1;
    }

    if (!queue->head) {
        errno = ENODATA;
        return -1;
    }

    return queue->head->entry.id;
}
