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

struct connection_handler_args {
    int conn_socket;
    int (*message_handler)(void *message, unsigned int message_size);
};

/**
 * Listens on a TCP connection.
 *
 * @param arg pointer to args of type `struct connection_handler_args`. must be
 * free once copied locally
 */
static void *connection_handler(void *arg) {
    struct connection_handler_args args =
        *(struct connection_handler_args *)arg;
    free(arg);

    while (1) {
        char buf[1024];
        int res = read_message(args.conn_socket, buf, sizeof(buf));
        if (res < 0) {
            break;
        }

        args.message_handler(buf, sizeof(buf));
    }

    close(args.conn_socket);
    return NULL;
}

int client_init(const char *server_host, unsigned int server_port) {
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

    res = connect(client_socket, (const struct sockaddr *)&server_address,
                  sizeof(server_address));
    if (res < 0) {
        close(client_socket);
        return -1;
    }

    return client_socket;
}

int server_init(unsigned int server_port,
                int (*message_handler)(void *message,
                                       unsigned int message_size)) {
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

    int res = bind(server_socket, (const struct sockaddr *)&server_address,
                   address_len);
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

        int client_socket =
            accept(server_socket, (struct sockaddr *)&client_address,
                   &client_address_len);
        if (client_socket < 0) {
            // TODO: error handling
            continue;
        }

        struct connection_handler_args *args =
            malloc(sizeof(struct connection_handler_args));
        if (args == NULL) {
            // TODO: error handling
            continue;
        }
        args->conn_socket = client_socket;
        args->message_handler = message_handler;

        pthread_t tid;
        pthread_create(&tid, NULL, connection_handler, args);
    }

    return 0;
}

int read_message(int conn_socket, void *buf, unsigned int message_size) {
    unsigned int total = 0;

    while (total < message_size) {
        int n = read(conn_socket, (char *)buf + total, message_size - total);
        if (n <= 0) {
            return -1;
        }

        total += n;
    }

    return 0;
}
