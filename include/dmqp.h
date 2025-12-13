#ifndef DMQP_H
#define DMQP_H

#include <stdint.h>
#include <sys/types.h>

#define DEFAULT_SERVER_PORT 8080
#define MB (1 << 20)
#define MAX_PAYLOAD_LENGTH 1 * MB
#define DMQP_HEADER_SIZE 16

enum dmqp_method { DMQP_RESPONSE, DMQP_PUSH, DMQP_POP, DMQP_PEEK };

// All header fields are expected to be in network byte order (big endian).
struct dmqp_header {
    uint64_t timestamp; // unix epoch, 1 second resolution. set by the top-level
                        // server on a DMQP_PUSH request, indiciating the
                        // timestamp at which data was received for in-order
                        // delivery. set by the partition on a DMQP_PEEK request
                        // to handle in-order popping
    uint32_t length;    // payload length
    uint8_t method;     // maps to `enum dmqp_method`
    uint8_t topic_id;   // id of topic to perform operation on. set by top-level
                        // server.
    int8_t status_code; // errno
};

struct dmqp_message {
    struct dmqp_header header;
    void *payload;
};

/**
 * Reads a DMQP message from a file descriptor. Converts header fields to host
 * byte order.
 *
 * Throws an error if `fd` is invalid, `buf` is null, if the
 * message's payload is too large, or if read fails.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP message buffer to write to
 * @returns the number of bytes read if success, -1 if error with global `errno`
 * set
 */
ssize_t read_dmqp_message(int fd, struct dmqp_message *buf);

/**
 * Sends a DMQP message to a socket. Converts header fields to network byte
 * order (big endian).
 *
 * Throws an error if `socket` is invalid, `buffer` is null, the message has a
 * specified length but no payload, the message's payload is too large, or if
 * send fails.
 *
 * @param socket socket to write to
 * @param buffer DMQP message buffer to send
 * @param flags same flags param as send syscall
 * @returns the number of bytes sent if success, -1 if error with global `errno`
 * set
 */
ssize_t send_dmqp_message(int socket, const struct dmqp_message *buffer,
                          int flags);

/**
 * Handles a message received at a DMQP server by reading the message
 * from a socket and serving the request.
 *
 * Throws an error if `socket` is invalid.
 *
 * @param socket socket that sent message
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_message(int socket);

// ----------------------------------------------------------------------------
// The following functions must be implemented separately by each DMQP server.

/**
 * Handles a DMQP message with method `DMQP_RESPONSE`.
 *
 * Throws an error if `message` is null, the method is not `DMQP_RESPONSE`, or
 * `reply_socket` is invalid.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_response(const struct dmqp_message *message, int reply_socket);

/**
 * Handles a DMQP message with method `DMQP_PUSH`.
 *
 * Throws an error if `message` is null, the method is not `DMQP_PUSH`, the
 * payload is empty, or `reply_socket` is invalid.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_push(const struct dmqp_message *message, int reply_socket);

/**
 * Handles a DMQP message with method `DMQP_POP`.
 *
 * Throws an error if `message` is null, the method is not `DMQP_POP`, or
 * `reply_socket` is invalid.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_pop(const struct dmqp_message *message, int reply_socket);

/**
 * Handles a DMQP message with method `DMQP_PEEK`.
 *
 * Throws an error if `message` is null, the method is not `DMQP_PEEK`, or
 * `reply_socket` is invalid.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_peek(const struct dmqp_message *message, int reply_socket);

/**
 * Handles a DMQP message with unknown method.
 *
 * Throws an error if `message` is null, the method is not unknown, or
 * `reply_socket` is invalid.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_unknown_method(const struct dmqp_message *message,
                               int reply_socket);

#endif
