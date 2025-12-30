#include "queue.h"

#include <messageq/test.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_queue_init_throws_error_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(queue_init(NULL) < 0);
    assert(EINVAL == errno);
    return 0;
}

int test_queue_init_success() {
    // arrange
    errno = 0;
    struct queue queue;

    // act & assert
    assert(queue_init(&queue) >= 0);
    assert(!errno);
    assert(!queue.head);
    assert(!queue.tail);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_push_throws_error_when_invalid_args() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    // act & assert
    assert(queue_push(NULL, NULL) < 0);
    assert(errno == EINVAL);

    assert(queue_push(NULL, &entry) < 0);
    assert(errno == EINVAL);

    assert(queue_push(&queue, NULL) < 0);
    assert(errno == EINVAL);

    // arrange
    entry.data = NULL;
    entry.size = 0;

    // act & assert
    assert(queue_push(&queue, &entry) < 0);
    assert(errno == EINVAL);

    // arrange
    entry.data = "Hello, World!";

    // act & assert
    assert(queue_push(&queue, &entry) < 0);
    assert(errno == EINVAL);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_push_success_when_empty() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;
    entry.data = "Hello, World!";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x1000;

    // act & assert
    assert(queue_push(&queue, &entry) >= 0);
    assert(!errno);
    assert(queue.head == queue.tail);
    assert(memcmp(queue.head->entry.data, entry.data, entry.size) == 0);
    assert(queue.head->entry.size == entry.size);
    assert(queue.head->entry.timestamp == entry.timestamp);
    assert(!queue.head->next);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_push_success_when_size_is_one() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry1;
    entry1.data = "Hello, ";
    entry1.size = strlen(entry1.data);
    entry1.timestamp = 0x1000;

    queue_push(&queue, &entry1);

    struct queue_entry entry2;
    entry2.data = "World!";
    entry2.size = strlen(entry2.data);
    entry2.timestamp = 0x2000;

    // act & assert
    assert(queue_push(&queue, &entry2) >= 0);
    assert(!errno);
    assert(queue.head != queue.tail);
    assert(memcmp(queue.head->entry.data, entry1.data, entry1.size) == 0);
    assert(queue.head->entry.size == entry1.size);
    assert(queue.head->entry.timestamp == entry1.timestamp);
    assert(queue.tail == queue.head->next);

    assert(memcmp(queue.tail->entry.data, entry2.data, entry2.size) == 0);
    assert(queue.tail->entry.size == entry2.size);
    assert(queue.tail->entry.timestamp == entry2.timestamp);
    assert(!queue.tail->next);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_push_success_when_size_is_greater_than_one() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry1;
    entry1.data = "Hello";
    entry1.size = strlen(entry1.data);
    entry1.timestamp = 0x1000;
    queue_push(&queue, &entry1);

    struct queue_entry entry2;
    entry2.data = ", ";
    entry2.size = strlen(entry2.data);
    entry2.timestamp = 0x2000;
    queue_push(&queue, &entry2);

    struct queue_entry entry3;
    entry3.data = "World";
    entry3.size = strlen(entry3.data);
    entry3.timestamp = 0x3000;
    queue_push(&queue, &entry3);

    struct queue_entry entry4;
    entry4.data = "!";
    entry4.size = strlen(entry4.data);
    entry4.timestamp = 0x4000;

    // act & assert
    assert(queue_push(&queue, &entry4) >= 0);
    assert(!errno);

    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;
    struct queue_node *node4 = node3->next;

    assert(queue.head != queue.tail);
    assert(memcmp(queue.tail->entry.data, entry4.data, entry4.size) == 0);
    assert(queue.tail->entry.size == entry4.size);
    assert(queue.tail->entry.timestamp == entry4.timestamp);
    assert(!queue.tail->next);

    assert(memcmp(node1->entry.data, entry1.data, entry1.size) == 0);
    assert(node1->entry.size == entry1.size);
    assert(node1->entry.timestamp == entry1.timestamp);

    assert(memcmp(node2->entry.data, entry2.data, entry2.size) == 0);
    assert(node2->entry.size == entry2.size);
    assert(node2->entry.timestamp == entry2.timestamp);

    assert(memcmp(node3->entry.data, entry3.data, entry3.size) == 0);
    assert(node3->entry.size == entry3.size);
    assert(node3->entry.timestamp == entry3.timestamp);

    assert(memcmp(node4->entry.data, entry4.data, entry4.size) == 0);
    assert(node4->entry.size == entry4.size);
    assert(node4->entry.timestamp == entry4.timestamp);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_pop_throws_error_when_invalid_args() {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;

    // act & assert
    assert(queue_pop(NULL, NULL) < 0);
    assert(errno == EINVAL);

    assert(queue_pop(NULL, &entry) < 0);
    assert(errno == EINVAL);

    assert(queue_pop(&queue, NULL) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_queue_pop_throws_error_when_empty() {
    // arrange
    errno = 0;
    struct queue queue;
    struct queue_entry entry;
    queue_init(&queue);

    // act & assert
    assert(queue_pop(&queue, &entry) < 0);
    assert(errno == ENODATA);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_pop_success_when_size_is_one() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;
    entry.data = "Hello, World!";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x1000;
    queue_push(&queue, &entry);

    // act & assert
    assert(queue_pop(&queue, &entry) >= 0);
    assert(!errno);
    assert(!queue.head);
    assert(!queue.tail);
    assert(memcmp(entry.data, "Hello, World!", 13) == 0);
    assert(entry.size == 13);
    assert(entry.timestamp == 0x1000);

    // teardown
    free(entry.data);
    queue_destroy(&queue);
    return 0;
}

int test_queue_pop_success_when_size_is_greater_than_one() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    entry.data = "Hello";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x1000;
    queue_push(&queue, &entry);

    entry.data = ", ";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x2000;
    queue_push(&queue, &entry);

    entry.data = "World";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x3000;
    queue_push(&queue, &entry);

    entry.data = "!";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x4000;
    queue_push(&queue, &entry);

    // act & assert
    assert(queue_pop(&queue, &entry) >= 0);
    assert(!errno);

    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;

    assert(memcmp(entry.data, "Hello", 5) == 0);
    assert(entry.size == 5);
    assert(entry.timestamp == 0x1000);

    assert(memcmp(node1->entry.data, ", ", 2) == 0);
    assert(node1->entry.size == 2);
    assert(node1->entry.timestamp == 0x2000);

    assert(memcmp(node2->entry.data, "World", 5) == 0);
    assert(node2->entry.size == 5);
    assert(node2->entry.timestamp == 0x3000);

    assert(memcmp(node3->entry.data, "!", 1) == 0);
    assert(node3->entry.size == 1);
    assert(node3->entry.timestamp == 0x4000);

    // teardown
    free(entry.data);
    queue_destroy(&queue);
    return 0;
}

// Test(queue, test_queue_peek_throws_error_when_invalid_args) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     struct queue_entry entry;

//     // act
//     int res1 = queue_peek(NULL, NULL);
//     int errno1 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res2 = queue_peek(NULL, &entry);
//     int errno2 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res3 = queue_peek(&queue, NULL);
//     int errno3 = errno;

//     // assert
//     cr_assert(res1 < 0);
//     cr_assert(res2 < 0);
//     cr_assert(res3 < 0);
//     cr_assert_eq(EINVAL, errno1);
//     cr_assert_eq(EINVAL, errno2);
//     cr_assert_eq(EINVAL, errno3);
// }

// Test(queue, test_queue_peek_throws_error_when_empty) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     struct queue_entry entry;
//     queue_init(&queue);

//     // act
//     int res = queue_peek(&queue, &entry);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(ENODATA, errno);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_peek_success) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry;

//     entry.data = "Hello";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x1000;
//     queue_push(&queue, &entry);

//     entry.data = ", ";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x2000;
//     queue_push(&queue, &entry);

//     entry.data = "World";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x3000;
//     queue_push(&queue, &entry);

//     entry.data = "!";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x4000;
//     queue_push(&queue, &entry);

//     // act
//     int res = queue_peek(&queue, &entry);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);

//     cr_assert_arr_eq(entry.data, "Hello", 5);
//     cr_assert_eq(entry.size, 5);
//     cr_assert_eq(entry.timestamp, 0x1000);

//     cr_assert_arr_eq(queue.head->entry.data, "Hello", 5);
//     cr_assert_eq(queue.head->entry.size, 5);
//     cr_assert_eq(queue.head->entry.timestamp, 0x1000);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_peek_timestamp_throws_error_when_invalid_args) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     long timestamp;

//     // act
//     int res1 = queue_peek_timestamp(NULL, NULL);
//     int errno1 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res2 = queue_peek_timestamp(NULL, &timestamp);
//     int errno2 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res3 = queue_peek_timestamp(&queue, NULL);
//     int errno3 = errno;

//     // assert
//     cr_assert(res1 < 0);
//     cr_assert(res2 < 0);
//     cr_assert(res3 < 0);
//     cr_assert_eq(errno1, EINVAL);
//     cr_assert_eq(errno2, EINVAL);
//     cr_assert_eq(errno3, EINVAL);
// }

// Test(queue, test_queue_peek_timestamp_throws_error_when_empty) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     long timestamp;
//     queue_init(&queue);

//     // act
//     int res = queue_peek_timestamp(&queue, &timestamp);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, ENODATA);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_peek_timestamp_success) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     long timestamp;
//     queue_init(&queue);

//     struct queue_entry entry;

//     entry.data = "Hello";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x1000;
//     queue_push(&queue, &entry);

//     entry.data = ", ";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x2000;
//     queue_push(&queue, &entry);

//     entry.data = "World";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x3000;
//     queue_push(&queue, &entry);

//     entry.data = "!";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x4000;
//     queue_push(&queue, &entry);

//     // act
//     int res = queue_peek_timestamp(&queue, &timestamp);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_eq(timestamp, 0x1000);
//     cr_assert_eq(queue.head->entry.timestamp, 0x1000);

//     // cleanup
//     queue_destroy(&queue);
// }

int test_queue_destroy_throws_error_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(queue_destroy(NULL) < 0);
    assert(EINVAL == errno);
    return 0;
}

int test_queue_destroy_success() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    entry.data = "Hello";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x1000;
    queue_push(&queue, &entry);

    entry.data = ", ";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x2000;
    queue_push(&queue, &entry);

    entry.data = "World";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x3000;
    queue_push(&queue, &entry);

    entry.data = "!";
    entry.size = strlen(entry.data);
    entry.timestamp = 0x4000;
    queue_push(&queue, &entry);

    // act & assert
    assert(queue_destroy(&queue) >= 0);
    assert(!errno);
    assert(!queue.head);
    assert(!queue.tail);
    return 0;
}

struct test_case tests[] = {
    {NULL, NULL, test_queue_init_throws_error_when_invalid_args},
    {NULL, NULL, test_queue_init_success},
    {NULL, NULL, test_queue_push_throws_error_when_invalid_args},
    {NULL, NULL, test_queue_push_success_when_empty},
    {NULL, NULL, test_queue_push_success_when_size_is_one},
    {NULL, NULL, test_queue_push_success_when_size_is_greater_than_one},
    {NULL, NULL, test_queue_pop_throws_error_when_invalid_args},
    {NULL, NULL, test_queue_pop_throws_error_when_empty},
    {NULL, NULL, test_queue_pop_success_when_size_is_one},
    {NULL, NULL, test_queue_pop_success_when_size_is_greater_than_one},
    {NULL, NULL, test_queue_destroy_throws_error_when_invalid_args},
    {NULL, NULL, test_queue_destroy_success}};

struct test_suite suite = {
    .name = "test_queue", .setup = NULL, .teardown = NULL};

int main() { run_suite(); }
