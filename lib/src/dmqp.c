#include "dmqp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "network.h"

#if defined(__linux__)
#include <endian.h>
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/endian.h>
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)
#else // Fallback implementation
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ntohll(x)                                                              \
    ((((uint64_t)ntohl((uint32_t)(x))) << 32) | ntohl((uint32_t)((x) >> 32)))
#define htonll(x)                                                              \
    ((((uint64_t)htonl((uint32_t)(x))) << 32) | htonl((uint32_t)((x) >> 32)))
#else
#define ntohll(x) (x)
#define htonll(x) (x)
#endif
#endif

int read_dmqp_message(int fd, struct dmqp_message *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    int res = read_stream(fd, &buf->header, sizeof(struct dmqp_header));
    if (res < 0) {
        return -1;
    }

    // no need to convert method, topic_id, and status_code since they're only a
    // byte
    buf->header.timestamp = ntohll(buf->header.timestamp);
    buf->header.length = ntohl(buf->header.length);

    if (buf->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    if (buf->header.length == 0) {
        buf->payload = NULL;
        return 0;
    }

    buf->payload = malloc(buf->header.length);
    if (buf->payload == NULL) {
        errno = ENOMEM;
        return -1;
    }

    res = read_stream(fd, buf->payload, buf->header.length);
    if (res < 0) {
        free(buf->payload);
        return -1;
    }

    return 0;
}

int send_dmqp_message(int socket, const struct dmqp_message *buf, int flags) {
    if (socket < 0 || buf == NULL ||
        (buf->header.length > 0 && buf->payload == NULL)) {
        errno = EINVAL;
        return -1;
    }

    if (buf->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    // no need to convert method, topic_id, and status_code since they're only a
    // byte
    struct dmqp_header network_byte_header = {
        .timestamp = htonll(buf->header.timestamp),
        .length = htonl(buf->header.length),
        .method = buf->header.method,
        .topic_id = buf->header.topic_id,
        .status_code = buf->header.status_code};

    int res =
        send(socket, &network_byte_header, sizeof(struct dmqp_header), flags);
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

    switch (buf.header.method) {
    case DMQP_RESPONSE:
        res = handle_dmqp_response(&buf, socket);
        goto cleanup;
    case DMQP_HEARTBEAT:
        res = handle_dmqp_heartbeat(&buf, socket);
        goto cleanup;
    case DMQP_PUSH:
        res = handle_dmqp_push(&buf, socket);
        goto cleanup;
    case DMQP_POP:
        res = handle_dmqp_pop(&buf, socket);
        goto cleanup;
    case DMQP_PEEK:
        res = handle_dmqp_peek(&buf, socket);
        goto cleanup;
    default:
        res = handle_dmqp_unknown_method(&buf, socket);
        goto cleanup;
    }

cleanup:
    if (buf.payload != NULL) {
        free(buf.payload);
    }

    return res;
}
