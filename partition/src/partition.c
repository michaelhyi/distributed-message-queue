#include "partition.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "dmqp.h"
#include "network.h"

struct queue queue;
pthread_mutex_t queue_lock;

int partition_init(unsigned int server_port) {
    int res = queue_init(&queue);
    if (res < 0) {
        return -1;
    }

    res = pthread_mutex_init(&queue_lock, NULL);
    if (res != 0) {
        errno = res;
        return -1;
    }

    res = server_init(server_port, handle_server_message);
    if (res < 0) {
        return 1;
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

int handle_dmqp_message(struct dmqp_message message, int conn_socket) {
    if ((message.header.length > 0 && message.payload == NULL) ||
        message.header.length > MAX_PAYLOAD_LENGTH || conn_socket) {
        errno = EINVAL;
        return -1;
    }

    int res;
    struct queue_entry entry;
    struct dmqp_header res_header;

    switch (message.header.method) {
    case RESPONSE:
        // TODO
        return -1;
    case HEARTBEAT:
        // TODO
        break;
    case PUSH:
        res = pthread_mutex_lock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = queue_push(&queue, message.payload, message.header.length);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = pthread_mutex_unlock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        // TODO: handle encryption
        res_header.method = RESPONSE;
        res_header.flags = 0;
        res_header.length = 0;

        res = send(conn_socket, &res_header, sizeof(struct dmqp_header), 0);
        if (res < 0) {
            return -1;
        }

        break;
    case POP:
        res = pthread_mutex_lock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = queue_pop(&queue, &entry);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = pthread_mutex_unlock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        // TODO: handle encrpytion
        res_header.method = RESPONSE;
        res_header.flags = 0;
        res_header.length = entry.size;
        res_header.queue_entry_timestamp = entry.timestamp;

        res = send(conn_socket, &res_header, sizeof(struct dmqp_header), 0);
        if (res < 0) {
            return -1;
        }

        res = send(conn_socket, &entry.data, res_header.length, 0);
        if (res < 0) {
            return -1;
        }

        free(entry.data);
        break;
    case PEEK:
        res = pthread_mutex_lock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = queue_peek(&queue, &entry);
        if (res < 0) {
            // TODO
            return -1;
        }

        res = pthread_mutex_unlock(&queue_lock);
        if (res < 0) {
            // TODO
            return -1;
        }

        // TODO: handle encrpytion
        struct dmqp_header response_header = {.method = RESPONSE,
                                              .flags = 0,
                                              .length = entry.size,
                                              .queue_entry_timestamp =
                                                  entry.timestamp};
        res =
            send(conn_socket, &response_header, sizeof(struct dmqp_header), 0);
        if (res < 0) {
            return -1;
        }

        res = send(conn_socket, &entry.data, response_header.length, 0);
        if (res < 0) {
            return -1;
        }

        break;
    default:
        // TODO
        return -1;
    }

    return 0;
}
