#ifndef DMQP_H
#define DMQP_H

#define MB (1 << 20)

#define DEFAULT_SERVER_PORT 8080
#define MAX_PAYLOAD_LENGTH 1 * MB

#define ENCRYPTED_FLAG 1 << 8
#define STATUS_CODE_MASK 0xFF

enum dmqp_method { RESPONSE, HEARTBEAT, PUSH, POP, PEEK };

/**
 * flags:
 * ------------------------------------
 * | 31 - 9 |     8     |    7 - 0    |
 * ------------------------------------
 * | unused | encrypted | status_code |
 * ------------------------------------
 *
 * status_code is an errno
 */
struct dmqp_header {
    enum dmqp_method method;
    int flags;
    long queue_entry_timestamp;
    unsigned int length;
};

struct dmqp_message {
    struct dmqp_header header;
    void *payload;
};

/**
 * Reads a DMQP message from a file descriptor.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP message buffer to write to
 * @returns 0 if success, -1 if error with global `errno` set
 */
int read_dmqp_message(int fd, struct dmqp_message *buf);

/**
 * Sends a DMQP message to a socket.
 *
 * @param socket socket to write to
 * @param buf DMQP message buffer to send
 * @param flags same flags param as send syscall
 * @returns 0 if success, -1 if error with global `errno` set
 */
int send_dmqp_message(int socket, const struct dmqp_message *buf, int flags);

/**
 * Handles a message received at a DMQP server by parsing the message using the
 * DMQP format and and then serving the request.
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
