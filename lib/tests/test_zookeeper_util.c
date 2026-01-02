// This test suite requires that a test ZooKeeper server be running at
// 127.0.0.1:2182

#include "messageq/test.h"
#include "messageq/util.h"
#include "zookeeper_util.h"

#include <errno.h>

#define TEST_ZOOKEEPER_SERVER_HOST "127.0.0.1:2182"

static zhandle_t *zh;

int test_zoo_init_throws_when_invalid_args() {
    // arrange
    errno = 0;

    // act & assert
    assert(!zoo_init(NULL));
    assert(errno == EINVAL);

    return 0;
}

int test_zoo_init_throws_when_nonexistent_host() {
    // arrange
    errno = 0;

    // act & assert
    assert(!zoo_init("127.0.0.1:3181"));
    assert(errno == ETIMEDOUT);

    return 0;
}

int test_zoo_init_success() {
    // arrange
    errno = 0;

    // act
    zhandle_t *zh = zoo_init(TEST_ZOOKEEPER_SERVER_HOST);

    // assert
    assert(zh);
    assert(!errno);

    // teardown
    zookeeper_close(zh);
    return 0;
}

int test_zoo_deleteall_throws_when_invalid_args() {
    // arrange
    char *path = "/utest";
    struct {
        zhandle_t *zh;
        const char *path;
        int version;
    } cases[] = {
        {NULL, NULL, -1},
        {NULL, path, -1},
        {zh, NULL, -1},
    };

    for (int i = 0; i < arrlen(cases); i++) {
        // arrange
        errno = 0;

        // act & assert
        assert(zoo_deleteall(cases[i].zh, cases[i].path, cases[i].version) < 0);
        assert(errno == EINVAL);
    }
    return 0;
}

int test_zoo_deleteall_throws_when_znode_does_not_exist() {
    // arrange
    char *path = "/utest";

    // act & assert
    assert(zoo_deleteall(zh, path, -1) < 0);
    assert(errno == ENODATA);
    return 0;
}

int test_zoo_deleteall_success_when_no_children() {
    // arrange
    char *path = "/utest";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);
    char buffer[512];
    int buffer_len = sizeof(buffer);

    // act & assert
    assert(zoo_deleteall(zh, path, -1) >= 0);
    assert(!errno);
    assert(zoo_get(zh, path, 0, buffer, &buffer_len, NULL) == ZNONODE);
    return 0;
}

int test_zoo_deleteall_success() {
    // arrange
    char *path = "/utest";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    path = "/utest/child1";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    path = "/utest/child2";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    path = "/utest/child2/grandchild1";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    path = "/utest/child2/grandchild2";
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    path = "/utest";
    char buffer[512];
    int buffer_len = sizeof(buffer);

    // act
    assert(zoo_deleteall(zh, path, -1) >= 0);
    assert(!errno);
    assert(zoo_get(zh, path, 0, buffer, &buffer_len, NULL) == ZNONODE);
    return 0;
}

void setup() {
    errno = 0;
    zoo_set_debug_level(0);
    zh = zookeeper_init(TEST_ZOOKEEPER_SERVER_HOST, NULL, 10000, 0, 0, 0);
}

void teardown() { zookeeper_close(zh); }

struct test_case tests[] = {
    {NULL, NULL, test_zoo_init_throws_when_invalid_args},
    {NULL, NULL, test_zoo_init_throws_when_nonexistent_host},
    {NULL, NULL, test_zoo_init_success},

    {setup, teardown, test_zoo_deleteall_throws_when_invalid_args},
    {setup, teardown, test_zoo_deleteall_throws_when_znode_does_not_exist},
    {setup, teardown, test_zoo_deleteall_success_when_no_children},
    {setup, teardown, test_zoo_deleteall_success}};

struct test_suite suite = {
    .name = "test_zookeeper_util", .setup = NULL, .teardown = NULL};

int main() { run_suite(); }
