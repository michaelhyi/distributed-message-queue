#ifndef QUEUE_H
#define QUEUE_H

struct queue_entry {
    unsigned int id;
    void *data;
    unsigned int size;
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
 */
void queue_init(struct queue *queue);

/**
 * Destroys a queue, freeing all its resources.
 *
 * @param queue the queue to destroy
 */
void queue_destroy(struct queue *queue);

/**
 * Pushes data on a queue.
 *
 * @param queue the queue to update
 * @param entry the entry to push
 * @returns 0 on success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `ENOMEM` out of memory
 */
int queue_push(struct queue *queue, const struct queue_entry *entry);

/**
 * Pops data off a queue.
 *
 * @param queue the queue to update
 * @returns popped queue entry if success, must be freed by caller. `NULL` if
 * error with global `errno` set. must be freed by caller
 * @throws `EINVAL` invalid args
 * @throws `ENODATA` queue empty
 */
struct queue_entry *queue_pop(struct queue *queue);

/**
 * Gets the ID of the head of a queue.
 *
 * @param queue the queue to peek
 * @returns queue's head id if success, -1 if error
 * @throws `EINVAL` invalid args
 * @throws `ENODATA` queue empty
 */
int queue_peek_id(struct queue *queue);

#endif
