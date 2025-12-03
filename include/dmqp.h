#ifndef DMQP_H
#define DMQP_H

#define MB 1 << 20

#define DEFAULT_SERVER_PORT 8080
#define MAX_PAYLOAD_LENGTH 1 * MB

#define ENCRYPTED_FLAG 0x1

enum dmqp_method { PUSH, POP, PEEK, HEARTBEAT, HEARTBEAT_ACK };

/**
 * flags:
 * ----------------------
 * | 31 - 1 |     0     |
 * ----------------------
 * | unused | encrypted |
 * ----------------------
 */
struct dmqp_header {
    enum dmqp_method method;
    int flags;
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
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_server_message(void *message, unsigned int message_size);

/**
 * Handles a message received at a DMQP server.
 *
 * @param message message received at server
 * @returns 0 if success, -1 if error with global `errno` set
 */
int handle_dmqp_message(struct dmqp_message message);

#endif
