#ifndef QUEUE_H
#define QUEUE_H

struct queue {
    struct queue_node *head;
    struct queue_node *tail;
};

struct queue_node {
    void *data;
    struct queue_node *next;
};

/**
 * Initializes a queue.
 * 
 * @param queue the queue to init
 * @returns -1 if error
 */
int queue_init(struct queue *queue);

/**
 * Pushes data on a queue.
 * 
 * @param queue the queue to update
 * @param data data to push on the queue
 * @param data_size size of the data
 * @returns 0 if success, -1 if error
 */
int queue_push(struct queue *queue, void *data, unsigned int data_size);

/**
 * Pops data off a queue.
 * 
 * @param queue the queue to update
 * @returns the node popped from the queue, `NULL` if error or empty queue
 */
struct queue_node *queue_pop(struct queue *queue);

/**
 * Gets the head of a queue.
 * 
 * @param queue the queue to peek
 * @returns the head node of the queue, `NULL` if error or empty queue
 */
struct queue_node *queue_peek(struct queue *queue);

// TODO: errno

/**
 * Destroys a queue. No threads should hold the queue's lock.
 * 
 * @param queue the queue to destroy
 * @returns 0 if success, -1 if error
 */
int queue_destroy(struct queue *queue);

#endif
