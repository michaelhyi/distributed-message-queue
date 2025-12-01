#ifndef NETWORK_H
#define NETWORK_H

#define DEFAULT_SERVER_PORT 8080
#define MAX_PAYLOAD_LENGTH 1 * MB
#define MB 1 << 20
#define NUM_REQUEST_HANDLER_THREADS 5

enum message_type {
    PUSH,
    POP,
    HEARTBEAT,
    HEARTBEAT_ACK
};

struct message_header {
    enum message_type type;
    int encrypted;
    unsigned int length;
};

struct message {
    struct message_header header;
    void *payload;
};

/**
 * Initializes a TCP client connection to a server using sockets.
 * 
 * @returns the socket file descriptor for the client connection, -1 if error
 */
int init_client(const char *server_address, unsigned int server_port);

/**
 * Initializes a TCP server using sockets.
 * 
 * @param server_port the port to bind
 * @returns 0 on success, -1 on error
 */
int init_server(unsigned int server_port);

/**
 * Sends a message to a TCP connection.
 * 
 * @param client_socket TCP connection to send message to
 * @param message the message to send
 * @returns 0 if success, -1 if error
 */
int send_message(int client_socket, struct message message);

/**
 * Receives a message from a TCP connection.
 * 
 * @param conn_socket TCP connection to receive message from
 * @returns the message received at the server, `NULL` if error
 */
struct message *receive_message(int conn_socket);

#endif
