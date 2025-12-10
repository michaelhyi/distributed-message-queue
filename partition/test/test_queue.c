#include "queue.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TestSuite(queue, .timeout = 10);

Test(queue, test_queue_init_throws_error_when_invalid_args) {
    // act
    errno = 0;
    int res = queue_init(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EINVAL, errno);
}

Test(queue, test_queue_init_success) {
    // arrange
    errno = 0;
    struct queue queue;

    // act
    int res = queue_init(&queue);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_throws_error_when_invalid_args) {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);
    char *data = (char *)0x1;

    // act
    int res1 = queue_push(NULL, NULL, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = queue_push(NULL, data, 1);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = queue_push(&queue, NULL, 1);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = queue_push(&queue, data, 0);
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

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_empty) {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_eq(queue.head, queue.tail);
    cr_assert_arr_eq(queue.head->entry.data, data, strlen(data));
    cr_assert_eq(queue.head->entry.size, strlen(data));
    cr_assert_null(queue.head->next);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_size_is_one) {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello, ";
    queue_push(&queue, data, strlen(data));

    data = "World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert(queue.head != queue.tail);
    cr_assert_arr_eq(queue.head->entry.data, "Hello, ", 7);
    cr_assert_eq(queue.head->entry.size, 7);
    cr_assert_eq(queue.tail, queue.head->next);

    cr_assert_arr_eq(queue.tail->entry.data, "World!", 6);
    cr_assert_eq(queue.tail->entry.size, 6);
    cr_assert_null(queue.tail->next);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_size_is_greater_than_one) {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data, strlen(data));

    data = ", ";
    queue_push(&queue, data, strlen(data));

    data = "World";
    queue_push(&queue, data, strlen(data));

    data = "!";

    // act
    int res = queue_push(&queue, data, strlen(data));
    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;
    struct queue_node *node4 = node3->next;

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert(queue.head != queue.tail);
    cr_assert_arr_eq(queue.tail->entry.data, "!", 1);
    cr_assert_eq(queue.tail->entry.size, 1);
    cr_assert_null(queue.tail->next);

    cr_assert_arr_eq(node1->entry.data, "Hello", 5);
    cr_assert_eq(node1->entry.size, 5);
    cr_assert_arr_eq(node2->entry.data, ", ", 2);
    cr_assert_eq(node2->entry.size, 2);
    cr_assert_arr_eq(node3->entry.data, "World", 5);
    cr_assert_eq(node3->entry.size, 5);
    cr_assert_arr_eq(node4->entry.data, "!", 1);
    cr_assert_eq(node4->entry.size, 1);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_throws_error_when_invalid_args) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;

    // act
    int res1 = queue_pop(NULL, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = queue_pop(NULL, &entry);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = queue_pop(&queue, NULL);
    int errno3 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
    cr_assert_eq(EINVAL, errno3);
}

Test(queue, test_queue_pop_throws_error_when_empty) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    // act
    int res = queue_pop(&queue, &entry);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(ENODATA, errno);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_success_when_size_is_one) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    char *data = "Hello, World!";
    queue_push(&queue, data, strlen(data));

    // act
    int res = queue_pop(&queue, &entry);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);
    cr_assert_arr_eq(entry.data, data, strlen(data));
    cr_assert_eq(entry.size, strlen(data));

    // cleanup
    free(entry.data);
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_success_when_size_is_greater_than_one) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data, strlen(data));

    data = ", ";
    queue_push(&queue, data, strlen(data));

    data = "World";
    queue_push(&queue, data, strlen(data));

    data = "!";
    queue_push(&queue, data, strlen(data));

    // act
    int res = queue_pop(&queue, &entry);
    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_arr_eq(entry.data, "Hello", 5);
    cr_assert_eq(entry.size, 5);
    cr_assert_arr_eq(node1->entry.data, ", ", 2);
    cr_assert_arr_eq(node2->entry.data, "World", 5);
    cr_assert_arr_eq(node3->entry.data, "!", 1);

    // cleanup
    free(entry.data);
    queue_destroy(&queue);
}

Test(queue, test_queue_peek_throws_error_when_invalid_args) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;

    // act
    int res1 = queue_peek(NULL, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = queue_peek(NULL, &entry);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = queue_peek(&queue, NULL);
    int errno3 = errno;

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
    cr_assert_eq(EINVAL, errno3);
}

Test(queue, test_queue_peek_throws_error_when_empty) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    // act
    int res = queue_peek(&queue, &entry);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(ENODATA, errno);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_peek_success) {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data, strlen(data));

    data = ", ";
    queue_push(&queue, data, strlen(data));

    data = "World";
    queue_push(&queue, data, strlen(data));

    data = "!";
    queue_push(&queue, data, strlen(data));

    // act
    int res = queue_peek(&queue, &entry);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_arr_eq(entry.data, "Hello", 5);
    cr_assert_arr_eq(queue.head->entry.data, "Hello", 5);
    cr_assert_eq(queue.head->entry.size, 5);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_destroy_throws_error_when_invalid_args) {
    // act
    errno = 0;
    int res = queue_destroy(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EINVAL, errno);
}

Test(queue, test_queue_destroy) {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data, strlen(data));

    data = ", ";
    queue_push(&queue, data, strlen(data));

    data = "World";
    queue_push(&queue, data, strlen(data));

    data = "!";
    queue_push(&queue, data, strlen(data));

    // act
    int res = queue_destroy(&queue);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);
}
