#ifndef DMQP_H
#define DMQP_H

#include <stdint.h>

#define DEFAULT_SERVER_PORT 8080
#define MB (1 << 20)
#define MAX_PAYLOAD_LENGTH 1 * MB

enum dmqp_method {
    DMQP_RESPONSE,
    DMQP_HEARTBEAT,
    DMQP_PUSH,
    DMQP_POP,
    DMQP_PEEK
};

// All header fields are expected to be in network-byte order (big endian).
struct dmqp_header {
    uint8_t method;     // casted to `enum dmqp_method`
    uint8_t topic_id;   // id of topic to push data on. set by top-level server.
    int8_t status_code; // errno
    uint32_t length;    // payload length
    int64_t timestamp;  // unix epoch, 1 second resolution. set by the top-level
                        // server on a DMQP_PUSH request, indiciating the
                        // timestamp at which data was received for in-order
                        // delivery. set by the partition on a DMQP_PEEK request
                        // to handle in-order popping
};

struct dmqp_message {
    struct dmqp_header header;
    void *payload;
};

/**
 * Reads a DMQP message from a file descriptor. Converts header fields to be
 * little endian.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP message buffer to write to
 * @returns 0 if success, -1 if error with global `errno` set
 */
int read_dmqp_message(int fd, struct dmqp_message *buf);

/**
 * Sends a DMQP message to a socket. Converts header fields to network-byte
 * order (big endian).
 *
 * @param socket socket to write to
 * @param buf DMQP message buffer to send
 * @param flags same flags param as send syscall
 * @returns 0 if success, -1 if error with global `errno` set
 */
int send_dmqp_message(int socket, const struct dmqp_message *buf, int flags);

/**
 * Handles a message received at a DMQP server by reading the message
 * from a socket and serving the request.
 *
 * @param socket socket that sent message
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_message(int socket);

/**
 * Handles a DMQP message with method response.
 *
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_response(int reply_socket);

/**
 * Handles a DMQP message with method heartbeat.
 *
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_heartbeat(int reply_socket);

/**
 * Handles a DMQP message with method push.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_push(struct dmqp_message message, int reply_socket);

/**
 * Handles a DMQP message with method pop.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_pop(int reply_socket);

/**
 * Handles a DMQP message with method peek.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_peek(int reply_socket);

/**
 * Handles a DMQP message with unknown method.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_unknown_method(int reply_socket);

#endif
