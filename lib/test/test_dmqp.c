#include "dmqp.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <unistd.h>

// Custom implementation of `handle_dmqp_message` for testing purposes.
int handle_dmqp_message(struct dmqp_message message, int conn_socket) {
    // unused params
    (void)message;
    (void)conn_socket;

    return 0;
}

Test(dmqp, test_handle_server_message_throws_error_when_invalid_args) {
    // arrange
    errno = 0;
    struct dmqp_message message;

    // act
    int res1 = handle_server_message(NULL, sizeof(struct dmqp_header), -1);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = handle_server_message(&message, sizeof(struct dmqp_header), -1);
    int errno2 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert_eq(errno1, EINVAL);
    cr_assert_eq(errno2, EINVAL);
}

Test(dmqp, test_handle_server_message_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header = {
        .method = PUSH, .flags = ENCRYPTED_FLAG, .length = strlen(data) + 1};

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy((char *)message + sizeof(struct dmqp_header), data, header.length);

    // act
    int res = handle_server_message(message, 1024, 1);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);

    // should not have memory leaks
}

Test(dmqp, test_parse_dmqp_message_throws_error_when_invalid_args) {
    // arrange
    errno = 0;
    struct dmqp_message message;

    // act
    int res1 =
        parse_dmqp_message(NULL, sizeof(struct dmqp_header) - 1, -1, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 =
        parse_dmqp_message(&message, sizeof(struct dmqp_header) - 1, -1, NULL);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 =
        parse_dmqp_message(&message, sizeof(struct dmqp_header), -1, NULL);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 =
        parse_dmqp_message(&message, sizeof(struct dmqp_header), 0, NULL);
    int errno4 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert(res4 < 0);
    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
    cr_assert_eq(EINVAL, errno3);
    cr_assert_eq(EINVAL, errno4);
}

Test(dmqp, test_handle_parse_dmqp_message_throws_error_when_payload_too_big) {
    // arrange
    errno = 0;
    struct dmqp_header header;
    header.length = 1 << 30;

    struct dmqp_message message;

    // act
    int res = parse_dmqp_message(&header, 1024, 1, &message);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EMSGSIZE, errno);
}

Test(dmqp, test_parse_dmqp_message_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header = {
        .method = PUSH, .flags = ENCRYPTED_FLAG, .length = strlen(data) + 1};

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy((char *)message + sizeof(struct dmqp_header), data, header.length);

    struct dmqp_message dmqp_message;

    // act
    int res = parse_dmqp_message(message, 1024, 1, &dmqp_message);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(dmqp_message.header.method, PUSH);
    cr_assert_neq(dmqp_message.header.flags & ENCRYPTED_FLAG, 0);
    cr_assert_eq(dmqp_message.header.length, strlen(data) + 1);
    cr_assert_str_eq(dmqp_message.payload, data);

    // cleanup
    free(dmqp_message.payload);
}

Test(dmqp, test_handle_server_message_request_additional_bytes_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header = {
        .method = PUSH, .flags = ENCRYPTED_FLAG, .length = strlen(data) + 1};

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy((char *)message + sizeof(struct dmqp_header), data, header.length);

    struct dmqp_message dmqp_message;

    int mock_sockets[2];
    pipe(mock_sockets);
    write(mock_sockets[1], "World!", 7);
    close(mock_sockets[1]);

    // act
    int res = parse_dmqp_message(message, sizeof(struct dmqp_header) + 7,
                                 mock_sockets[0], &dmqp_message);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(dmqp_message.header.method, PUSH);
    cr_assert_neq(dmqp_message.header.flags & ENCRYPTED_FLAG, 0);
    cr_assert_eq(dmqp_message.header.length, strlen(data) + 1);
    cr_assert_str_eq(dmqp_message.payload, data);

    // cleanup
    close(mock_sockets[0]);
    free(dmqp_message.payload);
}
