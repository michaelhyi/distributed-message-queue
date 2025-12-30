#include "network.h"
#include "test.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "util.h"

struct targs {
    unsigned short port;
    int result;
    int _errno;
};

static void *start_test_dmqp_server(void *arg) {
    struct targs *args = (struct targs *)arg;
    args->result = dmqp_server_init(args->port);
    args->_errno = errno;
    return NULL;
}

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

int test_dmqp_client_init_throws_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(dmqp_client_init(NULL, 8080) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_dmqp_client_init_throws_when_server_does_not_exist() {
    // arrange
    errno = 0;

    // act & assert
    assert(dmqp_client_init("127.0.0.1", 8080) < 0);
    assert(errno == EIO);
    return 0;
}

int test_dmqp_client_init_success() {
    // arrange
    errno = 0;
    struct targs args = {.port = 8080};
    pthread_t tid;
    pthread_create(&tid, NULL, start_test_dmqp_server, &args);
    sleep(1); // wait 1s for server to initialize

    // act
    int client = dmqp_client_init("127.0.0.1", 8080);

    // assert
    assert(client >= 0);
    assert(errno == 0);

    // teardown
    pthread_kill(tid, SIGTERM);
    pthread_join(tid, NULL);
    close(client);
    return 0;
}

int test_dmqp_server_init_handles_signals() {
    // arrange
    errno = 0;
    int signals[] = {SIGTERM, SIGINT};

    for (int i = 0; i < arrlen(signals); i++) {
        struct targs args = {.port = 8081 + i};
        pthread_t tid;
        pthread_create(&tid, NULL, start_test_dmqp_server, &args);
        sleep(1); // wait 1s for server to initialize

        // act
        pthread_kill(tid, signals[i]);
        pthread_join(tid, NULL);

        // assert
        assert(args.result >= 0);
        assert(!args._errno);
    }

    return 0;
}

int test_read_dmqp_message_throws_when_invalid_args() {
    // arrange
    struct dmqp_message buf;

    struct {
        int fd;
        struct dmqp_message *buf;
    } cases[] = {
        {-1, NULL},
        {0, NULL},
        {-1, &buf},
    };

    for (int i = 0; i < arrlen(cases); i++) {
        // arrange
        errno = 0;

        // act & assert
        assert(read_dmqp_message(cases[i].fd, cases[i].buf) < 0);
        assert(errno == EINVAL);
    }

    return 0;
}

int test_read_dmqp_message_throws_when_payload_too_large() {
    // arrange
    errno = 0;

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = 0,
    //                              .length = htonl(1 * MB + 1),
    //                              .method = 0,
    //                              .status_code = 0};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof(header_wire));

    uint32_t length = htonl(MAX_PAYLOAD_LENGTH + 1);
    memcpy(header_wire + 4, &length, 4);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);
    close(fds[1]);

    struct dmqp_message buf;

    // act & assert
    assert(read_dmqp_message(fds[0], &buf) < 0);
    assert(errno == EMSGSIZE);

    // teardown
    close(fds[0]);
    return 0;
}

int test_read_dmqp_message_success_when_no_payload() {
    // arrange
    errno = 0;

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = htonl(5),
    //                              .length = 0,
    //                              .method = htons(DMQP_RESPONSE),
    //                              .status_code = 0};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof header_wire);

    uint32_t sequence_id = htonl(5);
    memcpy(header_wire, &sequence_id, 4);
    header_wire[8] = htons(DMQP_RESPONSE);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act & assert
    assert(read_dmqp_message(fds[0], &buf) >= 0);
    assert(!errno);
    assert(buf.header.sequence_id == 5);
    assert(buf.header.length == 0);
    assert(buf.header.method == DMQP_PUSH);
    assert(buf.header.status_code == 0);
    assert(buf.payload == NULL);

    // teardown
    close(fds[0]);
    return 0;
}

int test_read_dmqp_message_success_with_payload() {
    // arrange
    errno = 0;

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = htonl(5),
    //                              .length = htonl(13),
    //                              .method = htons(DMQP_RESPONSE),
    //                              .status_code = htons(3)};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof(header_wire));
    uint32_t sequence_id = htonl(5);
    uint32_t length = htonl(13);
    uint16_t method = htons(DMQP_RESPONSE);
    int16_t status_code = htons(3);

    memcpy(header_wire, &sequence_id, 4);
    memcpy(header_wire + 4, &length, 4);
    memcpy(header_wire + 8, &method, 2);
    memcpy(header_wire + 10, &status_code, 2);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);

    char *payload = "Hello, World!";
    send_all(fds[1], payload, 13, 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act & assert
    assert(read_dmqp_message(fds[0], &buf) >= 0);
    assert(!errno);
    assert(buf.header.sequence_id == 5);
    assert(buf.header.length == 13);
    assert(buf.header.method == DMQP_RESPONSE);
    assert(buf.header.status_code == 3);
    assert(memcmp(buf.payload, payload, 13) == 0);

    // teardown
    free(buf.payload);
    close(fds[0]);
    return 0;
}

int test_send_dmqp_message_throws_when_invalid_args() {
    // arrange
    errno = 0;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct dmqp_header header = {.length = 5};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    struct {
        int socket;
        const struct dmqp_message *buffer;
        int flags;
    } cases[] = {{-1, NULL, 0}, {-1, &buf, 0}, {fd, NULL, 0}, {fd, &buf, 0}};

    for (int i = 0; i < arrlen(cases); i++) {
        // arrange
        errno = 0;

        // act & assert
        assert(send_dmqp_message(cases[i].socket, cases[i].buffer,
                                 cases[i].flags) < 0);
        assert(errno == EINVAL);

        // assert that `send_dmqp_message` didn't send anything
        assert(recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);
    }

    // teardown
    close(fd);
    return 0;
}

int test_send_dmqp_message_throws_when_payload_too_large() {
    // arrange
    errno = 0;

    struct dmqp_header header = {.sequence_id = 0,
                                 .length = MAX_PAYLOAD_LENGTH + 1,
                                 .method = 0,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = "Hello, World!"};
    char header_wire_buf[DMQP_HEADER_SIZE];

    // act & assert
    assert(send_dmqp_message(fds[1], &buf, 0) < 0);
    close(fds[1]);
    assert(errno == EMSGSIZE);

    // assert that `send_dmqp_message` didn't send anything
    assert(recv(fds[0], header_wire_buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // teardown
    close(fds[0]);
    return 0;
}

int test_send_dmqp_message_success_when_no_payload() {
    // arrange
    errno = 0;

    struct dmqp_header header = {.sequence_id = 5,
                                 .length = 0,
                                 .method = DMQP_RESPONSE,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = NULL};
    char header_wire_buf[DMQP_HEADER_SIZE];

    uint32_t expected_sequence_id = htonl(5);
    uint32_t expected_length = 0;
    uint16_t expected_method = htons(DMQP_RESPONSE);
    int16_t expected_status_code = 0;

    // act & assert
    assert(send_dmqp_message(fds[1], &buf, 0) >= 0);
    assert(!errno);
    read_all(fds[0], header_wire_buf, DMQP_HEADER_SIZE);

    // assert that `send_dmqp_message` didn't send a payload
    assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    assert(memcmp(header_wire_buf, &expected_sequence_id, 4) == 0);
    assert(memcmp(header_wire_buf + 4, &expected_length, 4) == 0);
    assert(memcmp(header_wire_buf + 8, &expected_method, 2) == 0);
    assert(memcmp(header_wire_buf + 10, &expected_status_code, 2) == 0);

    // teardown
    close(fds[0]);
    close(fds[1]);
    return 0;
}

int test_send_dmqp_message_success_with_payload() {
    // arrange
    errno = 0;

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    char payload[] = "Hello, World!";
    struct dmqp_header header = {.sequence_id = 5,
                                 .length = 13,
                                 .method = DMQP_RESPONSE,
                                 .status_code = 3};
    struct dmqp_message buf = {.header = header, .payload = payload};
    char header_wire_buf[DMQP_HEADER_SIZE];

    uint32_t expected_sequence_id = htonl(5);
    uint32_t expected_length = htonl(13);
    uint16_t expected_method = htons(DMQP_RESPONSE);
    int16_t expected_status_code = htons(3);

    // act
    assert(send_dmqp_message(fds[1], &buf, 0) >= 0);
    assert(!errno);
    close(fds[1]);

    read_all(fds[0], header_wire_buf, DMQP_HEADER_SIZE);
    read_all(fds[0], buf.payload, 13);

    // assert
    assert(memcmp(header_wire_buf, &expected_sequence_id, 4) == 0);
    assert(memcmp(header_wire_buf + 4, &expected_length, 4) == 0);
    assert(memcmp(header_wire_buf + 8, &expected_method, 2) == 0);
    assert(memcmp(header_wire_buf + 10, &expected_status_code, 2) == 0);
    assert(memcmp(buf.payload, "Hello, World!", 13) == 0);

    // assert that `send_dmqp_message` didn't send anything else
    assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // teardown
    close(fds[0]);
    return 0;
}

int test_dmqp_server_init_handles_message_with_unknown_method() {
    // arrange
    errno = 0;
    struct targs args = {.port = 8083};
    pthread_t tid;
    pthread_create(&tid, NULL, start_test_dmqp_server, &args);
    sleep(1); // wait 1s for server to initialize

    int client = dmqp_client_init("127.0.0.1", 8083);

    struct dmqp_header header = {0};
    header.method = DMQP_RESPONSE + 1;
    struct dmqp_message message = {.header = header, .payload = NULL};

    // act & assert
    assert(send_dmqp_message(client, &message, 0) >= 0);
    assert(!errno);
    assert(read_dmqp_message(client, &message) >= 0);
    assert(!errno);
    assert(message.header.sequence_id == 0);
    assert(message.header.length == 0);
    assert(message.header.method == DMQP_RESPONSE);
    assert(message.header.status_code == ENOSYS);

    // teardown
    pthread_kill(tid, SIGTERM);
    pthread_join(tid, NULL);
    close(client);
    return 0;
}

static struct test test_cases[] = {
    {NULL, NULL, test_dmqp_client_init_throws_when_invalid_args},
    {NULL, NULL, test_dmqp_client_init_throws_when_server_does_not_exist},
    {NULL, NULL, test_dmqp_client_init_success},
    {NULL, NULL, test_dmqp_server_init_handles_signals},
    {NULL, NULL, test_read_dmqp_message_throws_when_invalid_args},
    {NULL, NULL, test_read_dmqp_message_throws_when_payload_too_large},
    {NULL, NULL, test_read_dmqp_message_success_when_no_payload},
    {NULL, NULL, test_read_dmqp_message_success_with_payload},
    {NULL, NULL, test_send_dmqp_message_throws_when_invalid_args},
    {NULL, NULL, test_send_dmqp_message_throws_when_payload_too_large},
    {NULL, NULL, test_send_dmqp_message_success_when_no_payload},
    {NULL, NULL, test_send_dmqp_message_success_with_payload},
    {NULL, NULL, test_dmqp_server_init_handles_message_with_unknown_method}};

int main() {
    unsigned int passed = 0;

    for (int i = 0; i < arrlen(test_cases); i++) {
        if (test_cases[i].setup) {
            test_cases[i].setup();
        }

        if (test_cases[i].test_case() >= 0) {
            passed++;
        }

        if (test_cases[i].teardown) {
            test_cases[i].teardown();
        }
    }

    printf("Successfully passed %d/%d tests\n!", passed, arrlen(test_cases));
    return 0;
}
