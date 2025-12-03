#include "network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Reads bytes into a buffer from a TCP stream.
 * 
 * @param fd file descriptor to read from
 * @param buf buffer to write to
 * @param length number of bytes to read
 * @returns 0 if success, -1 if error with global `errno` set
 */
static int read_stream(int fd, void *buf, unsigned int length) {
    unsigned int total = 0;

    while (total < length) {
        int n = read(fd, (char *) buf + total, length - total);
        if (n <= 0) {
            return -1;
        }

        total += n;
    }

    return 0;
}

/**
 * Listens on a TCP connection.
 * 
 * @param arg pointer to connection socket. must be freed once locally copied
 */
static void *connection_handler(void *arg) {
    int conn_socket = *(int *) arg;
    free(arg);

    while (1) {
        struct message *message = receive_message(conn_socket);
        if (message == NULL) {
            continue;
        }

        // TODO: do something with message
        printf("received message: %s\n", (const char *) message->payload);

        free(message->payload);
        free(message);
    }

    // TODO: close conn_socket
    return NULL;
}

int init_client(const char *server_host, unsigned int server_port) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    int res = inet_pton(AF_INET, server_host, &server_address.sin_addr);
    if (res < 0) {
        close(client_socket);
        return -1;
    }

    res = connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address));
    if (res < 0) {
        close(client_socket);
        return -1;
    }

    return client_socket;
}

int init_server(unsigned int server_port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);
    socklen_t address_len = sizeof(server_address);

    int res = bind(server_socket, (const struct sockaddr *) &server_address, address_len);
    if (res < 0) {
        close(server_socket);
        return -1;
    }

    res = listen(server_socket, 16);
    if (res < 0) {
        close(server_socket);
        return -1;
    }

    printf("server listening on port %d\n", ntohs(server_address.sin_port));

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_len);
        if (client_socket < 0) {
            // TODO: error handling
            continue;
        }

        int *client_socket_copy = malloc(sizeof(int));
        if (client_socket_copy == NULL) {
            close(client_socket);
            continue;
        } 
        *client_socket_copy = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, connection_handler, client_socket_copy);
    }

    return 0;
}

int send_message(int conn_socket, struct message message) {
    int n = send(conn_socket, (const void *) &message.header, sizeof(struct message_header), 0);
    if (n < 0) {
        return -1;
    }

    n = send(conn_socket, message.payload, message.header.length, 0);
    if (n < 0) {
        return -1;
    }

    return 0;
}

struct message *receive_message(int conn_socket) {
    struct message *message = malloc(sizeof (struct message));
    if (message == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int res = read_stream(conn_socket, &message->header, sizeof(struct message_header));
    if (res < 0) {
        free(message);
        return NULL;
    }

    if (message->header.length >= MAX_PAYLOAD_LENGTH) {
        free(message);
        errno = EMSGSIZE;
        return NULL;
    }

    message->payload = malloc(message->header.length);
    if (message->payload == NULL) {
        free(message);
        errno = ENOMEM;
        return NULL;
    }

    res = read_stream(conn_socket, message->payload, message->header.length);
    if (res < 0) {
        free(message->payload);
        free(message);
        return NULL;
    }

    return message;
}
