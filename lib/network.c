#include "messageq/network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

int dmqp_client_init(const char *host, unsigned short port) {
    if (!host) {
        errno = EINVAL;
        return -1;
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        errno = EIO;
        return -1;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &address.sin_addr) <= 0) {
        goto cleanup;
    }

    if (connect(client, (struct sockaddr *)&address, sizeof address) < 0) {
        goto cleanup;
    }

    return client;

cleanup:
    close(client);
    errno = EIO;
    return -1;
}

pthread_mutex_t server_lock;
pthread_cond_t server_running_cond;
int server_running = 0;
unsigned short server_port = 0;
static struct sigaction sa;
static struct sigaction sa_ignore;

/**
 * Simple signal handler that sets `server_running` to zero to terminate server
 * threads.
 *
 * @param sig signal received by process
 */
static void signal_handler(int sig) {
    (void)sig; // unused
    pthread_mutex_lock(&server_lock);
    server_running = 0;
    pthread_mutex_unlock(&server_lock);
    pthread_cond_broadcast(&server_running_cond);
}

/**
 * Initializes signal handling. Gracefully terminates server when `SIGINT` or
 * `SIGTERM` is received. Ignores when `SIGPIPE` is received.
 */
static void signal_init() {
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGPIPE, &sa_ignore, NULL);
}

/**
 * Listens on a connection.
 *
 * @param arg pointer to client socket. must be freed once copied
 */
static void *connection_handler(void *arg) {
    int client = *(int *)arg;
    free(arg);

    for (;;) {
        pthread_mutex_lock(&server_lock);
        int running = server_running;
        pthread_mutex_unlock(&server_lock);

        if (!running) {
            break;
        }

        struct dmqp_message buf;
        // TODO: this means that any 30s timeout will clean resources. must only
        // close thread if tcp keepalive expires or connection closed
        if (read_dmqp_message(client, &buf) < 0) {
            break;
        }

        switch (buf.header.method) {
        case DMQP_PUSH:
            handle_dmqp_push(&buf, client);
            break;
        case DMQP_POP:
            handle_dmqp_pop(&buf, client);
            break;
        case DMQP_PEEK_SEQUENCE_ID:
            handle_dmqp_peek_sequence_id(&buf, client);
            break;
        case DMQP_RESPONSE:
            handle_dmqp_response(&buf, client);
            break;
        default:;
            struct dmqp_header header = {0};
            header.method = DMQP_RESPONSE;
            header.status_code = ENOSYS;
            struct dmqp_message response = {.header = header};

            send_dmqp_message(client, &response, 0);
            break;
        }

        if (buf.payload) {
            free(buf.payload);
        }
    }

    close(client);
    return NULL;
}

/**
 * Creates a detached thread to handle accepted TCP clients.
 *
 * @param socket client socket
 */
static void connection_thread_init(int socket) {
    int *args = malloc(sizeof(int));
    *args = socket;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, connection_handler, args);
    pthread_attr_destroy(&attr);
}

int dmqp_server_init(unsigned short port) {
    int ret = 0;
    signal_init();

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        errno = EIO;
        return -1;
    }

    int opt = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) < 0) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    if (setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt) < 0) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    socklen_t address_len = sizeof address;

    if (bind(server, (struct sockaddr *)&address, address_len) < 0) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    if (listen(server, LISTEN_BACKLOG) < 0) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    if (getsockname(server, (struct sockaddr *)&address, &address_len) < 0) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    pthread_mutex_lock(&server_lock);
    server_running = 1;
    server_port = ntohs(address.sin_port);
    pthread_mutex_unlock(&server_lock);
    pthread_cond_broadcast(&server_running_cond);

    printf("DMQP Server listening on port %d\n", server_port);

    for (;;) {
        pthread_mutex_lock(&server_lock);
        int running = server_running;
        pthread_mutex_unlock(&server_lock);

        if (!running) {
            break;
        }

        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof client_address;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server, &readfds);
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        int ready = select(server + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR) {
                errno = 0;
                continue;
            }

            break;
        }

        if (!ready || !FD_ISSET(server, &readfds)) {
            continue;
        }

        int client = accept(server, (struct sockaddr *)&client_address,
                            &client_address_len);
        if (client < 0) {
            if (errno == EINTR) {
                errno = 0;
                continue;
            }

            break;
        }

        // 30s timeout on reads/writes
        tv.tv_sec = 30;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof tv);

        // tcp keepalive
        int keepalive = 1;
        setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
                   sizeof keepalive);

        connection_thread_init(client);
    }

cleanup:
    close(server);
    return ret;
}

/**
 * Reads all bytes into a buffer from a file descriptor. Operation will return
 * once all `count` bytes are sent.
 *
 * @param fd file descriptor to read from
 * @param buf buffer to write to
 * @param count number of bytes to read
 * @returns 0 on success, -1 on error with global `errno` set
 */
static int read_all(int fd, void *buf, size_t count) {
    unsigned int total = 0;

    while (total < count) {
        int n = read(fd, (char *)buf + total, count - total);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        total += n;
    }

    return 0;
}

/**
 * Reads a DMQP header from a file descriptor. Converts header fields to host
 * byte order.
 *
 * @param fd file descriptor to read from
 * @param buf DMQP header buffer to write to
 * @returns 0 on success, -1 if error with global `errno` set
 * @throws `EIO` unexpected error
 */
static int read_dmqp_header(int fd, struct dmqp_header *buf) {
    char header_wire_buf[DMQP_HEADER_SIZE];
    if (read_all(fd, header_wire_buf, DMQP_HEADER_SIZE) < 0) {
        errno = EIO;
        return -1;
    }

    memcpy(&buf->sequence_id, header_wire_buf, 4);
    memcpy(&buf->length, header_wire_buf + 4, 4);
    memcpy(&buf->method, header_wire_buf + 8, 2);
    memcpy(&buf->status_code, header_wire_buf + 10, 2);

    buf->sequence_id = ntohl(buf->sequence_id);
    buf->length = ntohl(buf->length);
    buf->method = ntohs(buf->method);
    buf->status_code = ntohs(buf->status_code);
    return 0;
}

int read_dmqp_message(int fd, struct dmqp_message *buf) {
    if (fd < 0 || !buf) {
        errno = EINVAL;
        return -1;
    }

    if (read_dmqp_header(fd, &buf->header) < 0) {
        errno = EIO;
        return -1;
    }

    if (buf->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    if (buf->header.length == 0) {
        buf->payload = NULL;
        return 0;
    }

    buf->payload = malloc(buf->header.length);
    if (read_all(fd, buf->payload, buf->header.length) < 0) {
        free(buf->payload);
        errno = EIO;
        return -1;
    }

    return 0;
}

/**
 * Sends all bytes from a buffer to a socket. Operation will return once all
 * `length` bytes are sent.
 *
 * @param socket socket to send to
 * @param buffer buffer to send
 * @param length number of bytes to read
 * @param flags same flags param as that of `send` syscall
 * @returns 0 on success, -1 on error with global `errno` set
 */
static int send_all(int socket, const void *buffer, size_t length, int flags) {
    unsigned int total = 0;

    while (total < length) {
        int n = send(socket, (char *)buffer + total, length - total, flags);
        if (n <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        total += n;
    }

    return 0;
}

/**
 * Sends a DMQP header to a socket. Converts header fields to network byte
 * order (big endian).
 *
 * @param socket socket to write to
 * @param buf DMQP header buffer to send
 * @param flags same flags param as `send` syscall
 * @returns 0 on success, -1 if error with global `errno` set
 * @throws `EIO` unexpected error
 */
static int send_dmqp_header(int socket, const struct dmqp_header *buffer,
                            int flags) {
    uint32_t network_byte_ordered_sequence_id = htonl(buffer->sequence_id);
    uint32_t network_byte_ordered_length = htonl(buffer->length);
    uint16_t network_byte_ordered_method = htons(buffer->method);
    int16_t network_byte_ordered_status_code = htons(buffer->status_code);

    char header_wire_buf[DMQP_HEADER_SIZE];
    memcpy(header_wire_buf, &network_byte_ordered_sequence_id, 4);
    memcpy(header_wire_buf + 4, &network_byte_ordered_length, 4);
    memcpy(header_wire_buf + 8, &network_byte_ordered_method, 2);
    memcpy(header_wire_buf + 10, &network_byte_ordered_status_code, 2);

    if (send_all(socket, header_wire_buf, DMQP_HEADER_SIZE, flags) < 0) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int send_dmqp_message(int fd, const struct dmqp_message *buffer, int flags) {
    if (fd < 0 || !buffer ||
        (buffer->header.length > 0 && buffer->payload == NULL)) {
        errno = EINVAL;
        return -1;
    }

    if (buffer->header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    if (send_dmqp_header(fd, &buffer->header, flags) < 0) {
        errno = EIO;
        return -1;
    }

    if (buffer->header.length == 0) {
        return 0;
    }

    if (send_all(fd, buffer->payload, buffer->header.length, flags) < 0) {
        errno = EIO;
        return -1;
    }

    return 0;
}

__attribute__((weak)) void handle_dmqp_push(const struct dmqp_message *message,
                                            int client) {
    (void)message;
    (void)client;
}

__attribute__((weak)) void handle_dmqp_pop(const struct dmqp_message *message,
                                           int client) {
    (void)message;
    (void)client;
}

__attribute__((weak)) void
handle_dmqp_peek_sequence_id(const struct dmqp_message *message, int client) {
    (void)message;
    (void)client;
}

__attribute__((weak)) void
handle_dmqp_response(const struct dmqp_message *message, int client) {
    (void)message;
    (void)client;
}
