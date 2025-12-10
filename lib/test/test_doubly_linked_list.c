#include "doubly_linked_list.h"

#include <criterion/criterion.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

TestSuite(doubly_linked_list, .timeout = 10);

Test(doubly_linked_list,
     test_doubly_linked_list_init_throws_error_on_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = doubly_linked_list_init(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EINVAL, errno);
}

Test(doubly_linked_list, test_doubly_linked_list_init_success) {
    // arrange
    errno = 0;
    struct doubly_linked_list list;

    // act
    int res = doubly_linked_list_init(&list);

    // arrange
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(list.head);
    cr_assert_null(list.tail);
}

Test(doubly_linked_list,
     test_doubly_linked_list_add_throws_error_on_invalid_args) {
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
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);
    cr_assert(res4 < 0);

    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
    cr_assert_eq(EINVAL, errno3);
    cr_assert_eq(EINVAL, errno4);
}

Test(doubly_linked_list, test_doubly_linked_list_add_success_when_empty_list) {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello, World!";

    // act
    int res = doubly_linked_list_add(&list, data, strlen(data));

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_eq(list.head, list.tail);
    cr_assert_arr_eq(list.head->data, data, strlen(data));
    cr_assert_null(list.head->prev);
    cr_assert_null(list.head->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(doubly_linked_list,
     test_doubly_linked_list_add_success_when_list_size_one) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert(list.head != list.tail);

    cr_assert_arr_eq(list.head->data, "Hello, ", 7);
    cr_assert_null(list.head->prev);
    cr_assert_eq(list.tail, list.head->next);

    cr_assert_arr_eq(list.tail->data, "World!", 6);
    cr_assert_eq(list.head, list.tail->prev);
    cr_assert_null(list.tail->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(doubly_linked_list,
     test_doubly_linked_list_add_success_when_list_size_greater_than_one) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert(list.head != list.tail);
    cr_assert_eq(list.head, node1);
    cr_assert_eq(list.tail, node3);

    cr_assert_arr_eq(node1->data, "Hello", 5);
    cr_assert_null(node1->prev);
    cr_assert_eq(node2, node1->next);

    cr_assert_arr_eq(node2->data, ", ", 2);
    cr_assert_eq(node1, node2->prev);
    cr_assert_eq(node3, node2->next);

    cr_assert_arr_eq(node3->data, "World!", 6);
    cr_assert_eq(node2, node3->prev);
    cr_assert_null(node3->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(doubly_linked_list,
     test_doubly_linked_list_remove_throws_error_on_invalid_args) {
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
    cr_assert(res1 < 0);
    cr_assert(res2 < 0);
    cr_assert(res3 < 0);

    cr_assert_eq(EINVAL, errno1);
    cr_assert_eq(EINVAL, errno2);
    cr_assert_eq(EINVAL, errno3);
}

Test(doubly_linked_list,
     test_doubly_linked_list_remove_throws_error_when_empty_list) {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    // act
    int res = doubly_linked_list_remove(&list, (void *)0x1);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EINVAL, errno);
}

Test(doubly_linked_list,
     test_doubly_linked_list_remove_success_when_list_size_one) {
    // arrange
    errno = 0;

    struct doubly_linked_list list;
    doubly_linked_list_init(&list);

    char *data = "Hello, ";
    doubly_linked_list_add(&list, data, strlen(data));

    // act
    int res = doubly_linked_list_remove(&list, list.head);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(list.head);
    cr_assert_null(list.tail);
}

Test(
    doubly_linked_list,
    test_doubly_linked_list_remove_head_success_when_list_size_greater_than_two) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_eq(node1, list.head);
    cr_assert_eq(node2, list.tail);

    cr_assert_arr_eq(node1->data, ", ", 2);
    cr_assert_null(node1->prev);
    cr_assert_eq(node2, node1->next);

    cr_assert_arr_eq(node2->data, "World!", 6);
    cr_assert_eq(node1, node2->prev);
    cr_assert_null(node2->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(
    doubly_linked_list,
    test_doubly_linked_list_remove_middle_success_when_list_size_greater_than_two) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_eq(node1, list.head);
    cr_assert_eq(node2, list.tail);

    cr_assert_arr_eq(node1->data, "Hello", 5);
    cr_assert_null(node1->prev);
    cr_assert_eq(node2, node1->next);

    cr_assert_arr_eq(node2->data, "World!", 6);
    cr_assert_eq(node1, node2->prev);
    cr_assert_null(node2->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(
    doubly_linked_list,
    test_doubly_linked_list_remove_tail_success_when_list_size_greater_than_two) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_eq(node1, list.head);
    cr_assert_eq(node2, list.tail);

    cr_assert_arr_eq(node1->data, "Hello", 5);
    cr_assert_null(node1->prev);
    cr_assert_eq(node2, node1->next);

    cr_assert_arr_eq(node2->data, ", ", 2);
    cr_assert_eq(node1, node2->prev);
    cr_assert_null(node2->next);

    // cleanup
    doubly_linked_list_destroy(&list);
}

Test(doubly_linked_list,
     test_doubly_linked_list_destroy_throws_error_on_invalid_args) {
    // arrange
    errno = 0;

    // act
    int res = doubly_linked_list_destroy(NULL);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(EINVAL, errno);
}

Test(doubly_linked_list, test_doubly_linked_list_destroy_success) {
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
    cr_assert(res >= 0);
    cr_assert_eq(0, errno);
    cr_assert_null(list.head);
    cr_assert_null(list.tail);
}
