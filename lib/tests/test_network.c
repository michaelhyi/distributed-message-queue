#include "network.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

struct thread_arg {
    int socket;
    unsigned int delay; // in microseconds
};

static void *slow_writer(void *arg) {
    struct thread_arg *args = (struct thread_arg *)arg;
    send_all(args->socket, "Hello, ", 7, 0);
    usleep(args->delay);
    send_all(args->socket, "World!", 6, 0);
    close(args->socket);
    return NULL;
}

#ifdef DEBUG
TestSuite(network);
#else
TestSuite(network, .timeout = 10);
#endif

Test(network, test_tcp_client_init_throws_when_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = tcp_client_init(NULL, 8080);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(network, test_tcp_server_init_throws_when_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = tcp_server_init(8080, NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(network, test_read_all_throws_when_invalid_args) {
    // arrange
    errno = 0;
    char buf[512];

    // act
    ssize_t n1 = read_all(-1, NULL, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    ssize_t n2 = read_all(-1, buf, 0);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    ssize_t n3 = read_all(0, NULL, 0);
    int errno3 = errno;

    // assert
    cr_assert(n1 < 0);
    cr_assert(n2 < 0);
    cr_assert(n3 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
    cr_assert_eq(errno3, EINVAL);
}

Test(network, test_read_all_success) {
    // arrange
    errno = 0;
    char buf[512];

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], "Hello, World!", 13, 0);
    close(fds[1]);

    // act
    ssize_t n = read_all(fds[0], buf, 13);

    // assert
    cr_assert_eq(n, 13);
    cr_assert_eq(errno, 0);
    cr_assert_arr_eq(buf, "Hello, World!", 13);

    // cleanup
    close(fds[0]);
}

Test(network, test_read_all_success_with_partial_read) {
    // arrange
    errno = 0;
    char buf[512];

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct thread_arg args = {.socket = fds[1], .delay = 100000};
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, slow_writer, &args);

    // act
    ssize_t n = read_all(fds[0], buf, 13);

    // cleanup
    pthread_join(thread_id, NULL);
    close(fds[0]);

    // assert
    cr_assert_eq(n, 13);
    cr_assert_eq(errno, 0);
    cr_assert_arr_eq(buf, "Hello, World!", 13);
}

Test(network, test_send_all_throws_when_invalid_args) {
    // arrange
    errno = 0;
    char buf[512];

    // act
    ssize_t n1 = send_all(-1, NULL, 0, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    ssize_t n2 = send_all(-1, buf, 0, 0);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    ssize_t n3 = send_all(0, NULL, 0, 0);
    int errno3 = errno;

    // assert
    cr_assert(n1 < 0);
    cr_assert(n2 < 0);
    cr_assert(n3 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
    cr_assert_eq(errno3, EINVAL);
}

Test(network, test_send_all_success) {
    // arrange
    errno = 0;
    char buf[512];
    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    ssize_t n = send_all(fds[1], "Hello, World!", 13, 0);
    close(fds[1]);
    read_all(fds[0], buf, 13);

    // assert
    cr_assert_eq(n, 13);
    cr_assert_eq(errno, 0);
    cr_assert_arr_eq(buf, "Hello, World!", 13);

    // cleanup
    close(fds[0]);
}
