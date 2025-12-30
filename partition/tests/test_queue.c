#include "queue.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #ifdef DEBUG
// TestSuite(queue);
// #else
// TestSuite(queue, .timeout = 10);
// #endif

// Test(queue, test_queue_init_throws_error_when_invalid_args) {
//     // act
//     errno = 0;
//     int res = queue_init(NULL);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(EINVAL, errno);
// }

// Test(queue, test_queue_init_success) {
//     // arrange
//     errno = 0;
//     struct queue queue;

//     // act
//     int res = queue_init(&queue);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(0, errno);
//     cr_assert_null(queue.head);
//     cr_assert_null(queue.tail);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_push_throws_error_when_invalid_args) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry;

//     // act
//     int res1 = queue_push(NULL, NULL);
//     int errno1 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res2 = queue_push(NULL, &entry);
//     int errno2 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res3 = queue_push(&queue, NULL);
//     int errno3 = errno;

//     // arrange
//     errno = 0;
//     entry.data = NULL;
//     entry.size = 0;

//     // act
//     int res4 = queue_push(&queue, &entry);
//     int errno4 = errno;

//     // arrange
//     errno = 0;
//     entry.data = "Hello, World!";

//     // act
//     int res5 = queue_push(&queue, &entry);
//     int errno5 = errno;

//     // assert
//     cr_assert(res1 < 0);
//     cr_assert(res2 < 0);
//     cr_assert(res3 < 0);
//     cr_assert(res4 < 0);
//     cr_assert(res5 < 0);
//     cr_assert_eq(errno1, EINVAL);
//     cr_assert_eq(errno2, EINVAL);
//     cr_assert_eq(errno3, EINVAL);
//     cr_assert_eq(errno4, EINVAL);
//     cr_assert_eq(errno5, EINVAL);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_push_success_when_empty) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry;
//     entry.data = "Hello, World!";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x1000;

//     // act
//     int res = queue_push(&queue, &entry);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(0, errno);
//     cr_assert_eq(queue.head, queue.tail);
//     cr_assert_arr_eq(queue.head->entry.data, entry.data, entry.size);
//     cr_assert_eq(queue.head->entry.size, entry.size);
//     cr_assert_eq(queue.head->entry.timestamp, entry.timestamp);
//     cr_assert_null(queue.head->next);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_push_success_when_size_is_one) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry1;
//     entry1.data = "Hello, ";
//     entry1.size = strlen(entry1.data);
//     entry1.timestamp = 0x1000;

//     queue_push(&queue, &entry1);

//     struct queue_entry entry2;
//     entry2.data = "World!";
//     entry2.size = strlen(entry2.data);
//     entry2.timestamp = 0x2000;

//     // act
//     int res = queue_push(&queue, &entry2);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert(queue.head != queue.tail);
//     cr_assert_arr_eq(queue.head->entry.data, entry1.data, entry1.size);
//     cr_assert_eq(queue.head->entry.size, entry1.size);
//     cr_assert_eq(queue.head->entry.timestamp, entry1.timestamp);
//     cr_assert_eq(queue.tail, queue.head->next);

//     cr_assert_arr_eq(queue.tail->entry.data, entry2.data, entry2.size);
//     cr_assert_eq(queue.tail->entry.size, entry2.size);
//     cr_assert_eq(queue.tail->entry.timestamp, entry2.timestamp);
//     cr_assert_null(queue.tail->next);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_push_success_when_size_is_greater_than_one) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry1;
//     entry1.data = "Hello";
//     entry1.size = strlen(entry1.data);
//     entry1.timestamp = 0x1000;
//     queue_push(&queue, &entry1);

//     struct queue_entry entry2;
//     entry2.data = ", ";
//     entry2.size = strlen(entry2.data);
//     entry2.timestamp = 0x2000;
//     queue_push(&queue, &entry2);

//     struct queue_entry entry3;
//     entry3.data = "World";
//     entry3.size = strlen(entry3.data);
//     entry3.timestamp = 0x3000;
//     queue_push(&queue, &entry3);

//     struct queue_entry entry4;
//     entry4.data = "!";
//     entry4.size = strlen(entry4.data);
//     entry4.timestamp = 0x4000;

//     // act
//     int res = queue_push(&queue, &entry4);
//     struct queue_node *node1 = queue.head;
//     struct queue_node *node2 = node1->next;
//     struct queue_node *node3 = node2->next;
//     struct queue_node *node4 = node3->next;

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert(queue.head != queue.tail);
//     cr_assert_arr_eq(queue.tail->entry.data, entry4.data, entry4.size);
//     cr_assert_eq(queue.tail->entry.size, entry4.size);
//     cr_assert_eq(queue.tail->entry.timestamp, entry4.timestamp);
//     cr_assert_null(queue.tail->next);

//     cr_assert_arr_eq(node1->entry.data, entry1.data, entry1.size);
//     cr_assert_eq(node1->entry.size, entry1.size);
//     cr_assert_eq(node1->entry.timestamp, entry1.timestamp);

//     cr_assert_arr_eq(node2->entry.data, entry2.data, entry2.size);
//     cr_assert_eq(node2->entry.size, entry2.size);
//     cr_assert_eq(node2->entry.timestamp, entry2.timestamp);

//     cr_assert_arr_eq(node3->entry.data, entry3.data, entry3.size);
//     cr_assert_eq(node3->entry.size, entry3.size);
//     cr_assert_eq(node3->entry.timestamp, entry3.timestamp);

//     cr_assert_arr_eq(node4->entry.data, entry4.data, entry4.size);
//     cr_assert_eq(node4->entry.size, entry4.size);
//     cr_assert_eq(node4->entry.timestamp, entry4.timestamp);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_pop_throws_error_when_invalid_args) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     struct queue_entry entry;

//     // act
//     int res1 = queue_pop(NULL, NULL);
//     int errno1 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res2 = queue_pop(NULL, &entry);
//     int errno2 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res3 = queue_pop(&queue, NULL);
//     int errno3 = errno;

//     // assert
//     cr_assert(res1 < 0);
//     cr_assert(res2 < 0);
//     cr_assert(res3 < 0);
//     cr_assert_eq(EINVAL, errno1);
//     cr_assert_eq(EINVAL, errno2);
//     cr_assert_eq(EINVAL, errno3);
// }

// Test(queue, test_queue_pop_throws_error_when_empty) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     struct queue_entry entry;
//     queue_init(&queue);

//     // act
//     int res = queue_pop(&queue, &entry);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(ENODATA, errno);

//     // cleanup
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_pop_success_when_size_is_one) {
//     // arrange
//     errno = 0;
//     struct queue queue;
//     queue_init(&queue);

//     struct queue_entry entry;
//     entry.data = "Hello, World!";
//     entry.size = strlen(entry.data);
//     entry.timestamp = 0x1000;
//     queue_push(&queue, &entry);

//     // act
//     int res = queue_pop(&queue, &entry);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_null(queue.head);
//     cr_assert_null(queue.tail);
//     cr_assert_arr_eq(entry.data, "Hello, World!", 13);
//     cr_assert_eq(entry.size, 13);
//     cr_assert_eq(entry.timestamp, 0x1000);

//     // cleanup
//     free(entry.data);
//     queue_destroy(&queue);
// }

// Test(queue, test_queue_pop_success_when_size_is_greater_than_one) {
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
//     int res = queue_pop(&queue, &entry);
//     struct queue_node *node1 = queue.head;
//     struct queue_node *node2 = node1->next;
//     struct queue_node *node3 = node2->next;

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_arr_eq(entry.data, "Hello", 5);
//     cr_assert_eq(entry.size, 5);
//     cr_assert_eq(entry.timestamp, 0x1000);

//     cr_assert_arr_eq(node1->entry.data, ", ", 2);
//     cr_assert_eq(node1->entry.size, 2);
//     cr_assert_eq(node1->entry.timestamp, 0x2000);

//     cr_assert_arr_eq(node2->entry.data, "World", 5);
//     cr_assert_eq(node2->entry.size, 5);
//     cr_assert_eq(node2->entry.timestamp, 0x3000);

//     cr_assert_arr_eq(node3->entry.data, "!", 1);
//     cr_assert_eq(node3->entry.size, 1);
//     cr_assert_eq(node3->entry.timestamp, 0x4000);

//     // cleanup
//     free(entry.data);
//     queue_destroy(&queue);
// }

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

// Test(queue, test_queue_destroy_throws_error_when_invalid_args) {
//     // act
//     errno = 0;
//     int res = queue_destroy(NULL);

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(EINVAL, errno);
// }

// Test(queue, test_queue_destroy) {
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
//     int res = queue_destroy(&queue);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_null(queue.head);
//     cr_assert_null(queue.tail);
// }
