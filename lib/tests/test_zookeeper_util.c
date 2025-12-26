#include "zookeeper_util.h"

#include <criterion/criterion.h>
#include <errno.h>

#ifdef DEBUG
TestSuite(zookeeper_util);
#else
TestSuite(zookeeper_util, .timeout = 10);
#endif

struct args {
    zhandle_t *zh;
    const char *path;
    int version;
};

static void watcher(zhandle_t *zzh, int type, int state, const char *path,
                    void *watcherCtx) {
    (void)zzh;
    (void)type;
    (void)state;
    (void)path;
    (void)watcherCtx;
}

Test(zookeeper_util, test_zoo_deleteall_throws_when_invalid_args) {
    // arrange
    errno = 0;
    zhandle_t *zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

    char path[512] = "/";
    strcat(path, criterion_current_test->name);

    struct args test_cases[] = {
        {NULL, NULL, -1},
        {NULL, path, -1},
        {zh, NULL, -1},
    };

    for (int i = 0; i < (int)(sizeof(test_cases) / sizeof(test_cases[0]));
         i++) {
        // arrange
        errno = 0;
        struct args test_case = test_cases[i];

        // act
        int res =
            zoo_deleteall(test_case.zh, test_case.path, test_case.version);

        // assert
        cr_assert(res < 0);
        cr_assert_eq(errno, EINVAL);
    }
}

Test(zookeeper_util, test_zoo_deleteall_throws_when_znode_does_not_exist) {
    // arrange
    errno = 0;
    zhandle_t *zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

    char path[512] = "/";
    strcat(path, criterion_current_test->name);

    // act
    int res = zoo_deleteall(zh, path, -1);

    // assert
    cr_assert(res < 0);
    cr_assert_eq(errno, ENODATA);
}

Test(zookeeper_util, test_zoo_deleteall_success_when_no_children) {
    // arrange
    errno = 0;
    zhandle_t *zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

    char path[512] = "/";
    strcat(path, criterion_current_test->name);
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    char buffer[512];
    int buffer_len = sizeof(buffer);

    // act
    int res = zoo_deleteall(zh, path, -1);
    int rc = zoo_get(zh, path, 0, buffer, &buffer_len, NULL);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(rc, ZNONODE);
}

Test(zookeeper_util, test_zoo_deleteall_success) {
    // arrange
    errno = 0;
    zhandle_t *zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);

    char path[512] = "/";
    strcat(path, criterion_current_test->name);
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    strcat(path, "/child1");
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/%s/child2", criterion_current_test->name);
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/%s/child2/grandchild1",
             criterion_current_test->name);
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/%s/child2/grandchild2",
             criterion_current_test->name);
    zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
               0);

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path), "/%s", criterion_current_test->name);

    char buffer[512];
    int buffer_len = sizeof(buffer);

    // act
    int res = zoo_deleteall(zh, path, -1);
    int rc = zoo_get(zh, path, 0, buffer, &buffer_len, NULL);

    // assert
    cr_assert(res >= 0);
    cr_assert_eq(errno, 0);
    cr_assert_eq(rc, ZNONODE);
}
