#include "network.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

struct thread_arg {
    const char *host;
    unsigned short port;
    int res;
    int _errno;
};

// Imeplemented just to call server_init
int message_handler(int socket) {
    (void)socket; // unused
    return 0;
}

// Starts a server on a separate thread.
static void *start_test_server(void *arg) {
    struct thread_arg *args = (struct thread_arg *)arg;
    args->res = server_init(args->port, message_handler);
    args->_errno = errno;
    return NULL;
}

#ifdef DEBUG
TestSuite(integration_network);
#else
TestSuite(integration_network, .timeout = 10);
#endif

Test(integration_network, test_client_init_success) {
    // arrange
    errno = 0;
    int socket = -1;
    pthread_t thread_id;
    struct thread_arg arg = {.port = 8080, .res = -1, ._errno = -1};
    pthread_create(&thread_id, NULL, start_test_server, &arg);

    // act
    for (int i = 0; i < 100; i++) {
        socket = client_init("127.0.0.1", 8080);
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

// TODO: fix these commented tests

// Test(integration_network, test_server_init_graceful_termination_on_sigint) {
//     // arrange
//     errno = 0;
//     int socket = -1;
//     pthread_t thread_id;
//     struct thread_arg arg = {.port = 8080, .res = -1, ._errno = -1};
//     pthread_create(&thread_id, NULL, start_test_server, &arg);
//     usleep(100000);

//     // act
//     kill(getpid(), SIGINT);

//     // cleanup
//     pthread_join(thread_id, NULL);
//     close(socket);

//     // assert
//     cr_assert(socket >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_eq(arg.res, 0);
//     cr_assert_eq(arg._errno, 0);
// }

// Test(integration_network, test_server_init_graceful_termination_on_sigterm) {
//     // arrange
//     errno = 0;
//     int socket = -1;
//     pthread_t thread_id;
//     struct thread_arg arg = {.port = 8080, .res = -1, ._errno = -1};
//     pthread_create(&thread_id, NULL, start_test_server, &arg);
//     usleep(100000);

//     // act
//     kill(getpid(), SIGTERM);

//     // cleanup
//     pthread_join(thread_id, NULL);
//     close(socket);

//     // assert
//     cr_assert(socket >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_eq(arg.res, 0);
//     cr_assert_eq(arg._errno, 0);
// }

// Test(network, test_server_init_ignores_sigpipe) {

// }

// Test(network, test_read_all_success_despite_interrupt);
// Test(network, test_read_all_partial_read_on_disconnect);
// Test(network, test_read_all_partial_read_on_incomplete_data);
// Test(network, test_send_all_success_with_partial_write);
// Test(network, test_send_all_success_despite_interrupt);
