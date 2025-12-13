#include "dmqp.h"

#include <arpa/inet.h>
#include <criterion/criterion.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "network.h"

#if defined(__linux__)
#include <endian.h>
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/endian.h>
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)
#else // Fallback implementation
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ntohll(x)                                                              \
    ((((uint64_t)ntohl((uint32_t)(x))) << 32) | ntohl((uint32_t)((x) >> 32)))
#define htonll(x)                                                              \
    ((((uint64_t)htonl((uint32_t)(x))) << 32) | htonl((uint32_t)((x) >> 32)))
#else
#define ntohll(x) (x)
#define htonll(x) (x)
#endif
#endif

// used to validate propagation to downstream handlers
static unsigned int dmqp_response_count = 0;
static unsigned int dmqp_heartbeat_count = 0;
static unsigned int dmqp_push_count = 0;
static unsigned int dmqp_pop_count = 0;
static unsigned int dmqp_peek_count = 0;
static unsigned int dmqp_unknown_method_count = 0;

int handle_dmqp_response(const struct dmqp_message *message, int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_response_count++;
    return 0;
}

int handle_dmqp_heartbeat(const struct dmqp_message *message,
                          int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_heartbeat_count++;
    return 0;
}

int handle_dmqp_push(const struct dmqp_message *message, int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_push_count++;
    return 0;
}

int handle_dmqp_pop(const struct dmqp_message *message, int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_pop_count++;
    return 0;
}

int handle_dmqp_peek(const struct dmqp_message *message, int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_peek_count++;
    return 0;
}

int handle_dmqp_unknown_method(const struct dmqp_message *message,
                               int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_unknown_method_count++;
    return 0;
}

TestSuite(dmqp, .timeout = 10);

Test(dmqp, test_read_dmqp_message_throws_when_invalid_args) {
    // arrange
    errno = 0;
    int fd = 0;
    struct dmqp_message buf;

    // act
    int res1 = read_dmqp_message(-1, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = read_dmqp_message(-1, &buf);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = read_dmqp_message(fd, NULL);
    int errno3 = errno;

    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
    cr_assert_eq(errno3, EINVAL);
}

Test(dmqp, test_read_dmqp_message_throws_when_payload_too_big) {
    // arrange
    errno = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = htonl(1 * MB + 1),
                                 .method = 0,
                                 .topic_id = 0,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send(fds[1], &header, sizeof(struct dmqp_header), 0);
    close(fds[1]);

    struct dmqp_message buf;

    // act
    int res = read_dmqp_message(fds[0], &buf);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EMSGSIZE);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_read_dmqp_message_success_when_no_payload) {
    // arrange
    errno = 0;

    // no need to convert method and topic_id, since they're only a byte
    struct dmqp_header header = {.timestamp = htonll(5),
                                 .length = 0,
                                 .method = DMQP_PUSH,
                                 .topic_id = 3,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send(fds[1], &header, sizeof(struct dmqp_header), 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act
    int res = read_dmqp_message(fds[0], &buf);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(buf.header.timestamp, 5);
    cr_assert_eq(buf.header.length, 0);
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.topic_id, 3);
    cr_assert_eq(buf.header.status_code, 0);
    cr_assert_null(buf.payload);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_read_dmqp_message_success) {
    // arrange
    errno = 0;

    // no need to convert method and topic_id, since they're only a byte
    struct dmqp_header header = {.timestamp = htonll(5),
                                 .length = htonl(13),
                                 .method = DMQP_PUSH,
                                 .topic_id = 3,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send(fds[1], &header, sizeof(struct dmqp_header), 0);
    send(fds[1], "Hello, World!", 13, 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act
    int res = read_dmqp_message(fds[0], &buf);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(buf.header.timestamp, 5);
    cr_assert_eq(buf.header.length, 13);
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.topic_id, 3);
    cr_assert_eq(buf.header.status_code, 0);
    cr_assert_arr_eq(buf.payload, "Hello, World!", 13);

    // cleanup
    free(buf.payload);
    close(fds[0]);
}

Test(dmqp, test_send_dmqp_message_throws_when_invalid_args) {
    // arrange
    errno = 0;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct dmqp_header header = {.length = 5};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    // act
    int res1 = send_dmqp_message(-1, NULL, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = send_dmqp_message(-1, &buf, 0);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = send_dmqp_message(fd, NULL, 0);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = send_dmqp_message(fd, &buf, 0);
    int errno4 = errno;

    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert(res4 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
    cr_assert_eq(errno3, EINVAL);
    cr_assert_eq(errno4, EINVAL);

    // assert that `send_dmqp_message` didn't send anything
    cr_assert(recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);
}

Test(dmqp, test_send_dmqp_message_throws_when_payload_too_big) {
    // arrange
    errno = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 1 * MB + 1,
                                 .method = 0,
                                 .topic_id = 0,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = "Hello, World!"};

    // act
    int res = send_dmqp_message(fds[1], &buf, 0);
    close(fds[1]);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EMSGSIZE);

    // assert that `send_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_send_dmqp_message_success_when_no_payload) {
    // arrange
    errno = 0;

    struct dmqp_header header = {.timestamp = 5,
                                 .length = 0,
                                 .method = DMQP_PUSH,
                                 .topic_id = 3,
                                 .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = NULL};

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = read_stream(fds[0], &buf.header, sizeof(struct dmqp_header));
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);

    // assert that `send_dmqp_message` didn't send a payload
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // no need to convert method and topic_id, since they're only a byte
    cr_assert_eq(buf.header.timestamp, htonll(5));
    cr_assert_eq(buf.header.length, 0);
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.topic_id, 3);
    cr_assert_eq(buf.header.status_code, 0);
    cr_assert_null(buf.payload);

    // cleanup
    close(fds[0]);
    close(fds[1]);
}

Test(dmqp, test_send_dmqp_message_success) {
    // arrange
    errno = 0;

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    char payload[] = "Hello, World!";
    struct dmqp_header header = {.timestamp = 5,
                                 .length = 13,
                                 .method = DMQP_PUSH,
                                 .topic_id = 3,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = payload};

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = read_stream(fds[0], &buf.header, sizeof(struct dmqp_header));
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = read_stream(fds[0], buf.payload, 13);
    int errno3 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert(res3 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(errno3, 0);

    // no need to convert method and topic_id, since they're only a byte
    cr_assert_eq(buf.header.timestamp, htonll(5));
    cr_assert_eq(buf.header.length, htonl(13));
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.topic_id, 3);
    cr_assert_eq(buf.header.status_code, 0);
    cr_assert_arr_eq(buf.payload, "Hello, World!", 13);

    // assert that `send_dmqp_message` didn't send anything else
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_throws_error_when_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = handle_dmqp_message(-1);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_response) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 1);
    cr_assert_eq(dmqp_heartbeat_count, 0);
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_count, 0);
    cr_assert_eq(dmqp_unknown_method_count, 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_heartbeat) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 0,
                                 .method = DMQP_HEARTBEAT,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    close(fds[1]);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 0);
    cr_assert_eq(dmqp_heartbeat_count, 1);
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_count, 0);
    cr_assert_eq(dmqp_unknown_method_count, 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_push) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 13,
                                 .method = DMQP_PUSH,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = "Hello, World!"};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 0);
    cr_assert_eq(dmqp_heartbeat_count, 0);
    cr_assert_eq(dmqp_push_count, 1);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_count, 0);
    cr_assert_eq(dmqp_unknown_method_count, 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_pop) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 0,
                                 .method = DMQP_POP,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 0);
    cr_assert_eq(dmqp_heartbeat_count, 0);
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 1);
    cr_assert_eq(dmqp_peek_count, 0);
    cr_assert_eq(dmqp_unknown_method_count, 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_peek) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 0,
                                 .method = DMQP_PEEK,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 0);
    cr_assert_eq(dmqp_heartbeat_count, 0);
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_count, 1);
    cr_assert_eq(dmqp_unknown_method_count, 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_unknown) {
    // arrange
    errno = 0;
    dmqp_response_count = 0;
    dmqp_heartbeat_count = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_count = 0;
    dmqp_unknown_method_count = 0;

    struct dmqp_header header = {.timestamp = 0,
                                 .length = 0,
                                 .method = DMQP_PEEK + 1,
                                 .topic_id = 0,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_message(fds[0]);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(dmqp_response_count, 0);
    cr_assert_eq(dmqp_heartbeat_count, 0);
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_count, 0);
    cr_assert_eq(dmqp_unknown_method_count, 1);

    // cleanup
    close(fds[0]);
}
