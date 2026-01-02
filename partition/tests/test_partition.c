#include "partition.h"

#include <messageq/network.h>
#include <messageq/test.h>
#include <messageq/zookeeper.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define TEST_ZOOKEEPER_HOST "127.0.0.1:2182"

static zhandle_t *zh;

struct targ {
    int result;
    int _errno;
};

void *partition_thread(void *arg) {
    struct targ *targ = (struct targ *)arg;
    targ->result = start_partition(TEST_ZOOKEEPER_HOST);
    targ->_errno = errno;
    return NULL;
}

int test_start_partition_registers_with_zookeeper() {
    // arrange
    errno = 0;

    // act
    pthread_t tid;
    struct targ arg = {0};
    pthread_create(&tid, NULL, partition_thread, &arg);

    struct timeval tv;
    struct timespec ts = {0};
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + 10;
    pthread_mutex_lock(&server_lock);
    while (!server_running) {
        assert(pthread_cond_timedwait(&server_running_cond, &server_lock,
                                      &ts) != ETIMEDOUT);
    }
    pthread_mutex_unlock(&server_lock);

    char expected_host[512] = "127.0.0.1:";
    sprintf(expected_host, "127.0.0.1:%d", server_port);

    int found = 0;
    int retries = 5;
    while (!found && retries-- > 0) {
        sleep(1);

        struct String_vector partitions;
        zoo_get_children(zh, "/partitions", 0, &partitions);

        for (int i = 0; i < partitions.count; i++) {
            char path[512] = "/partitions/";
            strcat(path, partitions.data[i]);

            char buf[512];
            int buflen = sizeof buf;
            zoo_get(zh, path, 0, buf, &buflen, NULL);

            if (strcmp(buf, expected_host) == 0) {
                found = 1;
                break;
            }
        }
    }

    // assert
    assert(found);
    assert(!errno);

    // teardown
    pthread_kill(tid, SIGTERM);
    pthread_join(tid, NULL);

    // assert
    assert(arg.result >= 0);
    assert(!arg._errno);
    return 0;
}

int test_start_partition_becomes_leader_when_assigned_to_shard() { return -1; }

int test_start_partition_becomes_replica_when_assigned_to_shard() { return -1; }

int test_start_partition_becomes_leader_when_prev_leader_dies() { return -1; }

void setup() { zh = zoo_init(TEST_ZOOKEEPER_HOST); }

void teardown() { zookeeper_close(zh); }

struct test_case tests[] = {
    {NULL, NULL, test_start_partition_registers_with_zookeeper},
    {NULL, NULL, test_start_partition_becomes_leader_when_assigned_to_shard},
    {NULL, NULL, test_start_partition_becomes_replica_when_assigned_to_shard},
    {NULL, NULL, test_start_partition_becomes_leader_when_prev_leader_dies}};

struct test_suite suite = {
    .name = "test_partition", .setup = setup, .teardown = teardown};

int main() { run_suite(); }

// TODO: fix tests post refactorign

// #ifdef DEBUG
// TestSuite(partition);
// #else
// TestSuite(partition, .timeout = 10);
// #endif

// Test(partition, test_partition_destroy_success) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);

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
//     partition_destroy();

//     // assert
//     cr_assert_eq(errno, 0);
//     cr_assert_null(queue.head);
//     cr_assert_null(queue.tail);
// }

// Test(partition, test_handle_dmqp_message_throws_error_on_invalid_args) {
//     // arrange
//     errno = 0;
//     struct dmqp_header header1 = {.method = DMQP_HEARTBEAT,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = 14};
//     struct dmqp_message message1 = {.header = header1, .payload = NULL};

//     struct dmqp_header header2 = {.method = DMQP_HEARTBEAT,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = MAX_PAYLOAD_LENGTH + 14};
//     struct dmqp_message message2 = {.header = header2,
//                                     .payload = "Hello, World!"};

//     struct dmqp_header header3 = {.method = DMQP_HEARTBEAT,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = 14};
//     struct dmqp_message message3 = {.header = header3,
//                                     .payload = "Hello, World!"};

//     // act
//     int res1 = handle_dmqp_message(message1, 0);
//     int errno1 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res2 = handle_dmqp_message(message2, 0);
//     int errno2 = errno;

//     // arrange
//     errno = 0;

//     // act
//     int res3 = handle_dmqp_message(message3, -1);
//     int errno3 = errno;

//     // assert
//     cr_assert(res1 < 0);
//     cr_assert(res2 < 0);
//     cr_assert(res3 < 0);
//     cr_assert_eq(errno1, EINVAL);
//     cr_assert_eq(errno2, EINVAL);
//     cr_assert_eq(errno3, EINVAL);
// }

// Test(partition,
//      test_handle_dmqp_message_throws_error_on_unknown_method_received) {
//     // arrange
//     errno = 0;
//     struct dmqp_header header = {.method = PEEK + 0xFFFF,
//                                  .flags = 0,
//                                  .timestamp = 0,
//                                  .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, ENOSYS);
//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, ENOSYS);
//     cr_assert_eq(res_header.timestamp, 0);
//     cr_assert_eq(res_header.length, 0);

//     // cleanup
//     free(message.payload);
//     close(mock_sockets[0]);
// }

// Test(partition, test_handle_dmqp_message_throws_error_on_response_received) {
//     // arrange
//     errno = 0;
//     struct dmqp_header header = {.method = RESPONSE,
//                                  .flags = 0,
//                                  .timestamp = 0,
//                                  .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, EPROTO);
//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, EPROTO);
//     cr_assert_eq(res_header.timestamp, 0);
//     cr_assert_eq(res_header.length, 0);

//     // cleanup
//     free(message.payload);
//     close(mock_sockets[0]);
// }

// Test(partition, test_handle_dmqp_message_success_on_heartbeat_received) {
//     // arrange
//     errno = 0;
//     struct dmqp_header header = {.method = DMQP_HEARTBEAT,
//                                  .flags = 0,
//                                  .timestamp = 0,
//                                  .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);
//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, 0);
//     cr_assert_eq(res_header.timestamp, 0);
//     cr_assert_eq(res_header.length, 0);

//     // cleanup
//     free(message.payload);
//     close(mock_sockets[0]);
// }

// Test(partition, test_handle_dmqp_message_success_on_push_received) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);
//     char *data1 = "Hello";
//     char *data2 = ", ";
//     char *data3 = "World!";

//     struct dmqp_header header1 = {.method = PUSH,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = strlen(data1)};
//     struct dmqp_message message1 = {.header = header1, .payload = data1};

//     struct dmqp_header header2 = {.method = PUSH,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = strlen(data2)};
//     struct dmqp_message message2 = {.header = header2, .payload = data2};

//     struct dmqp_header header3 = {.method = PUSH,
//                                   .flags = 0,
//                                   .timestamp = 0,
//                                   .length = strlen(data3)};
//     struct dmqp_message message3 = {.header = header3, .payload = data3};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res1 = handle_dmqp_message(message1, mock_sockets[1]);
//     int errno1 = errno;
//     close(mock_sockets[1]);

//     struct dmqp_header res_header1;
//     read_all(mock_sockets[0], &res_header1, sizeof(struct
//     dmqp_header)); close(mock_sockets[0]);

//     // arrange
//     errno = 0;
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res2 = handle_dmqp_message(message2, mock_sockets[1]);
//     int errno2 = errno;
//     close(mock_sockets[1]);

//     struct dmqp_header res_header2;
//     read_all(mock_sockets[0], &res_header2, sizeof(struct
//     dmqp_header)); close(mock_sockets[0]);

//     // arrange
//     errno = 0;
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     int res3 = handle_dmqp_message(message3, mock_sockets[1]);
//     int errno3 = errno;
//     close(mock_sockets[1]);

//     struct dmqp_header res_header3;
//     read_all(mock_sockets[0], &res_header3, sizeof(struct
//     dmqp_header)); close(mock_sockets[0]);

//     // assert
//     cr_assert(res1 >= 0);
//     cr_assert(res2 >= 0);
//     cr_assert(res3 >= 0);
//     cr_assert_eq(errno1, 0);
//     cr_assert_eq(errno2, 0);
//     cr_assert_eq(errno3, 0);

//     cr_assert_eq(res_header1.method, RESPONSE);
//     cr_assert_eq(res_header1.flags, 0);
//     cr_assert_eq(res_header1.timestamp, 0);
//     cr_assert_eq(res_header1.length, 0);

//     cr_assert_eq(res_header2.method, RESPONSE);
//     cr_assert_eq(res_header2.flags, 0);
//     cr_assert_eq(res_header2.timestamp, 0);
//     cr_assert_eq(res_header2.length, 0);

//     cr_assert_eq(res_header3.method, RESPONSE);
//     cr_assert_eq(res_header3.flags, 0);
//     cr_assert_eq(res_header3.timestamp, 0);
//     cr_assert_eq(res_header3.method, RESPONSE);

//     // cleanup
//     partition_destroy();
// }

// Test(partition,
//      test_handle_dmqp_message_throws_error_when_pop_on_empty_partition) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);

//     struct dmqp_header header = {
//         .method = POP, .flags = 0, .timestamp = 0, .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     handle_dmqp_message(message, mock_sockets[1]);
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, ENODATA);

//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, ENODATA);
//     cr_assert_eq(res_header.timestamp, 0);
//     cr_assert_eq(res_header.length, 0);

//     // cleanup
//     close(mock_sockets[0]);
//     partition_destroy();
// }

// Test(partition, test_handle_dmqp_message_success_on_pop_received) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);
//     char *data1 = "Hello";
//     char *data2 = ", ";
//     char *data3 = "World!";

//     queue_push(&queue, data1, strlen(data1));
//     queue_push(&queue, data2, strlen(data2));
//     queue_push(&queue, data3, strlen(data3));

//     struct dmqp_header header = {
//         .method = POP, .flags = 0, .timestamp = 0, .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     long expected_timestamp = queue.head->entry.timestamp;

//     // act
//     handle_dmqp_message(message, mock_sockets[1]);
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     void *payload = malloc(res_header.length);
//     read_all(mock_sockets[0], payload, res_header.length);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);

//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, 0);
//     cr_assert_eq(res_header.timestamp, expected_timestamp);
//     cr_assert_eq(res_header.length, strlen(data1));
//     cr_assert_arr_eq(payload, data1, res_header.length);

//     // cleanup
//     free(payload);
//     close(mock_sockets[0]);
//     partition_destroy();
// }

// Test(partition,
//      test_handle_dmqp_message_throws_error_when_peek_on_empty_partition) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);

//     struct dmqp_header header = {
//         .method = PEEK, .flags = 0, .timestamp = 0, .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     // act
//     handle_dmqp_message(message, mock_sockets[1]);
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     // assert
//     cr_assert(res < 0);
//     cr_assert_eq(errno, ENODATA);

//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, ENODATA);
//     cr_assert_eq(res_header.timestamp, 0);
//     cr_assert_eq(res_header.length, 0);

//     // cleanup
//     close(mock_sockets[0]);
//     partition_destroy();
// }

// Test(partition, test_handle_dmqp_message_success_on_peek_received) {
//     // arrange
//     errno = 0;
//     queue_init(&queue);
//     pthread_mutex_init(&queue_lock, NULL);
//     char *data1 = "Hello";
//     char *data2 = ", ";
//     char *data3 = "World!";

//     queue_push(&queue, data1, strlen(data1));
//     queue_push(&queue, data2, strlen(data2));
//     queue_push(&queue, data3, strlen(data3));

//     struct dmqp_header header = {
//         .method = PEEK, .flags = 0, .timestamp = 0, .length = 0};
//     struct dmqp_message message = {.header = header, .payload = NULL};

//     int mock_sockets[2];
//     socketpair(AF_UNIX, SOCK_STREAM, 0, mock_sockets);

//     long expected_timestamp = queue.head->entry.timestamp;

//     // act
//     handle_dmqp_message(message, mock_sockets[1]);
//     int res = handle_dmqp_message(message, mock_sockets[1]);
//     close(mock_sockets[1]);

//     struct dmqp_header res_header;
//     read_all(mock_sockets[0], &res_header, sizeof(struct
//     dmqp_header));

//     void *payload = malloc(res_header.length);
//     read_all(mock_sockets[0], payload, res_header.length);

//     // assert
//     cr_assert(res >= 0);
//     cr_assert_eq(errno, 0);

//     cr_assert_eq(res_header.method, RESPONSE);
//     cr_assert_eq(res_header.flags, 0);
//     cr_assert_eq(res_header.timestamp, expected_timestamp);
//     cr_assert_eq(res_header.length, strlen(data1));
//     cr_assert_arr_eq(payload, data1, res_header.length);

//     // cleanup
//     free(payload);
//     close(mock_sockets[0]);
//     partition_destroy();
// }
