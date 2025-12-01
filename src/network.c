#include "network.h"

#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static volatile sig_atomic_t stop = 0;
static int server_socket = -1;

static void handle_signal(int sig) {
    (void) sig; // suppress unused param error

    stop = 1;

    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

static void init_signals() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void *handle_requests(void *arg) {
    (void) arg; // suppress unused param error

    while (!stop) {
        struct sockaddr_in client_address;
        socklen_t address_len = sizeof(client_address);

        int conn_socket = accept(server_socket, (struct sockaddr *) &client_address, &address_len);
        if (conn_socket < 0) {
            continue;
        }

        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        int res = setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (res < 0) {
            close(conn_socket);
            continue;
        }

        struct message *message = receive_message(conn_socket);
        if (message == NULL) {
            close(conn_socket);
            continue;
        }

        // TODO: persist conn --> heartbeats?
        // TODO: do something with message
        free(message->payload);
        free(message);
    }

    return NULL;
}

int init_server(unsigned int server_port) {
    init_signals();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
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

    pthread_t threads[NUM_REQUEST_HANDLER_THREADS];
    for (int i = 0; i < NUM_REQUEST_HANDLER_THREADS; i++) {
        pthread_create(&threads[i], NULL, handle_requests, NULL);
    }

    for (int i = 0; i < NUM_REQUEST_HANDLER_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

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

struct message *receive_message(int conn_socket) {
    struct message *message = malloc(sizeof (struct message));
    if (message == NULL) {
        return NULL;
    }

    int res = read_stream(conn_socket, &message->header, sizeof(struct message_header));
    if (res < 0) {
        free(message);
        return NULL;
    }

    if (message->header.length >= MAX_PAYLOAD_LENGTH) {
        free(message);
        return NULL;
    }

    message->payload = malloc(message->header.length);
    if (message->payload == NULL) {
        free(message);
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
