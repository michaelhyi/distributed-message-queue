#include "dmqp.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "network.h"

int read_dmqp_message(int fd, struct dmqp_message *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    int res = receive_message(fd, &buf->header, sizeof(struct dmqp_header));
    if (res < 0) {
        return -1;
    }

    // TODO: validate method exists?
    if (buf->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    buf->payload = malloc(buf->header.length);
    if (buf->payload == NULL) {
        errno = ENOMEM;
        return -1;
    }

    res = receive_message(fd, buf->payload, buf->header.length);
    if (res < 0) {
        free(buf->payload);
        return -1;
    }

    return 0;
}

int send_dmqp_message(int socket, const struct dmqp_message *buf, int flags) {
    // TODO: validate that method exists?
    if (socket < 0 || buf == NULL || buf->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EINVAL;
        return -1;
    }

    int res = send(socket, &buf->header, sizeof(struct dmqp_header), flags);
    if (res < 0) {
        return -1;
    }

    res = send(socket, buf->payload, buf->header.length, flags);
    if (res < 0) {
        return -1;
    }

    return 0;
}

int handle_dmqp_message(int socket) {
    if (socket < 0) {
        errno = EINVAL;
        return -1;
    }

    struct dmqp_message buf;
    int res = read_dmqp_message(socket, &buf);
    if (res < 0) {
        return -1;
    }

    // TODO: validate buf
    //     if ((message.header.length > 0 && message.payload == NULL) ||
    //     message.header.length > MAX_PAYLOAD_LENGTH || reply_socket < 0) {
    //     errno = EINVAL;
    //     return -1;
    // }

    int ret = 0;
    switch (buf.header.method) {
    case DMQP_RESPONSE:
        // TODO: error handling should be unique per implementation
        ret = -1;
        res = handle_dmqp_response(socket);
        if (res < 0) {
            goto cleanup;
        }

        errno = EPROTO;
        goto cleanup;
    case DMQP_HEARTBEAT:
        res = handle_dmqp_heartbeat(socket);
        if (res < 0) {
            ret = -1;
        }

        goto cleanup;
    case DMQP_PUSH:
        res = handle_dmqp_push(buf, socket);
        if (res < 0) {
            ret = -1;
        }

        goto cleanup;
    case DMQP_POP:
        res = handle_dmqp_pop(socket);
        if (res < 0) {
            ret = -1;
        }

        goto cleanup;
    case DMQP_PEEK:
        res = handle_dmqp_peek(socket);
        if (res < 0) {
            ret = -1;
        }

        goto cleanup;
    default:
        res = handle_dmqp_unknown_method(socket);
        if (res < 0) {
            ret = -1;
        }

        goto cleanup;
    }

cleanup:
    free(buf.payload);
    return ret;
}
