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

/**
 * Reads a DMQP header from a file descriptor. Converts header fields to host
 * byte order.
 *
 * Throws an error if `fd` is invalid, `buf` is null, or if read fails.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP header buffer to write to
 * @returns 0 if success, -1 if error with global `errno` set
 */
static int read_dmqp_header(int fd, struct dmqp_header *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    char header_wire_buf[DMQP_HEADER_SIZE];
    int res = read_stream(fd, header_wire_buf, DMQP_HEADER_SIZE);
    if (res < 0) {
        return -1;
    }

    memcpy(&buf->timestamp, header_wire_buf, 8);
    memcpy(&buf->length, header_wire_buf + 8, 4);
    buf->method = header_wire_buf[13];
    buf->topic_id = header_wire_buf[14];
    buf->status_code = header_wire_buf[15];

    // no need to convert method, topic_id, and status_code since they're only a
    // byte
    buf->timestamp = ntohll(buf->timestamp);
    buf->length = ntohl(buf->length);

    return 0;
}

/**
 * Sends a DMQP header to a socket. Converts header fields to network byte
 * order (big endian).
 *
 * Throws an error if `socket` is invalid, `buf` is null, the message's payload is too large, or if
 * send fails.
 *
 * @param socket socket to write to
 * @param buf DMQP header buffer to send
 * @param flags same flags param as send syscall
 * @returns 0 if success, -1 if error with global `errno` set
 */
static int send_dmqp_header(int socket, const struct dmqp_header *buf,
                            int flags) {
    if (socket < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (buf->length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    // no need to convert method, topic_id, and status_code since they're only a
    // byte
    uint64_t network_byte_ordered_timestamp = htonll(buf->timestamp);
    uint32_t network_byte_ordered_length = htonl(buf->length);

    char header_wire_buf[DMQP_HEADER_SIZE];
    memcpy(header_wire_buf, &network_byte_ordered_timestamp, 8);
    memcpy(header_wire_buf + 8, &network_byte_ordered_length, 4);
    header_wire_buf[13] = buf->method;
    header_wire_buf[14] = buf->topic_id;
    header_wire_buf[15] = buf->status_code;

    ssize_t n =
        send_all(socket, header_wire_buf, sizeof(struct dmqp_header), flags);
    if (n < 0) {
        return -1;
    }

    return 0;
}

int read_dmqp_message(int fd, struct dmqp_message *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    int res = read_dmqp_header(fd, &buf->header);
    if (res < 0) {
        return -1;
    }

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

    int res = send_dmqp_header(socket, &buf->header, flags);
    if (res < 0) {
        return -1;
    }

    if (buf->header.length == 0) {
        return 0;
    }

    ssize_t n = send_all(socket, buf->payload, buf->header.length, flags);
    if (n < 0) {
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
        break;
    case DMQP_HEARTBEAT:
        res = handle_dmqp_heartbeat(&buf, socket);
        break;
    case DMQP_PUSH:
        res = handle_dmqp_push(&buf, socket);
        break;
    case DMQP_POP:
        res = handle_dmqp_pop(&buf, socket);
        break;
    case DMQP_PEEK:
        res = handle_dmqp_peek(&buf, socket);
        break;
    default:
        res = handle_dmqp_unknown_method(&buf, socket);
        break;
    }

    if (buf.payload != NULL) {
        free(buf.payload);
    }

    return res;
}
