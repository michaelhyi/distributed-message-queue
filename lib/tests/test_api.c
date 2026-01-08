// This test suite requires that a test ZooKeeper server be running at
// 127.0.0.1:2182

#include "messageq/api.h"
#include "messageq/constants.h"
#include "messageq/test.h"
#include "messageq/util.h"
#include "messageq/zookeeper.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

#define TEST_ZOOKEEPER_HOST "127.0.0.1:2182"

static zhandle_t *zh;

int test_client_init_throws_if_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(client_init(NULL) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_client_init_throws_if_client_already_initialized() {
    // arrange
    errno = 0;
    client_init("127.0.0.1:2181");

    // act & assert
    assert(client_init("127.0.0.1:2181") < 0);
    assert(errno == EALREADY);

    // teardown
    client_destroy();
    return 0;
}

int test_client_init_success() {
    // arrange
    errno = 0;

    // act & assert
    assert(client_init("127.0.0.1:2181") >= 0);
    assert(!errno);

    // teardown
    client_destroy();
    return 0;
}

int test_create_topic_throws_if_client_uninitialized() {
    // arrange
    errno = 0;
    struct topic topic = {
        .name = "utest-topic", .shards = 1, .replication_factor = 0};

    // act & assert
    assert(create_topic(&topic) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_create_topic_throws_if_invalid_args() {
    // arrange
    struct topic *tests[] = {
        NULL, &(struct topic){NULL, 0, 0},
        &(struct topic){.name = "___max_topic_name_length_exceeded",
                        .shards = 0,
                        .replication_factor = 0},
        &(struct topic){
            .name = "utest-topic", .shards = 0, .replication_factor = 0},
        &(struct topic){
            .name = "utest-topic", .shards = 1, .replication_factor = 0}};

    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        // arrange
        errno = 0;

        // act
        int res = create_topic(tests[i]);

        // assert
        assert(res < 0);
        assert(errno == EINVAL);
    }

    return 0;
}

int test_create_topic_throws_if_not_enough_partitions() {
    // arrange
    zoo_set(zh, "/partitions/free-count", "4", 1, -1);
    char *partitions[] = {"127.0.0.1:8080", "127.0.0.1:8081", "127.0.0.1:8082",
                          "127.0.0.1:8083"};
    char partition_ids[arrlen(partitions)][MAX_PATH_LEN];
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_create(zh, "/partitions/partition-", partitions[i],
                   strlen(partitions[i]), &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_PERSISTENT_SEQUENTIAL, path, MAX_PATH_LEN);
        // must be persistent during tests due to timeout
    }

    struct topic topic = {
        .name = "utest-topic", .shards = 2, .replication_factor = 3};

    // act
    int res = create_topic(&topic);

    // assert
    assert(res < 0);
    assert(errno == ENODEV);

    // teardown
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_delete(zh, path, -1);
    }
    zoo_set(zh, "/partitions/free-count", "0", 1, -1);
    return 0;
}

int test_create_topic_throws_if_topic_already_exists() {
    // arrange
    zoo_set(zh, "/partitions/free-count", "8", 1, -1);
    char *partitions[] = {"127.0.0.1:8080", "127.0.0.1:8081", "127.0.0.1:8082",
                          "127.0.0.1:8083", "127.0.0.1:8084", "127.0.0.1:8085",
                          "127.0.0.1:8086", "127.0.0.1:8087"};
    char partition_ids[arrlen(partitions)][MAX_PATH_LEN];
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_create(zh, "/partitions/partition-", partitions[i],
                   strlen(partitions[i]), &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_PERSISTENT_SEQUENTIAL, path, MAX_PATH_LEN);
        // must be persistent during tests due to timeout
    }

    zoo_create(zh, "/topics/utest-topic", NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
               ZOO_PERSISTENT, NULL, 0);

    struct topic topic = {
        .name = "utest-topic", .shards = 2, .replication_factor = 3};

    // act
    int res = create_topic(&topic);

    // assert
    assert(res < 0);
    assert(errno == EEXIST);

    // teardown
    zoo_delete(zh, "/topics/utest-topic", -1);
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_delete(zh, path, -1);
    }
    zoo_set(zh, "/partitions/free-count", "0", 1, -1);
    return 0;
}

int test_create_topic_success() {
    // arrange
    zoo_set(zh, "/partitions/free-count", "8", 1, -1);
    char *partitions[] = {"127.0.0.1:8080", "127.0.0.1:8081", "127.0.0.1:8082",
                          "127.0.0.1:8083", "127.0.0.1:8084", "127.0.0.1:8085",
                          "127.0.0.1:8086", "127.0.0.1:8087"};
    char partition_ids[arrlen(partitions)][MAX_PATH_LEN];
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_create(zh, "/partitions/partition-", partitions[i],
                   strlen(partitions[i]), &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_PERSISTENT_SEQUENTIAL, path, MAX_PATH_LEN);
        // must be persistent during tests due to timeout
    }

    struct topic topic = {
        .name = "utest-topic", .shards = 2, .replication_factor = 3};

    // act
    int res = create_topic(&topic);

    // assert
    char free_cnt[16] = {0};
    int free_cnt_len = sizeof free_cnt;
    int rc1 =
        zoo_get(zh, "/partitions/free-count", 0, free_cnt, &free_cnt_len, NULL);

    char buf[512] = {0};
    int buf_len = sizeof buf;
    int rc2 = zoo_get(zh, "/topics/utest-topic", 0, buf, &buf_len, NULL);
    int rc3 =
        zoo_get(zh, "/topics/utest-topic/sequence-id", 0, buf, &buf_len, NULL);
    int rc4 = zoo_get(zh, "/topics/utest-topic/sequence-id/lock", 0, buf,
                      &buf_len, NULL);
    int rc5 = zoo_get(zh, "/topics/utest-topic/shards", 0, buf, &buf_len, NULL);

    struct String_vector shards;
    int rc6 = zoo_get_children(zh, "/topics/utest-topic/shards", 0, &shards);

    assert(res >= 0);
    assert(errno == 0);

    assert(!rc1);
    assert(strcmp(free_cnt, "2") == 0);

    assert(!rc2);
    assert(!rc3);
    assert(!rc4);
    assert(!rc5);

    assert(!rc6);
    assert((unsigned int)shards.count == topic.shards);

    for (int i = 0; i < shards.count; i++) {
        char path[512] = "/topics/utest-topic/shards/";
        strcat(path, shards.data[i]);
        strcat(path, "/partitions");

        struct String_vector replicas;
        int rc = zoo_get_children(zh, path, 0, &replicas);
        assert(rc >= 0);
        assert((unsigned int)replicas.count == topic.replication_factor);

        for (int j = 0; j < replicas.count; j++) {
            snprintf(path, sizeof path,
                     "/topics/utest-topic/shards/%s/partitions/%s",
                     shards.data[i], replicas.data[i]);
            char buf[512] = {0};
            int buf_len = sizeof buf;

            rc = zoo_get(zh, path, 0, buf, &buf_len, NULL);
            assert(!rc);
            buf[buf_len] = '\0';

            char *partition_id = strdup(buf);
            char buffer[512];
            int buffer_len = sizeof buffer;
            rc = zoo_get(zh, partition_id, 0, buffer, &buffer_len, NULL);
            assert(!rc);
            buffer[buffer_len] = '\0';

            strtok(buffer, ";");
            char *assigned_shard = strtok(NULL, ";");
            assert(assigned_shard);

            char expected_assigned_shard[512] = "/topics/utest-topic/shards/";
            strcat(expected_assigned_shard, shards.data[i]);
            assert(strcmp(assigned_shard, expected_assigned_shard) == 0);

            free(partition_id);
        }

        deallocate_String_vector(&replicas);
    }

    // teardown
    deallocate_String_vector(&shards);
    zoo_deleteall(zh, "/topics/utest-topic", -1);
    for (int i = 0; i < arrlen(partitions); i++) {
        char *path = partition_ids[i];
        zoo_delete(zh, path, -1);
    }
    zoo_set(zh, "/partitions/free-count", "0", 1, -1);
    return 0;
}

void setup() {
    errno = 0;
    zh = zoo_init(TEST_ZOOKEEPER_HOST);

    client_init(TEST_ZOOKEEPER_HOST);
}

void teardown() {
    client_destroy();
    zookeeper_close(zh);
}

struct test_case tests[] = {
    {"test_client_init_throws_if_invalid_args", NULL, NULL,
     test_client_init_throws_if_invalid_args},
    {"test_client_init_throws_if_client_already_initialized", NULL, NULL,
     test_client_init_throws_if_client_already_initialized},
    {"test_client_init_success", NULL, NULL, test_client_init_success},
    {"test_create_topic_throws_if_client_uninitialized", NULL, NULL,
     test_create_topic_throws_if_client_uninitialized},
    {"test_create_topic_throws_if_invalid_args", setup, teardown,
     test_create_topic_throws_if_invalid_args},
    {"test_create_topic_throws_if_not_enough_partitions", setup, teardown,
     test_create_topic_throws_if_not_enough_partitions},
    {"test_create_topic_throws_if_topic_already_exists", setup, teardown,
     test_create_topic_throws_if_topic_already_exists},
    {"test_create_topic_success", setup, teardown, test_create_topic_success}};

struct test_suite suite = {.name = "test_api", .setup = NULL, .teardown = NULL};

int main() { run_suite(); }
