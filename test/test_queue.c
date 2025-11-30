#include "queue.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_queue_init_throws_error_when_invalid_args() {
    // act
    int res = queue_init(NULL);

    // assert
    assert(res < 0);
}

void test_queue_init_success() {
    // arrange
    struct queue queue;

    // act
    int res = queue_init(&queue);

    // assert
    assert(res >= 0);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);

    queue_destroy(&queue);
}

void test_queue_push_throws_error_when_invalid_args() {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = (char *) 0x1;

    // act
    int res1 = queue_push(NULL, NULL, 0);
    int res2 = queue_push(&queue, NULL, 0);
    int res3 = queue_push(NULL, data, 0);
    int res4 = queue_push(NULL, NULL, 1);

    // assert
    assert(res1 < 0);
    assert(res2 < 0);
    assert(res3 < 0);
    assert(res4 < 0);

    queue_destroy(&queue);
}

void test_queue_push_success_when_empty() {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    assert(res >= 0);
    assert(queue.head == queue.tail);
    assert(memcmp(queue.head->data, data, strlen(data)) == 0);
    assert(queue.head->next == NULL);

    queue_destroy(&queue);
}

void test_queue_push_success_when_size_is_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello, ";
    queue_push(&queue, data, strlen(data));

    data = "World!";

    // act
    int res = queue_push(&queue, data, strlen(data));

    // assert
    assert(res >= 0);
    assert(queue.head != queue.tail);
    assert(memcmp(queue.head->data, "Hello, ", 7) == 0);
    assert(queue.head->next == queue.tail);

    assert(memcmp(queue.tail->data, "World!", 6) == 0);
    assert(queue.tail->next == NULL);

    queue_destroy(&queue);
}

void test_queue_push_success_when_size_is_greater_than_one() {
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
    assert(res >= 0);
    assert(queue.head != queue.tail);
    assert(memcmp(queue.tail->data, "!", 1) == 0);
    assert(queue.tail->next == NULL);

    assert(memcmp(node1->data, "Hello", 5) == 0);
    assert(memcmp(node2->data, ", ", 2) == 0);
    assert(memcmp(node3->data, "World", 5) == 0);
    assert(memcmp(node4->data, "!", 1) == 0);

    queue_destroy(&queue);
}

void test_queue_pop_throws_error_when_invalid_args() {
    // act
    char *res = queue_pop(NULL);

    // assert
    assert(res == NULL);
}

void test_queue_pop_returns_null_when_empty() {
    // arrange
    struct queue queue;
    queue_init(&queue);

    // act
    char *res = queue_pop(&queue);

    // assert
    assert(res == NULL);

    queue_destroy(&queue);
}

void test_queue_pop_success_when_size_is_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";
    queue_push(&queue, data, strlen(data));

    // act
    char *res = queue_pop(&queue);

    // assert
    assert(res != NULL);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    assert(memcmp(res, data, strlen(data)) == 0);

    free(res);
    queue_destroy(&queue);
}

void test_queue_pop_success_when_size_is_greater_than_one() {
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
    char *res = queue_pop(&queue);
    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;

    // assert
    assert(memcmp(res, "Hello", 5) == 0);
    assert(memcmp(node1->data, ", ", 2) == 0);
    assert(memcmp(node2->data, "World", 5) == 0);
    assert(memcmp(node3->data, "!", 1) == 0);

    free(res);
    queue_destroy(&queue);
}

void (*tests[])(void) = {
    test_queue_init_throws_error_when_invalid_args,
    test_queue_init_success,

    test_queue_push_throws_error_when_invalid_args,
    test_queue_push_success_when_empty,
    test_queue_push_success_when_size_is_one,
    test_queue_push_success_when_size_is_greater_than_one,

    test_queue_pop_throws_error_when_invalid_args,
    test_queue_pop_returns_null_when_empty,
    test_queue_pop_success_when_size_is_one,
    test_queue_pop_success_when_size_is_greater_than_one
};

int main() {
    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        tests[i]();
    }

    printf("All tests passed!\n");
    return 0;
}
