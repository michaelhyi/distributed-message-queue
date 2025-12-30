#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

#define LISTEN_BACKLOG 128
#define MAX_PAYLOAD_LENGTH (1 << 20) // 1MB
#define DMQP_HEADER_SIZE 12          // bytes

enum dmqp_method { DMQP_PUSH, DMQP_POP, DMQP_PEEK_SEQUENCE_ID, DMQP_RESPONSE };

struct dmqp_header {
    uint32_t sequence_id; // unique sequence number of queue entry
    uint32_t length;      // payload length
    uint16_t method;      // maps to `enum dmqp_method`
    int16_t status_code;  // unix errno
};

struct dmqp_message {
    struct dmqp_header header;
    void *payload;
};

extern volatile int server_running;
extern unsigned short server_port;

// TODO: TLS

/**
 * Initializes a DMQP client connection to a DMQP server.
 *
 * @param host the server host address
 * @param port the server port
 * @returns the socket of the DMQP client, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `EIO` unexpected error
 */
int dmqp_client_init(const char *host, unsigned short port);

/**
 * Initializes a DMQP server. Signals are handled to gracefully exit.
 *
 * @param port the port to bind the server to
 * @returns 0 on success, -1 on error with global `errno` set. does not return
 * until the server is interrupted or terminated
 * @throws `EIO` unexpected error
 */
int dmqp_server_init(unsigned short port);

/**
 * Reads a DMQP message from a file descriptor. Converts header fields to host
 * byte order.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP message buffer to write to
 * @returns 0 on success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `EMSGSIZE` message payload too large
 * @throws `EIO` unexpected error
 */
int read_dmqp_message(int fd, struct dmqp_message *buf);

/**
 * Sends a DMQP message to a file descriptor. Converts header fields to network
 * byte order (big endian).
 *
 * @param fd file descriptor to write to
 * @param buffer DMQP message buffer to send
 * @param flags same flags param as send syscall
 * @returns 0 on success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `EMSGSIZE` message payload too large
 * @throws `EIO` unexpected error
 */
int send_dmqp_message(int fd, const struct dmqp_message *buffer, int flags);

// ----------------------------------------------------------------------------
// The following functions must be implemented separately by each DMQP server.

/**
 * Handles a DMQP message with method `DMQP_PUSH`.
 *
 * @param message message received by server
 * @param client socket to reply on
 */
void handle_dmqp_push(const struct dmqp_message *message, int client);

/**
 * Handles a DMQP message with method `DMQP_POP`.
 *
 * @param message message received by server
 * @param client socket to reply on
 */
void handle_dmqp_pop(const struct dmqp_message *message, int client);

/**
 * Handles a DMQP message with method `DMQP_PEEK_SEQUENCE_ID`.
 *
 * @param message message received by server
 * @param reply_socket socket to reply on
 */
void handle_dmqp_peek_sequence_id(const struct dmqp_message *message,
                                  int client);

/**
 * Handles a DMQP message with method `DMQP_RESPONSE`.
 *
 * @param message message received by server
 * @param client socket to reply on
 */
void handle_dmqp_response(const struct dmqp_message *message, int client);

#endif
