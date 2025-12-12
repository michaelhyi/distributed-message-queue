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

    res = server_init(server_port, handle_dmqp_message);
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

// TODO: test all these
int handle_dmqp_response(int reply_socket) {
    if (reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    struct dmqp_header res_header = {
        .method = DMQP_RESPONSE, .flags = EPROTO, .length = 0, .timestamp = 0};

    int res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        return -1;
    }

    return 0;
}

int handle_dmqp_heartbeat(int reply_socket) {
    if (reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    struct dmqp_header res_header = {
        .method = DMQP_RESPONSE, .flags = 0, .length = 0, .timestamp = 0};

    int res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        return -1;
    }

    return 0;
}

int handle_dmqp_push(struct dmqp_message message, int reply_socket) {
    if ((message.header.length > 0 && message.payload == NULL) ||
        message.header.length > MAX_PAYLOAD_LENGTH || reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;
    struct dmqp_header res_header;

    int res = pthread_mutex_lock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

    res = queue_push(&queue, message.payload, message.header.length);
    if (res < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }

        ret = -1;
        goto release_lock;
    }

release_lock:
    res = pthread_mutex_unlock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

reply:
    // TODO: handle encryption
    res_header.method = DMQP_RESPONSE;
    res_header.flags = errno;
    res_header.length = 0;
    res_header.timestamp = 0;

    res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        return -1;
    }

    return ret;
}

int handle_dmqp_pop(int reply_socket) {
    if (reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;
    struct queue_entry entry = {.data = NULL, .size = 0, .timestamp = 0};
    struct dmqp_header res_header;

    int res = pthread_mutex_lock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

    res = queue_pop(&queue, &entry);
    if (res < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }

        ret = -1;
        goto release_lock;
    }

release_lock:
    res = pthread_mutex_unlock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

reply:
    // TODO: handle encryption
    res_header.method = DMQP_RESPONSE;
    res_header.flags = errno;
    res_header.length = entry.size;
    res_header.timestamp = entry.timestamp;

    res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        ret = -1;
        goto cleanup;
    }

    res = send(reply_socket, entry.data, res_header.length, 0);
    if (res < 0) {
        ret = -1;
        goto cleanup;
    }

cleanup:
    if (entry.data != NULL) {
        free(entry.data);
    }

    return ret;
}

int handle_dmqp_peek(int reply_socket) {
    if (reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;
    struct queue_entry entry = {.data = NULL, .size = 0, .timestamp = 0};
    struct dmqp_header res_header;

    int res = pthread_mutex_lock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

    res = queue_peek(&queue, &entry);
    if (res < 0) {
        if (errno != ENODATA) {
            errno = EIO;
        }

        ret = -1;
        goto release_lock;
    }

release_lock:
    res = pthread_mutex_unlock(&queue_lock);
    if (res != 0) {
        errno = EIO;
        ret = -1;
        goto reply;
    }

reply:
    // TODO: handle encryption
    res_header.method = DMQP_RESPONSE;
    res_header.flags = errno;
    res_header.length = entry.size;
    res_header.timestamp = entry.timestamp;

    res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        ret = -1;
        goto cleanup;
    }

    res = send(reply_socket, entry.data, res_header.length, 0);
    if (res < 0) {
        ret = -1;
        goto cleanup;
    }

cleanup:
    return ret;
}

int handle_dmqp_unknown_method(int reply_socket) {
    if (reply_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    struct dmqp_header res_header = {
        .method = DMQP_RESPONSE, .flags = ENOSYS, .length = 0, .timestamp = 0};

    int res = send(reply_socket, &res_header, sizeof(struct dmqp_header), 0);
    if (res < 0) {
        return -1;
    }

    return 0;
}
