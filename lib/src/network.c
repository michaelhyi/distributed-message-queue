#include "network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

struct connection_handler_args {
    int socket; // socket of connection
    int (*message_handler)(
        int socket); // handler that reads a message, parses it, and serves it.
                     // takes in the socket of connection as an argument to
                     // handle responses
};

static volatile sig_atomic_t running = 1;
static struct sigaction sa;

static void signal_handler(int sig) {
    (void)sig; // unused
    running = 0;
}

/**
 * Listens on a connection.
 *
 * Throws an error if `arg` is null, `arg->socket` is invalid, or
 * `arg->message_handler` is null.
 *
 * @param arg pointer to args of type `struct connection_handler_args`. must be
 * free once copied locally
 */
static void *connection_handler(void *arg) {
    struct connection_handler_args args = {
        .socket = -1,
        .message_handler = NULL}; // initialized to prevent compiler error

    if (arg == NULL) {
        errno = EINVAL;
        goto cleanup;
    }

    args = *(struct connection_handler_args *)arg;
    free(arg);

    if (args.socket < 0 || args.message_handler == NULL) {
        errno = EINVAL;
        goto cleanup;
    }

    while (running) {
        int res = args.message_handler(args.socket);
        if (res < 0) {
            goto cleanup;
        }
    }

cleanup:
    if (args.socket >= 0) {
        close(args.socket);
    }

    return NULL;
}

int client_init(const char *server_host, unsigned short server_port) {
    if (server_host == NULL) {
        errno = EINVAL;
        return -1;
    }

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

int server_init(unsigned short server_port,
                int (*message_handler)(int socket)) {
    if (message_handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    int res = sigaction(SIGINT, &sa, NULL);
    if (res < 0) {
        return -1;
    }

    res = sigaction(SIGTERM, &sa, NULL);
    if (res < 0) {
        return -1;
    }

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

    res = bind(server_socket, (const struct sockaddr *)&server_address,
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

    while (running) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        int client_socket =
            accept(server_socket, (struct sockaddr *)&client_address,
                   &client_address_len);
        if (client_socket < 0) {
            continue;
        }

        struct timeval timeout = {.tv_sec = 30, .tv_usec = 0};
        res = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
                         (const void *)&timeout, sizeof(timeout));
        if (res < 0) {
            close(client_socket);
            continue;
        }

        int keepalive = 1;
        res = setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
                         sizeof(keepalive));
        if (res < 0) {
            close(client_socket);
            continue;
        }

        struct connection_handler_args *args =
            malloc(sizeof(struct connection_handler_args));
        if (args == NULL) {
            close(client_socket);
            continue;
        }
        args->socket = client_socket;
        args->message_handler = message_handler;

        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_create(&thread, &attr, connection_handler, args);
        pthread_attr_destroy(&attr);
    }

    errno = 0;
    close(server_socket);
    return 0;
}

ssize_t read_all(int fd, void *buf, size_t count) {
    if (buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    unsigned int total = 0;

    while (total < count) {
        int n = read(fd, (char *)buf + total, count - total);
        if (n <= 0) {
            return -1;
        }

        total += n;
    }

    return total;
}

ssize_t send_all(int socket, const void *buffer, size_t length, int flags) {
    if (socket < 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    unsigned int total = 0;

    while (total < length) {
        ssize_t n = send(socket, (char *)buffer + total, length - total, flags);
        if (n <= 0) {
            return -1;
        }

        total += n;
    }

    return total;
}
