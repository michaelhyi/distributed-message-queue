#include "queue.h"

#include <messageq/test.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_queue_init_success() {
    // arrange
    errno = 0;
    struct queue queue;

    // act
    queue_init(&queue);

    // assert
    assert(!queue.head);
    assert(!queue.tail);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_destroy_success() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    entry.id = 1;
    entry.data = "Hello";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 2;
    entry.data = ", ";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 3;
    entry.data = "World";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 4;
    entry.data = "!";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    // act
    queue_destroy(&queue);

    // assert
    assert(!queue.head);
    assert(!queue.tail);
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
    entry.id = 1;
    entry.data = "Hello, World!";
    entry.size = strlen(entry.data);

    // act & assert
    assert(queue_push(&queue, &entry) >= 0);
    assert(!errno);
    assert(queue.head == queue.tail);
    assert(queue.head->entry.id == entry.id);
    assert(memcmp(queue.head->entry.data, entry.data, entry.size) == 0);
    assert(queue.head->entry.size == entry.size);
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
    entry1.id = 1;
    entry1.data = "Hello, ";
    entry1.size = strlen(entry1.data);

    queue_push(&queue, &entry1);

    struct queue_entry entry2;
    entry2.id = 2;
    entry2.data = "World!";
    entry2.size = strlen(entry2.data);

    // act & assert
    assert(queue_push(&queue, &entry2) >= 0);
    assert(!errno);
    assert(queue.head != queue.tail);
    assert(queue.head->entry.id == entry1.id);
    assert(memcmp(queue.head->entry.data, entry1.data, entry1.size) == 0);
    assert(queue.head->entry.size == entry1.size);
    assert(queue.tail == queue.head->next);

    assert(queue.tail->entry.id == entry2.id);
    assert(memcmp(queue.tail->entry.data, entry2.data, entry2.size) == 0);
    assert(queue.tail->entry.size == entry2.size);
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
    entry1.id = 1;
    entry1.data = "Hello";
    entry1.size = strlen(entry1.data);
    queue_push(&queue, &entry1);

    struct queue_entry entry2;
    entry2.id = 2;
    entry2.data = ", ";
    entry2.size = strlen(entry2.data);
    queue_push(&queue, &entry2);

    struct queue_entry entry3;
    entry3.id = 3;
    entry3.data = "World";
    entry3.size = strlen(entry3.data);
    queue_push(&queue, &entry3);

    struct queue_entry entry4;
    entry4.data = "!";
    entry4.size = strlen(entry4.data);
    entry4.id = 4;

    // act & assert
    assert(queue_push(&queue, &entry4) >= 0);
    assert(!errno);

    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;
    struct queue_node *node4 = node3->next;

    assert(queue.head != queue.tail);
    assert(queue.tail->entry.id == entry4.id);
    assert(memcmp(queue.tail->entry.data, entry4.data, entry4.size) == 0);
    assert(queue.tail->entry.size == entry4.size);
    assert(!queue.tail->next);

    assert(node1->entry.id == entry1.id);
    assert(memcmp(node1->entry.data, entry1.data, entry1.size) == 0);
    assert(node1->entry.size == entry1.size);

    assert(node2->entry.id == entry2.id);
    assert(memcmp(node2->entry.data, entry2.data, entry2.size) == 0);
    assert(node2->entry.size == entry2.size);

    assert(node3->entry.id == entry3.id);
    assert(memcmp(node3->entry.data, entry3.data, entry3.size) == 0);
    assert(node3->entry.size == entry3.size);

    assert(node4->entry.id == entry4.id);
    assert(memcmp(node4->entry.data, entry4.data, entry4.size) == 0);
    assert(node4->entry.size == entry4.size);

    // teardown
    queue_destroy(&queue);
    return 0;
}

int test_queue_pop_throws_error_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(!queue_pop(NULL));
    assert(errno == EINVAL);
    return 0;
}

int test_queue_pop_throws_error_when_empty() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    // act & assert
    assert(!queue_pop(&queue));
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
    entry.id = 1;
    entry.data = "Hello, World!";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    // act
    struct queue_entry *popped = queue_pop(&queue);

    // assert
    assert(popped);
    assert(!errno);
    assert(!queue.head);
    assert(!queue.tail);
    assert(popped->id == 1);
    assert(memcmp(popped->data, "Hello, World!", 13) == 0);
    assert(popped->size == 13);

    // teardown
    free(popped->data);
    free(popped);
    queue_destroy(&queue);
    return 0;
}

int test_queue_pop_success_when_size_is_greater_than_one() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    entry.id = 1;
    entry.data = "Hello";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 2;
    entry.data = ", ";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 3;
    entry.data = "World";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 4;
    entry.data = "!";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    // act
    struct queue_entry *popped = queue_pop(&queue);

    // assert
    assert(popped);
    assert(!errno);

    struct queue_node *node1 = queue.head;
    struct queue_node *node2 = node1->next;
    struct queue_node *node3 = node2->next;

    assert(popped->id == 1);
    assert(memcmp(popped->data, "Hello", 5) == 0);
    assert(popped->size == 5);

    assert(node1->entry.id == 2);
    assert(memcmp(node1->entry.data, ", ", 2) == 0);
    assert(node1->entry.size == 2);

    assert(node2->entry.id == 3);
    assert(memcmp(node2->entry.data, "World", 5) == 0);
    assert(node2->entry.size == 5);

    assert(node3->entry.id == 4);
    assert(memcmp(node3->entry.data, "!", 1) == 0);
    assert(node3->entry.size == 1);

    // teardown
    free(popped->data);
    free(popped);
    queue_destroy(&queue);
    return 0;
}

int test_queue_peek_id_throws_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(queue_peek_id(NULL) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_queue_peek_id_throws_when_empty() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    // act & assert
    assert(queue_peek_id(&queue) < 0);
    assert(errno == ENODATA);
    return 0;
}

int test_queue_peek_id_success() {
    // arrange
    errno = 0;
    struct queue queue;
    queue_init(&queue);

    struct queue_entry entry;

    entry.id = 1;
    entry.data = "Hello";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 2;
    entry.data = ", ";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 3;
    entry.data = "World";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    entry.id = 4;
    entry.data = "!";
    entry.size = strlen(entry.data);
    queue_push(&queue, &entry);

    // act
    assert(queue_peek_id(&queue) == 1);
    assert(!errno);

    // teardown
    queue_destroy(&queue);
    return 0;
}

struct test_case tests[] = {
    {"test_queue_init_success", NULL, NULL, test_queue_init_success},
    {"test_queue_destroy_success", NULL, NULL, test_queue_destroy_success},
    {"test_queue_push_throws_error_when_invalid_args", NULL, NULL,
     test_queue_push_throws_error_when_invalid_args},
    {"test_queue_push_success_when_empty", NULL, NULL,
     test_queue_push_success_when_empty},
    {"test_queue_push_success_when_size_is_one", NULL, NULL,
     test_queue_push_success_when_size_is_one},
    {"test_queue_push_success_when_size_is_greater_than_one", NULL, NULL,
     test_queue_push_success_when_size_is_greater_than_one},
    {"test_queue_pop_throws_error_when_invalid_args", NULL, NULL,
     test_queue_pop_throws_error_when_invalid_args},
    {"test_queue_pop_throws_error_when_empty", NULL, NULL,
     test_queue_pop_throws_error_when_empty},
    {"test_queue_pop_success_when_size_is_one", NULL, NULL,
     test_queue_pop_success_when_size_is_one},
    {"test_queue_pop_success_when_size_is_greater_than_one", NULL, NULL,
     test_queue_pop_success_when_size_is_greater_than_one},
    {"test_queue_peek_id_throws_when_invalid_args", NULL, NULL,
     test_queue_peek_id_throws_when_invalid_args},
    {"test_queue_peek_id_throws_when_empty", NULL, NULL,
     test_queue_peek_id_throws_when_empty},
    {"test_queue_peek_id_success", NULL, NULL, test_queue_peek_id_success}};

struct test_suite suite = {
    .name = "test_queue", .setup = NULL, .teardown = NULL};

int main() { run_suite(); }
