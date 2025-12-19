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
 * @returns the number of bytes read if success, -1 if error with global `errno`
 * set
 */
static ssize_t read_dmqp_header(int fd, struct dmqp_header *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    char header_wire_buf[DMQP_HEADER_SIZE];
    ssize_t n = read_all(fd, header_wire_buf, DMQP_HEADER_SIZE);
    if (n < DMQP_HEADER_SIZE) {
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

    return n;
}

/**
 * Sends a DMQP header to a socket. Converts header fields to network byte
 * order (big endian).
 *
 * Throws an error if `socket` is invalid, `buf` is null, the message's payload
 * is too large, or if send fails.
 *
 * @param socket socket to write to
 * @param buf DMQP header buffer to send
 * @param flags same flags param as send syscall
 * @returns the number of bytes sent if success, -1 if error with global `errno`
 * set
 */
static ssize_t send_dmqp_header(int socket, const struct dmqp_header *buffer,
                                int flags) {
    if (socket < 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (buffer->length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    // no need to convert method, topic_id, and status_code since they're only a
    // byte
    uint64_t network_byte_ordered_timestamp = htonll(buffer->timestamp);
    uint32_t network_byte_ordered_length = htonl(buffer->length);

    char header_wire_buf[DMQP_HEADER_SIZE];
    memcpy(header_wire_buf, &network_byte_ordered_timestamp, 8);
    memcpy(header_wire_buf + 8, &network_byte_ordered_length, 4);
    header_wire_buf[13] = buffer->method;
    header_wire_buf[14] = buffer->topic_id;
    header_wire_buf[15] = buffer->status_code;

    ssize_t n = send_all(socket, header_wire_buf, DMQP_HEADER_SIZE, flags);
    if (n < DMQP_HEADER_SIZE) {
        return -1;
    }

    return n;
}

int dmqp_client_init(const char *server_host, unsigned short server_port) {
    if (server_host == NULL) {
        errno = EINVAL;
        return -1;
    }

    int socket = client_init(server_host, server_port);
    if (socket < 0) {
        return -1;
    }

    return socket;
}

int dmqp_server_init(unsigned short server_port) {
    int res = server_init(server_port, handle_dmqp_message);
    if (res < 0) {
        return -1;
    }

    return 0;
}

ssize_t read_dmqp_message(int fd, struct dmqp_message *buf) {
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
        return DMQP_HEADER_SIZE;
    }

    buf->payload = malloc(buf->header.length);
    if (buf->payload == NULL) {
        errno = ENOMEM;
        return -1;
    }

    ssize_t n = read_all(fd, buf->payload, buf->header.length);
    if (n < buf->header.length) {
        free(buf->payload);
        return -1;
    }

    return DMQP_HEADER_SIZE + buf->header.length;
}

ssize_t send_dmqp_message(int socket, const struct dmqp_message *buffer,
                          int flags) {
    if (socket < 0 || buffer == NULL ||
        (buffer->header.length > 0 && buffer->payload == NULL)) {
        errno = EINVAL;
        return -1;
    }

    if (buffer->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    int res = send_dmqp_header(socket, &buffer->header, flags);
    if (res < 0) {
        return -1;
    }

    if (buffer->header.length == 0) {
        return DMQP_HEADER_SIZE;
    }

    ssize_t n = send_all(socket, buffer->payload, buffer->header.length, flags);
    if (n < buffer->header.length) {
        return -1;
    }

    return DMQP_HEADER_SIZE + buffer->header.length;
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
    case DMQP_PUSH:
        res = handle_dmqp_push(&buf, socket);
        break;
    case DMQP_POP:
        res = handle_dmqp_pop(&buf, socket);
        break;
    case DMQP_PEEK:
        res = handle_dmqp_peek(&buf, socket);
        break;
    case DMQP_PEEK_TIMESTAMP:
        res = handle_dmqp_peek_timestamp(&buf, socket);
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
