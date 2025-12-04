#include "partition.h"

#include <errno.h>

#include "dmqp.h"

struct queue queue;
pthread_mutex_t queue_lock;

int partition_init(void) {
    int res = queue_init(&queue);
    if (res < 0) {
        return -1;
    }

    res = pthread_mutex_init(&queue_lock, NULL);
    if (res != 0) {
        errno = res;
        return -1;
    }

    return 0;
}

int partition_destroy(void) {
    int res = queue_destroy(&queue);
    if (res < 0) {
        return -1;
    }

    res = pthread_mutex_destroy(&queue_lock);
    if (res != 0) {
        errno = res;
        return -1;
    }

    return 0;
}

int handle_dmqp_message(struct dmqp_message message) {
    (void)message; // unused param
    return 0;
}
