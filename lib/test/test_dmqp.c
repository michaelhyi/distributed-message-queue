#include "dmqp.h"

#include <criterion/criterion.h>
#include <errno.h>

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

// TODO: hard to unit test
Test(dmqp, test_handle_server_message_success) {
    // arrange
    errno = 0;
    char *data = "Hello, World!";
    struct dmqp_header header;
    header.length = strlen(data);

    char message[sizeof(struct dmqp_header) + header.length];
    memcpy(message, &header, sizeof(struct dmqp_header));
    memcpy(message + sizeof(struct dmqp_header), data, header.length);

    // act
    int res = handle_server_message(&header, 1024, 1);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
}

Test(dmqp, test_handle_server_message_request_additional_bytes_success) {}
