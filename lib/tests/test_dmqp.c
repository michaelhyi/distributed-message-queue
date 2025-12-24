#include "dmqp.h"

#include <arpa/inet.h>
#include <criterion/criterion.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "network.h"

// used to validate propagation to downstream handlers
static unsigned int dmqp_push_count = 0;
static unsigned int dmqp_pop_count = 0;
static unsigned int dmqp_peek_sequence_id_count = 0;
static unsigned int dmqp_response_count = 0;

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

int handle_dmqp_peek_sequence_id(const struct dmqp_message *message,
                                 int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_peek_sequence_id_count++;
    return 0;
}

int handle_dmqp_response(const struct dmqp_message *message, int reply_socket) {
    (void)message;
    (void)reply_socket;
    dmqp_response_count++;
    return 0;
}

// TODO: endinaness on DMQP_PUSH is weak, since DMQP_PUSH is 0
#ifdef DEBUG
TestSuite(dmqp);
#else
TestSuite(dmqp, .timeout = 10);
#endif

Test(dmqp, test_dmqp_client_init_throws_when_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = dmqp_client_init(NULL, 8080);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);
}

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

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = 0,
    //                              .length = htonl(1 * MB + 1),
    //                              .method = 0,
    //                              .status_code = 0};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof(header_wire));

    uint32_t length = htonl(1 * MB + 1);
    memcpy(header_wire + 4, &length, 4);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);
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

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = htonl(5),
    //                              .length = 0,
    //                              .method = htons(DMQP_PUSH),
    //                              .status_code = 0};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof(header_wire));

    uint32_t sequence_id = htonl(5);
    memcpy(header_wire, &sequence_id, 4);
    header_wire[8] = htons(DMQP_PUSH);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act
    int res = read_dmqp_message(fds[0], &buf);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(buf.header.sequence_id, 5);
    cr_assert_eq(buf.header.length, 0);
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.status_code, 0);
    cr_assert_null(buf.payload);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_read_dmqp_message_success) {
    // arrange
    errno = 0;

    // below is the wire format for
    // struct dmqp_header header = {.sequence_id = htonl(5),
    //                              .length = htonl(13),
    //                              .method = htons(DMQP_PUSH),
    //                              .status_code = htons(3)};
    char header_wire[DMQP_HEADER_SIZE];
    memset(header_wire, 0, sizeof(header_wire));
    uint32_t sequence_id = htonl(5);
    uint32_t length = htonl(13);
    uint16_t method = htons(DMQP_PUSH);
    int16_t status_code = htons(3);

    memcpy(header_wire, &sequence_id, 4);
    memcpy(header_wire + 4, &length, 4);
    memcpy(header_wire + 8, &method, 2);
    memcpy(header_wire + 10, &status_code, 2);

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    send_all(fds[1], header_wire, DMQP_HEADER_SIZE, 0);
    send_all(fds[1], "Hello, World!", 13, 0);
    close(fds[1]);

    struct dmqp_message buf = {.payload = NULL};

    // act
    ssize_t n = read_dmqp_message(fds[0], &buf);

    // assert
    cr_assert(n >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(buf.header.sequence_id, 5);
    cr_assert_eq(buf.header.length, 13);
    cr_assert_eq(buf.header.method, DMQP_PUSH);
    cr_assert_eq(buf.header.status_code, 3);
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

    // cleanup
    close(fd);
}

Test(dmqp, test_send_dmqp_message_throws_when_payload_too_big) {
    // arrange
    errno = 0;

    struct dmqp_header header = {
        .sequence_id = 0, .length = 1 * MB + 1, .method = 0, .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = "Hello, World!"};
    char header_wire_buf[DMQP_HEADER_SIZE];

    // act
    // TODO: get rid of all ssize_t operations
    ssize_t n = send_dmqp_message(fds[1], &buf, 0);
    close(fds[1]);

    // assert
    cr_assert(n < 0);
    cr_assert_eq(errno, EMSGSIZE);

    // assert that `send_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], header_wire_buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_send_dmqp_message_success_when_no_payload) {
    // arrange
    errno = 0;

    struct dmqp_header header = {
        .sequence_id = 5, .length = 0, .method = DMQP_PUSH, .status_code = 0};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    struct dmqp_message buf = {.header = header, .payload = NULL};
    char header_wire_buf[DMQP_HEADER_SIZE];

    uint32_t expected_sequence_id = htonl(5);
    uint32_t expected_length = 0;
    uint16_t expected_method = htons(DMQP_PUSH);
    int16_t expected_status_code = 0;

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = read_all(fds[0], header_wire_buf, DMQP_HEADER_SIZE);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);

    // assert that `send_dmqp_message` didn't send a payload
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    cr_assert_arr_eq(header_wire_buf, &expected_sequence_id, 4);
    cr_assert_arr_eq(header_wire_buf + 4, &expected_length, 4);
    cr_assert_arr_eq(header_wire_buf + 8, &expected_method, 2);
    cr_assert_arr_eq(header_wire_buf + 10, &expected_status_code, 2);

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
    struct dmqp_header header = {
        .sequence_id = 5, .length = 13, .method = DMQP_PUSH, .status_code = 3};
    struct dmqp_message buf = {.header = header, .payload = payload};
    char header_wire_buf[DMQP_HEADER_SIZE];

    uint32_t expected_sequence_id = htonl(5);
    uint32_t expected_length = htonl(13);
    uint16_t expected_method = htons(DMQP_PUSH);
    int16_t expected_status_code = htons(3);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
    int errno1 = errno;
    close(fds[1]);

    // arrange
    errno = 0;

    // act
    int res2 = read_all(fds[0], header_wire_buf, DMQP_HEADER_SIZE);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = read_all(fds[0], buf.payload, 13);
    int errno3 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert(res3 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);
    cr_assert_eq(errno3, 0);

    cr_assert_arr_eq(header_wire_buf, &expected_sequence_id, 4);
    cr_assert_arr_eq(header_wire_buf + 4, &expected_length, 4);
    cr_assert_arr_eq(header_wire_buf + 8, &expected_method, 2);
    cr_assert_arr_eq(header_wire_buf + 10, &expected_status_code, 2);
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

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_push) {
    // arrange
    errno = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_sequence_id_count = 0;
    dmqp_response_count = 0;

    struct dmqp_header header = {
        .sequence_id = 0, .length = 13, .method = DMQP_PUSH, .status_code = 0};
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
    cr_assert_eq(dmqp_push_count, 1);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_sequence_id_count, 0);
    cr_assert_eq(dmqp_response_count, 0);

    // assert that `handle_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_pop) {
    // arrange
    errno = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_sequence_id_count = 0;
    dmqp_response_count = 0;

    struct dmqp_header header = {
        .sequence_id = 0, .length = 0, .method = DMQP_POP, .status_code = 0};
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
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 1);
    cr_assert_eq(dmqp_peek_sequence_id_count, 0);
    cr_assert_eq(dmqp_response_count, 0);

    // assert that `handle_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp,
     test_handle_dmqp_message_success_when_method_is_dmqp_peek_sequence_id) {
    // arrange
    errno = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_sequence_id_count = 0;
    dmqp_response_count = 0;

    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_PEEK_SEQUENCE_ID,
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
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_sequence_id_count, 1);
    cr_assert_eq(dmqp_response_count, 0);

    // assert that `handle_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_dmqp_response) {
    // arrange
    errno = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_sequence_id_count = 0;
    dmqp_response_count = 0;

    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE,
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
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_sequence_id_count, 0);
    cr_assert_eq(dmqp_response_count, 1);

    // assert that `handle_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
}

Test(dmqp, test_handle_dmqp_message_success_when_method_is_unknown) {
    // arrange
    errno = 0;
    dmqp_push_count = 0;
    dmqp_pop_count = 0;
    dmqp_peek_sequence_id_count = 0;
    dmqp_response_count = 0;

    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE + 1,
                                 .status_code = 0};
    struct dmqp_message buf = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = send_dmqp_message(fds[1], &buf, 0);
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
    cr_assert_eq(dmqp_push_count, 0);
    cr_assert_eq(dmqp_pop_count, 0);
    cr_assert_eq(dmqp_peek_sequence_id_count, 0);
    cr_assert_eq(dmqp_response_count, 0);

    // TODO: this should fail
    // assert that `handle_dmqp_message` didn't send anything
    cr_assert(recv(fds[0], &buf, 1, MSG_PEEK | MSG_DONTWAIT) <= 0);

    // cleanup
    close(fds[0]);
    close(fds[1]);
}

Test(dmqp, test_handle_dmqp_unknown_method_throws_when_invalid_args) {
    // arrange
    errno = 0;
    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE + 1,
                                 .status_code = 0};
    struct dmqp_message message = {.header = header, .payload = NULL};
    int reply_socket = socket(AF_INET, SOCK_STREAM, 0);

    // act
    int res1 = handle_dmqp_unknown_method(NULL, -1);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = handle_dmqp_unknown_method(NULL, 0);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = handle_dmqp_unknown_method(NULL, reply_socket);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = handle_dmqp_unknown_method(&message, -1);
    int errno4 = errno;

    // arrange
    errno = 0;

    // act
    int res5 = handle_dmqp_unknown_method(&message, 0);
    int errno5 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert(res4 < 0);
    cr_assert(res5 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
    cr_assert_eq(errno3, EINVAL);
    cr_assert_eq(errno4, ENOTSOCK);
    cr_assert_eq(errno5, ENOTSOCK);

    // cleanup
    close(reply_socket);
}

Test(dmqp, test_handle_dmqp_unknown_method_throws_when_method_known) {
    // arrange
    errno = 0;
    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE,
                                 .status_code = 0};
    struct dmqp_message message = {.header = header, .payload = NULL};
    int reply_socket = socket(AF_INET, SOCK_STREAM, 0);

    // act
    int res = handle_dmqp_unknown_method(&message, reply_socket);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, EINVAL);

    // cleanup
    close(reply_socket);
}

Test(dmqp, test_handle_dmqp_unknown_method_success) {
    // arrange
    errno = 0;
    struct dmqp_header header = {.sequence_id = 0,
                                 .length = 0,
                                 .method = DMQP_RESPONSE + 1,
                                 .status_code = 0};
    struct dmqp_message message = {.header = header, .payload = NULL};

    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    // act
    int res1 = handle_dmqp_unknown_method(&message, fds[1]);
    int errno1 = errno;
    close(fds[1]);

    // arrnage
    errno = 0;

    // act
    int res2 = read_dmqp_message(fds[0], &message);
    int errno2 = errno;

    // assert
    cr_assert(res1 >= 0);
    cr_assert(res2 >= 0);
    cr_assert_eq(errno1, 0);
    cr_assert_eq(errno2, 0);

    cr_assert_eq(message.header.sequence_id, 0);
    cr_assert_eq(message.header.length, 0);
    cr_assert_eq(message.header.method, DMQP_RESPONSE);
    cr_assert_eq(message.header.status_code, ENOSYS);

    // cleanup
    close(fds[0]);
}
