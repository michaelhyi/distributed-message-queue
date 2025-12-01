#include "doubly_linked_list.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void test_doubly_linked_list_init_throws_error_on_invalid_args() {
    // arrange
    errno = 0;

    // act
    int res = doubly_linked_list_init(NULL);

    // assert
    assert(res < 0);
    assert(errno == EINVAL);
}

void test_doubly_linked_list_init_success() {
    // arrange
    errno = 0;
    struct doubly_linked_list list;

    // act
    int res = doubly_linked_list_init(&list);

    // arrange
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == NULL);
    assert(list.tail == NULL);
}

void test_doubly_linked_list_add_throws_error_on_invalid_args() {
    // arrange
    errno = 0;

    // act
    int res1 = doubly_linked_list_add(NULL, NULL, 0);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = doubly_linked_list_add(NULL, (void *)0x1, 1);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = doubly_linked_list_add((void *)0x1, NULL, 1);
    int errno3 = errno;

    // arrange
    errno = 0;

    // act
    int res4 = doubly_linked_list_add((void *)0x1, (void *)0x1, 0);
    int errno4 = errno;

    // assert
    assert(res1 < 0);
    assert(res2 < 0);
    assert(res3 < 0);
    assert(res4 < 0);

    assert(errno1 == EINVAL);
    assert(errno2 == EINVAL);
    assert(errno3 == EINVAL);
    assert(errno4 == EINVAL);
}

void test_doubly_linked_list_add_success_when_empty_list() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello, World!";

    // act
    int res = doubly_linked_list_add(&list, data, strlen(data));

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == list.tail);
    assert(memcmp(list.head->data, data, strlen(data)) == 0);
    assert(list.head->prev == NULL);
    assert(list.head->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_add_success_when_list_size_one() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello, ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";

    // act
    int res = doubly_linked_list_add(&list, data, strlen(data));

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head != list.tail);

    assert(memcmp(list.head->data, "Hello, ", 7) == 0);
    assert(list.head->prev == NULL);
    assert(list.head->next == list.tail);

    assert(memcmp(list.tail->data, "World!", 6) == 0);
    assert(list.tail->prev == list.head);
    assert(list.tail->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_add_success_when_list_size_greater_than_one() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello";
    doubly_linked_list_add(&list, data, strlen(data));

    data = ", ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";

    // act
    int res = doubly_linked_list_add(&list, data, strlen(data));
    struct doubly_linked_list_node *node1 = list.head;
    struct doubly_linked_list_node *node2 = node1->next;
    struct doubly_linked_list_node *node3 = node2->next;

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head != list.tail);
    assert(list.head == node1);
    assert(list.tail == node3);

    assert(memcmp(node1->data, "Hello", 5) == 0);
    assert(node1->prev == NULL);
    assert(node1->next == node2);

    assert(memcmp(node2->data, ", ", 2) == 0);
    assert(node2->prev == node1);
    assert(node2->next == node3);

    assert(memcmp(node3->data, "World!", 6) == 0);
    assert(node3->prev == node2);
    assert(node3->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_remove_throws_error_on_invalid_args() {
    // arrange
    errno = 0;

    // act
    int res1 = doubly_linked_list_remove(NULL, NULL);
    int errno1 = errno;

    // arrange
    errno = 0;

    // act
    int res2 = doubly_linked_list_remove((void *)0x1, NULL);
    int errno2 = errno;

    // arrange
    errno = 0;

    // act
    int res3 = doubly_linked_list_remove(NULL, (void *)0x1);
    int errno3 = errno;

    // assert
    assert(res1 < 0);
    assert(res2 < 0);
    assert(res3 < 0);

    assert(errno1 == EINVAL);
    assert(errno2 == EINVAL);
    assert(errno3 == EINVAL);
}

void test_doubly_linked_list_remove_throws_error_when_empty_list() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    // act
    int res = doubly_linked_list_remove(&list, (void *)0x1);

    // assert
    assert(res < 0);
    assert(errno == EINVAL);
}

void test_doubly_linked_list_remove_success_when_list_size_one() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello, ";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_remove(&list, list.head);

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == NULL);
    assert(list.tail == NULL);
}

void test_doubly_linked_list_remove_head_success_when_list_size_greater_than_two() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello";
    doubly_linked_list_add(&list, data, strlen(data));

    data = ", ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_remove(&list, list.head);
    struct doubly_linked_list_node *node1 = list.head;
    struct doubly_linked_list_node *node2 = node1->next;

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == node1);
    assert(list.tail == node2);

    assert(memcmp(node1->data, ", ", 2) == 0);
    assert(node1->prev == NULL);
    assert(node1->next == node2);

    assert(memcmp(node2->data, "World!", 6) == 0);
    assert(node2->prev == node1);
    assert(node2->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_remove_middle_success_when_list_size_greater_than_two() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello";
    doubly_linked_list_add(&list, data, strlen(data));

    data = ", ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_remove(&list, list.head->next);
    struct doubly_linked_list_node *node1 = list.head;
    struct doubly_linked_list_node *node2 = node1->next;

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == node1);
    assert(list.tail == node2);

    assert(memcmp(node1->data, "Hello", 5) == 0);
    assert(node1->prev == NULL);
    assert(node1->next == node2);

    assert(memcmp(node2->data, "World!", 6) == 0);
    assert(node2->prev == node1);
    assert(node2->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_remove_tail_success_when_list_size_greater_than_two() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello";
    doubly_linked_list_add(&list, data, strlen(data));

    data = ", ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_remove(&list, list.tail);
    struct doubly_linked_list_node *node1 = list.head;
    struct doubly_linked_list_node *node2 = node1->next;

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == node1);
    assert(list.tail == node2);

    assert(memcmp(node1->data, "Hello", 5) == 0);
    assert(node1->prev == NULL);
    assert(node1->next == node2);

    assert(memcmp(node2->data, ", ", 2) == 0);
    assert(node2->prev == node1);
    assert(node2->next == NULL);

    // cleanup
    doubly_linked_list_destroy(&list);
}

void test_doubly_linked_list_destroy_throws_error_on_invalid_args() {
    // arrange
    errno = 0;

    // act
    int res = doubly_linked_list_destroy(NULL);

    // assert
    assert(res < 0);
    assert(errno == EINVAL);
}

void test_doubly_linked_list_destroy_success() {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello";
    doubly_linked_list_add(&list, data, strlen(data));

    data = ", ";
    doubly_linked_list_add(&list, data, strlen(data));

    data = "World!";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_destroy(&list);

    // assert
    assert(res >= 0);
    assert(errno == 0);
    assert(list.head == NULL);
    assert(list.tail == NULL);
}

void (*tests[])(void) = {
    test_doubly_linked_list_init_throws_error_on_invalid_args,
    test_doubly_linked_list_init_success,

    test_doubly_linked_list_add_throws_error_on_invalid_args,
    test_doubly_linked_list_add_success_when_empty_list,
    test_doubly_linked_list_add_success_when_list_size_one,
    test_doubly_linked_list_add_success_when_list_size_greater_than_one,

    test_doubly_linked_list_remove_throws_error_on_invalid_args,
    test_doubly_linked_list_remove_throws_error_when_empty_list,
    test_doubly_linked_list_remove_success_when_list_size_one,
    test_doubly_linked_list_remove_head_success_when_list_size_greater_than_two,
    test_doubly_linked_list_remove_middle_success_when_list_size_greater_than_two,
    test_doubly_linked_list_remove_tail_success_when_list_size_greater_than_two,

    test_doubly_linked_list_destroy_throws_error_on_invalid_args,
    test_doubly_linked_list_destroy_success
};

int main() {
    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        tests[i]();
    }

    return 0;
}