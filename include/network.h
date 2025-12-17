#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <sys/types.h>

/**
 * TODO:
 * 1. unit testing?
 * 2. TLS
 */

/**
 * Initializes a client connection to a server.
 *
 * Throws an error if `server_host` is null or any network operations fail.
 *
 * @param server_host the server host address
 * @param server_port the server port
 * @returns the socket of the client, -1 if error with global `errno` set
 */
int client_init(const char *server_host, unsigned short server_port);

/**
 * Initializes a server and creates a new thread for each connection. Signals
 * are handled to gracefully exit.
 *
 * Throws an error if `message_handler` is null or any network operations fail.
 *
 * @param server_port the port to bind the server to
 * @param message_handler function from the application-layer protocol that
 * handles messages. passed down to connection handler thread to handle messages
 * received at server
 * @returns 0 on success, -1 on error with global `errno` set. does not return
 * until the server is interrupted or terminated
 */
int server_init(unsigned short server_port, int (*message_handler)(int socket));

/**
 * Reads all bytes into a buffer from a file descriptor.
 *
 * Throws an error if `buf` is null or the `read` syscall fails.
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
 * Throws an error if `socket` is invalid, `buffer` is null, or if then `send`
 * syscall fails.
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
