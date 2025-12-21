#include "dmqp.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

struct thread_arg {
    unsigned short port;
    int res;
    int _errno;
};

// Starts a DMQP server on a separate thread.
static void *start_test_dmqp_server(void *arg) {
    struct thread_arg *args = (struct thread_arg *)arg;
    args->res = dmqp_server_init(args->port);
    args->_errno = errno;
    return NULL;
}

TestSuite(integration_dmqp, .timeout = 10);

Test(integration_dmqp, test_dmqp_client_init_throws_when_no_server_found) {
    // arrange
    errno = 0;

    // act
    int res = dmqp_client_init("127.0.0.1", 8080);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ECONNREFUSED);
}

Test(integration_dmqp, test_dmqp_client_init_success) {
    // arrange
    errno = 0;
    int socket = -1;
    pthread_t thread_id;
    struct thread_arg arg = {.port = 8080, .res = -1, ._errno = -1};
    pthread_create(&thread_id, NULL, start_test_dmqp_server, &arg);

    // act
    for (int i = 0; i < 100; i++) {
        socket = dmqp_client_init("127.0.0.1", 8080);
        if (socket >= 0) {
            errno = 0;
            break;
        }

        usleep(10000);
    }

    // cleanup
    kill(getpid(), SIGTERM);
    pthread_join(thread_id, NULL);
    close(socket);

    // assert
    cr_assert(socket >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(arg.res, 0);
    cr_assert_eq(arg._errno, 0);
}

// TODO:
// Test(integration_dmqp, test_dmqp_server_init_success) {}
