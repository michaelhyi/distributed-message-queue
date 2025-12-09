#ifndef DMQP_H
#define DMQP_H

#define MB 1 << 20

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
 * Handles a message received at a TCP server.
 *
 * @param message message received at server
 * @param message_size number of bytes received at server
 * @param conn_socket socket of connection where the message was received
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_server_message(void *message, unsigned int message_size,
                          int conn_socket);

/**
 * Handles a message received at a DMQP server.
 *
 * @param message message received at server
 * @param conn_socket socket of connection where the message was received
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_message(struct dmqp_message message, int conn_socket);

#endif
