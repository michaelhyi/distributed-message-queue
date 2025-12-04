#include "dmqp.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <unistd.h>

static struct dmqp_message test_message;

/**
 * Custom implementation of `handle_dmqp_message` for testing purposes.
 * Updates a global variable `test_message` used for assertions.
 */
int handle_dmqp_message(struct dmqp_message message) {
    test_message = message;
    test_message.payload = malloc(test_message.header.length);
    memcpy(test_message.payload, message.payload, test_message.header.length);
    return 0;
}

Test(dmqp, test_handle_server_message_throws_error_when_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res1 = handle_server_message(NULL, 0, -1);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = handle_server_message((char *)0x1, 5, 1);
    int errno2 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
}

Test(dmqp, test_handle_server_message_throws_error_when_payload_too_big) {
    // arrange
    errno = 0;
    struct dmqp_header header;
    header.length = 1 << 30;

    // act
    int res = handle_server_message(&header, 1024, 1);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EMSGSIZE, errno);
}

Test(dmqp, test_handle_server_message_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header;
    header.method = PUSH;
    header.flags = ENCRYPTED_FLAG;
    header.length = strlen(data) + 1;

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy((char *)message + sizeof(struct dmqp_header), data, header.length);

    // act
    int res = handle_server_message(message, 1024, 1);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(test_message.header.method, PUSH);
    cr_assert_eq(test_message.header.flags & ENCRYPTED_FLAG, 1);
    cr_assert_eq(test_message.header.length, strlen(data) + 1);
    cr_assert_str_eq(test_message.payload, data);

    // cleanup
    free(test_message.payload);
}

Test(dmqp, test_handle_server_message_request_additional_bytes_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header;
    header.method = PUSH;
    header.flags = ENCRYPTED_FLAG;
    header.length = strlen(data) + 1;

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy((char *)message + sizeof(struct dmqp_header), data, header.length);

    int fds[2];
    pipe(fds);
    write(fds[1], "World!", 7);
    close(fds[1]);

    // act
    int res =
        handle_server_message(message, sizeof(struct dmqp_header) + 7, fds[0]);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(test_message.header.method, PUSH);
    cr_assert_eq(test_message.header.flags & ENCRYPTED_FLAG, 1);
    cr_assert_eq(test_message.header.length, strlen(data) + 1);
    cr_assert_str_eq(test_message.payload, data);

    // cleanup
    close(fds[0]);
    free(test_message.payload);
}
