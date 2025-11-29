#ifndef QUEUE_H
#define QUEUE_H

struct queue {
    struct queue_node *head;
    struct queue_node *tail;
};

struct queue_node {
    char *data;
    unsigned long long timestamp;

    struct queue_node *next;
};

/**
 * Initializes the queue.
 * 
 * @returns -1 if error
 */
int queue_init(struct queue *queue);

/**
 * Pushes data on the queue.
 * 
 * @param queue the queue to update
 * @param data data to push on the queue
 * @param data_size size of the data
 * @returns 0 if success, -1 if error
 */
int queue_push(struct queue *queue, char *data, unsigned int data_size);

/**
 * Pops data off the queue.
 * 
 * @param queue the queue to update
 * @returns the data popped from the queue. `NULL` if error or empty queue
 */
char *queue_pop(struct queue *queue);

#endif
