#include "queue.h"

#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TOOD: test errno

Test(queue, test_queue_init_throws_error_when_invalid_args) {
    // act
    int res = queue_init(NULL);

    // assert
    cr_assert(res < 0);
}

Test(queue, test_queue_init_success) {
    // arrange
    struct queue queue;

    // act
    int res = queue_init(&queue);

    // assert
    cr_assert(res >= 0);
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_throws_error_when_invalid_args) {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = (char *) 0x1;

    // act
    int res1 = queue_push(NULL, NULL, 0);
    int res2 = queue_push(NULL, data, 1);
    int res3 = queue_push(&queue, NULL, 1);
    int res4 = queue_push(&queue, data, 0);

    // assert
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert(res4 < 0);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_empty) {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(queue.head, queue.tail);
    cr_assert_arr_eq(queue.head->data, data, strlen(data));
    cr_assert_null(queue.head->next);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_size_is_one) {
    // arrange
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello, ";
    queue_push(&queue, data, strlen(data));

    data = "World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    cr_assert(res >= 0);
    cr_assert(queue.head != queue.tail);
    cr_assert_arr_eq(queue.head->data, "Hello, ", 7);
    cr_assert_eq(queue.tail, queue.head->next);

    cr_assert_arr_eq(queue.tail->data, "World!", 6);
    cr_assert_null(queue.tail->next);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_push_success_when_size_is_greater_than_one) {
    // arrange
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
    cr_assert(queue.head != queue.tail);
    cr_assert_arr_eq(queue.tail->data, "!", 1);
    cr_assert_null(queue.tail->next);

    cr_assert_arr_eq(node1->data, "Hello", 5);
    cr_assert_arr_eq(node2->data, ", ", 2);
    cr_assert_arr_eq(node3->data, "World", 5);
    cr_assert_arr_eq(node4->data, "!", 1);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_throws_error_when_invalid_args) {
    // act
    struct queue_node *res = queue_pop(NULL);

    // assert
    cr_assert_null(res);
}

Test(queue, test_queue_pop_returns_null_when_empty) {
    // arrange
    struct queue queue;
    queue_init(&queue);

    // act
    struct queue_node *res = queue_pop(&queue);

    // assert
    cr_assert_null(res);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_success_when_size_is_one) {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";
    queue_push(&queue, data, strlen(data));

    // act
    struct queue_node *res = queue_pop(&queue);

    // assert
    cr_assert_not_null(res);
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);
    cr_assert_arr_eq(res->data, data, strlen(data));

    // cleanup
    free(res->data);
    free(res);
    queue_destroy(&queue);
}

Test(queue, test_queue_pop_success_when_size_is_greater_than_one) {
    // arrange
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
    struct queue_node *res = queue_pop(&queue);
    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;

    // assert
    cr_assert_arr_eq(res->data, "Hello", 5);
    cr_assert_arr_eq(node1->data, ", ", 2);
    cr_assert_arr_eq(node2->data, "World", 5);
    cr_assert_arr_eq(node3->data, "!", 1);

    // cleanup
    free(res->data);
    free(res);
    queue_destroy(&queue);
}

Test(queue, test_queue_peek_throws_error_when_invalid_args) {
    // act
    struct queue_node *res = queue_peek(NULL);

    // assert
    cr_assert_null(res);
}

Test(queue, test_queue_peek_returns_null_when_empty) {
    // arrange
    struct queue queue;
    queue_init(&queue);

    // act
    struct queue_node *res = queue_peek(&queue);

    // assert
    cr_assert_null(res);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_peek_success) {
    // arrange
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
    struct queue_node *res = queue_peek(&queue);

    // assert
    cr_assert_arr_eq(res->data, "Hello", 5);

    // cleanup
    queue_destroy(&queue);
}

Test(queue, test_queue_destroy_throws_error_when_invalid_args) {
    // act
    int res = queue_destroy(NULL);

    // assert
    cr_assert(res < 0);
}

Test(queue, test_queue_destroy) {
    // arrange
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
    cr_assert_null(queue.head);
    cr_assert_null(queue.tail);
}
