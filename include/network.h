#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <sys/types.h>

/**
 * TODO:
 * 1. docs
 * 2. errno
 * 3. clean up resources
 * 4. unit testing?
 * 5. timeouts
 * 6. signal handling
 * 7. heartbeats
 * 8. TLS
 * 9. TCP keepalive to replace heartbeats
 */

/**
 * Initializes a client connection to a server.
 *
 * @param server_host the server host address
 * @param server_port the server port
 * @returns the socket file descriptor of the client, -1 if error
 * with global `errno` set
 */
int client_init(const char *server_host, unsigned int server_port);

/**
 * Initializes a server.
 *
 * @param server_port the port to bind the server to
 * @param message_handler pointer to a function from the application-layer
 * protocol that handles messages. passed down to connection handler thread to
 * handle messages received at server
 * @returns 0 on success, -1 on error with global `errno` set
 */
int server_init(unsigned int server_port, int (*message_handler)(int socket));

/**
 * Reads all bytes into a buffer from a file descriptor.
 *
 * @param fd file descriptor to read from
 * @param buf buffer to write to
 * @param count number of bytes to read
 * @returns the number of bytes read if success, -1 if error with global `errno`
 * set
 */
ssize_t read_all(int fd, void *buf, size_t count);

/**
 * Sends all bytes from a buffer to a socket.
 *
 * @param socket socket to send to
 * @param buffer buffer to send
 * @param length number of bytes to read
 * @param flags same flags param as that of `send` syscall
 * @returns the number of bytes sent if success, -1 if error with global `errno`
 * set
 */
ssize_t send_all(int socket, const void *buffer, size_t length, int flags);

#endif
