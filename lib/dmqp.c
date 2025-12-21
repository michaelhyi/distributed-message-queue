#include "dmqp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "network.h"

/**
 * Reads a DMQP header from a file descriptor. Converts header fields to host
 * byte order.
 *
 * Throws an error if `fd` is invalid, `buf` is null, or if read fails.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP header buffer to write to
 * @returns 0 on success, -1 if error with global `errno` set
 */
static int read_dmqp_header(int fd, struct dmqp_header *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    char header_wire_buf[DMQP_HEADER_SIZE];
    ssize_t n = read_all(fd, header_wire_buf, DMQP_HEADER_SIZE);
    if (n < DMQP_HEADER_SIZE) {
        return -1;
    }

    memcpy(&buf->sequence_id, header_wire_buf, 4);
    memcpy(&buf->length, header_wire_buf + 4, 4);
    memcpy(&buf->method, header_wire_buf + 8, 2);
    memcpy(&buf->status_code, header_wire_buf + 10, 2);

    buf->sequence_id = ntohl(buf->sequence_id);
    buf->length = ntohl(buf->length);
    buf->method = ntohs(buf->method);
    buf->status_code = ntohs(buf->status_code);

    return 0;
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
 * @param flags same flags param as `send` syscall
 * @returns 0 on success, -1 if error with global `errno` set
 */
static int send_dmqp_header(int socket, const struct dmqp_header *buffer,
                            int flags) {
    if (socket < 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (buffer->length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    uint32_t network_byte_ordered_sequence_id = htonl(buffer->sequence_id);
    uint32_t network_byte_ordered_length = htonl(buffer->length);
    uint16_t network_byte_ordered_method = htons(buffer->method);
    int16_t network_byte_ordered_status_code = htons(buffer->status_code);

    char header_wire_buf[DMQP_HEADER_SIZE];
    memcpy(header_wire_buf, &network_byte_ordered_sequence_id, 4);
    memcpy(header_wire_buf + 4, &network_byte_ordered_length, 4);
    memcpy(header_wire_buf + 8, &network_byte_ordered_method, 2);
    memcpy(header_wire_buf + 10, &network_byte_ordered_status_code, 2);

    ssize_t n = send_all(socket, header_wire_buf, DMQP_HEADER_SIZE, flags);
    if (n < 0) {
        return -1;
    }

    return 0;
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

    ssize_t n = read_all(fd, buf->payload, buf->header.length);
    if (n < buf->header.length) {
        free(buf->payload);
        return -1;
    }

    return 0;
}

int send_dmqp_message(int socket, const struct dmqp_message *buffer,
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
        return 0;
    }

    ssize_t n = send_all(socket, buffer->payload, buffer->header.length, flags);
    if (n < buffer->header.length) {
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
    case DMQP_PUSH:
        res = handle_dmqp_push(&buf, socket);
        break;
    case DMQP_POP:
        res = handle_dmqp_pop(&buf, socket);
        break;
    case DMQP_PEEK_SEQUENCE_ID:
        res = handle_dmqp_peek_sequence_id(&buf, socket);
        break;
    case DMQP_RESPONSE:
        res = handle_dmqp_response(&buf, socket);
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

/**
 * Determines if a given method is defined by DMQP.
 *
 * @param method method to check if defined
 * @returns 1 if method is defined, 0 otherwise
 */
static inline int is_defined_method(enum dmqp_method method) {
    return method == DMQP_PUSH || method == DMQP_POP ||
           method == DMQP_PEEK_SEQUENCE_ID || method == DMQP_RESPONSE;
}

int handle_dmqp_unknown_method(const struct dmqp_message *message,
                               int reply_socket) {
    if (message == NULL || is_defined_method(message->header.method)) {
        errno = EINVAL;
        return -1;
    }

    if (!is_socket(reply_socket)) {
        errno = ENOTSOCK;
        return -1;
    }

    struct dmqp_header res_header = {.sequence_id = 0,
                                     .length = 0,
                                     .method = DMQP_RESPONSE,
                                     .status_code = ENOSYS};
    struct dmqp_message res_message = {.header = res_header};

    int res = send_dmqp_message(reply_socket, &res_message, 0);
    if (res < 0) {
        return -1;
    }

    return 0;
}
