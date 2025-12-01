#ifndef NETWORK_H
#define NETWORK_H

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
 * Initializes a TCP server using sockets.
 * 
 * @param server_port the port to bind
 * @returns 0 on success, -1 on error
 */
int init_server(unsigned int server_port);

/**
 * Receives a message from a TCP connection.
 * 
 * @param conn_socket the socket of the incoming TCP connection
 * @returns the message received at the server, `NULL` if error
 */
struct message *receive_message(int conn_socket);

#endif
