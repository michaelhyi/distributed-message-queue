#include "partition.h"

#include <errno.h>
#include <messageq/network.h>
#include <stdlib.h>
#include <sys/socket.h>

struct queue queue;
pthread_mutex_t queue_lock;

int partition_init() {
    if (queue_init(&queue) < 0) {
        errno = EIO;
        return -1;
    }

    pthread_mutex_init(&queue_lock, NULL);

    if (dmqp_server_init(0) < 0) {
        partition_destroy();
        errno = EIO;
        return -1;
    }

    return 0;
}

void partition_destroy() {
    pthread_mutex_destroy(&queue_lock);
    queue_destroy(&queue);
}

// TODO: test and fix all these
void handle_dmqp_push(const struct dmqp_message *message, int client) {
    struct dmqp_header res_header;

    pthread_mutex_lock(&queue_lock);
    struct queue_entry entry = {.data = message->payload,
                                .size = message->header.length,
                                .timestamp = message->header.sequence_id};

    if (queue_push(&queue, &entry) < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }
    }

    pthread_mutex_unlock(&queue_lock);

    res_header.method = DMQP_RESPONSE;
    res_header.status_code = errno;
    res_header.length = 0;
    res_header.sequence_id = 0;
    struct dmqp_message res_message = {.header = res_header};

    send_dmqp_message(client, &res_message, 0);
}

void handle_dmqp_pop(const struct dmqp_message *message, int client) {
    (void)message;
    struct queue_entry entry = {.data = NULL, .size = 0, .timestamp = 0};
    struct dmqp_header res_header;

    pthread_mutex_lock(&queue_lock);

    if (queue_pop(&queue, &entry) < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }
    }

    pthread_mutex_unlock(&queue_lock);

    res_header.method = DMQP_RESPONSE;
    res_header.status_code = errno;
    res_header.length = entry.size;
    res_header.sequence_id = entry.timestamp;
    struct dmqp_message res_message = {.header = res_header,
                                       .payload = entry.data};

    send_dmqp_message(client, &res_message, 0);

    if (entry.data != NULL) {
        free(entry.data);
    }
}

void handle_dmqp_peek_sequence_id(const struct dmqp_message *message,
                                  int client) {
    (void)message;
    struct dmqp_header res_header;

    pthread_mutex_lock(&queue_lock);

    long timestamp;
    if (queue_peek_timestamp(&queue, &timestamp) < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }
    }

    pthread_mutex_unlock(&queue_lock);

    res_header.method = DMQP_RESPONSE;
    res_header.status_code = errno;
    res_header.length = 0;
    res_header.sequence_id = timestamp;
    struct dmqp_message res_message = {.header = res_header, .payload = NULL};

    send_dmqp_message(client, &res_message, 0);
}

void handle_dmqp_response(const struct dmqp_message *message, int client) {
    (void)message;
    struct dmqp_header res_header = {.method = DMQP_RESPONSE,
                                     .status_code = EPROTO,
                                     .length = 0,
                                     .sequence_id = 0};
    struct dmqp_message res_message = {.header = res_header};

    send_dmqp_message(client, &res_message, 0);
    errno = EPROTO;
}
