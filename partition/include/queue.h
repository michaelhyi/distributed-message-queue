#ifndef QUEUE_H
#define QUEUE_H

// TODO: is it inefficient to peek all data first instead of metadata, during
// mapreduce?

struct queue_entry {
    void *data;
    unsigned int size;
    long timestamp;
};

struct queue_node {
    struct queue_entry entry;
    struct queue_node *next;
};

struct queue {
    struct queue_node *head;
    struct queue_node *tail;
};

/**
 * Initializes a queue.
 *
 * @param queue the queue to init
 * @returns 0 on success, -1 if error with global `errno` set
 */
int queue_init(struct queue *queue);

/**
 * Pushes data on a queue.
 *
 * @param queue the queue to update
 * @param data data to push on the queue
 * @param data_size size of the data
 * @returns 0 on success, -1 if error with global `errno` set
 */
int queue_push(struct queue *queue, void *data, unsigned int data_size);

/**
 * Pops data off a queue.
 *
 * @param queue the queue to update
 * @param entry output param returning the popped queue entry
 * @returns 0 if success, -1 if error with global `errno` set
 */
int queue_pop(struct queue *queue, struct queue_entry *entry);

/**
 * Gets the head of a queue.
 *
 * @param queue the queue to peek
 * @param entry output param returning the head queue entry
 * @returns 0 if success, -1 if error with global `errno` set
 */
int queue_peek(struct queue *queue, struct queue_entry *entry);

/**
 * Destroys a queue. No threads should hold the queue's lock.
 *
 * @param queue the queue to destroy
 * @returns 0 on success, -1 if error with global `errno` set
 */
int queue_destroy(struct queue *queue);

#endif
