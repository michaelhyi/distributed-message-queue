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
}

void test_queue_push_throws_error_when_invalid_args() {
    // arrange
    struct queue queue;
    char *data;

    // act
    int res1 = queue_push(NULL, NULL);
    int res2 = queue_push(&queue, NULL);
    int res3 = queue_push(NULL, data);

    // assert
    assert(res1 < 0);
    assert(res2 < 0);
    assert(res3 < 0);
}

void test_queue_push_success_when_empty() {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";

    // act
    int res = queue_push(&queue, data);

    // assert
    assert(res >= 0);
    assert(queue.head == queue.tail);
    assert(strcmp(queue.head->data, data) == 0);
    assert(queue.head->next == NULL);

    free(queue.head);
}

void test_queue_push_success_when_size_is_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello, ";
    queue_push(&queue, data);

    data = "World!";

    // act
    int res = queue_push(&queue, data);

    // assert
    assert(res >= 0);
    assert(queue.head != queue.tail);
    assert(strcmp(queue.head->data, "Hello, ") == 0);
    assert(queue.head->next == queue.tail);

    assert(strcmp(queue.tail->data, "World!") == 0);
    assert(queue.tail->next == NULL);

    free(queue.head);
    free(queue.tail);
}

void test_queue_push_success_when_size_is_greater_than_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data);

    data = ", ";
    queue_push(&queue, data);

    data = "World";
    queue_push(&queue, data);

    data = "!";

    // act
    int res = queue_push(&queue, data);

    // assert
    assert(res >= 0);
    assert(queue.head != queue.tail);
    assert(strcmp(queue.tail->data, "!") == 0);
    assert(queue.tail->next == NULL);

    struct queue_node *curr = queue.head;
    assert(strcmp(curr->data, "Hello") == 0);
    struct queue_node *temp = curr;
    curr = curr->next;
    free(temp);

    assert(strcmp(curr->data, ", ") == 0);
    temp = curr;
    curr = curr->next;
    free(temp);

    assert(strcmp(curr->data, "World") == 0);
    temp = curr;
    curr = curr->next;
    free(temp);

    assert(strcmp(curr->data, "!") == 0);
    free(curr);
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
}

void test_queue_pop_success_when_size_is_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);
    char *data = "Hello, World!";
    queue_push(&queue, data);

    // act
    char *res = queue_pop(&queue);

    // assert
    assert(res != NULL);
    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    assert(strcmp(res, data) == 0);

    free(res);
}

void test_queue_pop_success_when_size_is_greater_than_one() {
    // arrange
    struct queue queue;
    queue_init(&queue);

    char *data = "Hello";
    queue_push(&queue, data);

    data = ", ";
    queue_push(&queue, data);

    data = "World";
    queue_push(&queue, data);

    data = "!";
    queue_push(&queue, data);

    // act
    char *res = queue_pop(&queue);

    // assert
    assert(strcmp(res, "Hello") == 0);

    struct queue_node *curr = queue.head;
    assert(strcmp(curr->data, ", ") == 0);
    struct queue_node *temp = curr;
    curr = curr->next;
    free(temp);

    assert(strcmp(curr->data, "World") == 0);
    temp = curr;
    curr = curr->next;
    free(temp);

    assert(strcmp(curr->data, "!") == 0);
    curr = curr->next;
    free(curr);
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
    unsigned int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (unsigned int i = 0; i < num_tests; i++) {
        tests[i]();
    }

    printf("All tests passed!\n");
    return 0;
}
