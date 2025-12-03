#ifndef NETWORK_H
#define NETWORK_H

/**
 * TODO:
 * 1. get small server working first
 * 2. errno
 * 3. clean up resources
 * 4. get small client working first
 * 5. unit testing?
 * 6. timeouts
 * 7. signal handling
 * 8. heartbeats
 */

#define DEFAULT_SERVER_PORT 8080
#define MAX_PAYLOAD_LENGTH 1 * MB
#define MB 1 << 20
#define ENCRYPTED_FLAG 0x1

enum message_type {
    PUSH,
    POP,
    PEEK,
    HEARTBEAT,
    HEARTBEAT_ACK
};

/**
 * flags:
 * ----------------------
 * | 31 - 1 |     0     |
 * ----------------------
 * | unused | encrypted |
 * ----------------------
 */
struct message_header {
    enum message_type type;
    int flags;
    unsigned int length;
};

struct message {
    struct message_header header;
    void *payload;
};

/**
 * Initializes a TCP client connection to a server.
 * 
 * @param server_address the server address
 * @param server_port the server port
 * @returns the socket file descriptor for the client connection, -1 if error
 * with global `errno` set
 */
int init_client(const char *server_address, unsigned int server_port);

/**
 * Initializes a TCP server.
 * 
 * @param server_port the port to bind the server to
 * @returns 0 on success, -1 on error with global `errno` set
 */
int init_server(unsigned int server_port);

/**
 * Sends a message to a TCP connection.
 * 
 * @param conn_socket TCP connection to send message to
 * @param message the message to send
 * @returns 0 if success, -1 if error with global `errno` set
 */
int send_message(int conn_socket, struct message message);

/**
 * Receives a message from a TCP connection.
 * 
 * @param conn_socket TCP connection to receive message from
 * @returns the message received at the server, `NULL` if error with global
 * `errno` set
 */
struct message *receive_message(int conn_socket);

#endif
