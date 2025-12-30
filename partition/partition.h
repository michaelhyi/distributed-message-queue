#ifndef PARTITION_H
#define PARTITION_H

#include <pthread.h>

#include "queue.h"

extern struct queue queue;
extern pthread_mutex_t queue_lock;

/**
 * Initializes a partition by creating a queue and starting a DMQP server. The
 * server will listen on a random port assigned by the OS.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EIO` unexpected error
 */
int partition_init(void);

/**
 * Destroys a partition, cleaning up all its resources, its queue, and its DMQP
 * server.
 */
void partition_destroy(void);

#endif
