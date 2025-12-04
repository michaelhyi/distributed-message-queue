#ifndef PARTITION_H
#define PARTITION_H

#include <pthread.h>

#include "queue.h"

extern struct queue queue;
extern pthread_mutex_t queue_lock;

/**
 * Initializes a partition.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 */
int partition_init(void);

/**
 * Destroys a partition.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 */
int partition_destroy(void);

#endif
