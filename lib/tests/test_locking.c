// Testing concurrency deterministically is difficult, so the developer should
// run this test suite multiple times

// This test suite requires that a test ZooKeeper server be running at
// 127.0.0.1:2182

#include "messageq/constants.h"
#include "messageq/locking.h"
#include "messageq/test.h"
#include "messageq/util.h"
#include "messageq/zookeeper.h"

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <uuid/uuid.h>

#define TEST_ZOOKEEPER_HOST "127.0.0.1:2182"

static zhandle_t *zh;

int test_acquire_distributed_lock_throws_when_invalid_args() {
    // act & assert
    assert(acquire_distributed_lock(NULL, NULL) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(acquire_distributed_lock(NULL, zh) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(acquire_distributed_lock("/utest-lock", NULL) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(acquire_distributed_lock("", zh) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_acquire_distributed_lock_throws_when_lock_not_found() {
    // act & assert
    assert(acquire_distributed_lock("/utest-lock", zh) < 0);
    assert(errno == ENODATA);
    return 0;
}

int test_release_distributed_lock_throws_when_invalid_args() {
    // act & assert
    assert(release_distributed_lock(NULL, NULL) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(release_distributed_lock(NULL, zh) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(release_distributed_lock("/utest-lock", NULL) < 0);
    assert(errno == EINVAL);

    // arrange
    errno = 0;

    // act & assert
    assert(release_distributed_lock("", zh) < 0);
    assert(errno == EINVAL);
    return 0;
}

int test_release_distributed_lock_throws_when_lock_not_found() {
    // act & assert
    assert(release_distributed_lock("/utest-lock", zh) < 0);
    assert(errno == ENODATA);
    return 0;
}

// TODO: more complex example where somebody else is holding the lock
int test_release_distributed_lock_throws_when_caller_not_holding_lock() {
    // arrange
    zoo_create(zh, "/utest-lock", NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
               ZOO_PERSISTENT, NULL, 0);

    // act & assert
    assert(release_distributed_lock("/utest-lock", zh) < 0);
    assert(errno == EPERM);

    // teardown
    zoo_delete(zh, "/utest-lock", -1);
    return 0;
}

static int counter = 0;
struct targ {
    int result;
    int _errno;
};

static void *thread(void *arg) {
    struct targ *targ = (struct targ *)arg;
    zhandle_t *zzh = zoo_init(TEST_ZOOKEEPER_HOST);
    if (!zzh) {
        dprintf("thread zoo_init() failed: %s\n", strerror(errno));
        targ->result = -1;
        targ->_errno = errno;
        return NULL;
    }

    int res = acquire_distributed_lock("/utest-lock", zzh);
    if (res < 0) {
        dprintf("acquire() failed: %s\n", strerror(errno));
        targ->result = -1;
        targ->_errno = errno;
        return NULL;
    }
    dprintf("thread %ld acquired lock\n", pthread_self());

    counter++;

    res = release_distributed_lock("/utest-lock", zzh);
    if (res < 0) {
        dprintf("release() failed: %s\n", strerror(errno));
        targ->result = -1;
        targ->_errno = errno;
        return NULL;
    }
    dprintf("thread %ld released lock\n", pthread_self());

    zookeeper_close(zzh);
    targ->result = 0;
    targ->_errno = 0;
    return NULL;
}

int test_no_data_race() {
    // arrange
    zoo_create(zh, "/utest-lock", NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
               ZOO_PERSISTENT, NULL, 0);

    // act
    struct targ targ[32];
    pthread_t tid[32];
    for (int i = 0; i < arrlen(tid); i++) {
        pthread_create(&tid[i], NULL, thread, &targ[i]);
    }
    for (int i = 0; i < arrlen(tid); i++) {
        pthread_join(tid[i], NULL);
        assert(targ[i].result >= 0);
        assert(!targ[i]._errno);
    }

    // assert
    assert(counter == 32);

    // teardown
    zoo_delete(zh, "/utest-lock", -1);
    return 0;
}

static void *double_acquire_thread(void *arg) {
    struct targ *targ = (struct targ *)arg;

    zhandle_t *zzh = zoo_init(TEST_ZOOKEEPER_HOST);
    if (!zzh) {
        dprintf("acquire_thread zoo_init() failed: %s\n", strerror(errno));
        targ->result = -1;
        targ->_errno = errno;
        return NULL;
    }

    if (acquire_distributed_lock("/utest-lock", zzh) < 0) {
        dprintf("acquire_thread acquire() failed: %s\n", strerror(errno));
        targ->result = -1;
        targ->_errno = errno;
        return NULL;
    }

    targ->result = 0;
    targ->_errno = 0;
    return NULL;
}

int test_deadlock_double_acquire() {
    // arrange
    zoo_create(zh, "/utest-lock", NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
               ZOO_PERSISTENT, NULL, 0);
    acquire_distributed_lock("/utest-lock", zh);

    // act
    pthread_t tid;
    struct targ targ = {0};
    pthread_create(&tid, NULL, double_acquire_thread, &targ);

    struct timeval tv;
    struct timespec ts;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + 15;
    ts.tv_nsec = 0;

    // assert
    assert(pthread_timedjoin_np(tid, NULL, &ts) == ETIMEDOUT);
    assert(!targ.result);
    assert(!targ._errno);

    struct String_vector lock_nodes;
    zoo_get_children(zh, "/utest-lock", 0, &lock_nodes);
    assert(lock_nodes.count == 2);

    char path[MAX_PATH_LEN + 1];
    snprintf(path, sizeof path, "/utest-lock/%s", lock_nodes.data[0]);
    char buf1[512];
    int buf1_len = sizeof buf1;
    zoo_get(zh, path, 0, buf1, &buf1_len, NULL);

    snprintf(path, sizeof path, "/utest-lock/%s", lock_nodes.data[1]);
    char buf2[512];
    int buf2_len = sizeof buf2;
    zoo_get(zh, path, 0, buf2, &buf2_len, NULL);

    assert(memcmp(buf1, buf2, sizeof(uuid_t)));

    // teardown
    zoo_deleteall(zh, "/utest-lock", -1);
    return 0;
}

void setup() {
    errno = 0;
    zh = zoo_init(TEST_ZOOKEEPER_HOST);
}

void teardown() { zookeeper_close(zh); }

struct test_case tests[] = {
    {"test_acquire_distributed_lock_throws_when_invalid_args", setup, teardown,
     test_acquire_distributed_lock_throws_when_invalid_args},
    {"test_acquire_distributed_lock_throws_when_lock_not_found", setup,
     teardown, test_acquire_distributed_lock_throws_when_lock_not_found},

    {"test_release_distributed_lock_throws_when_invalid_args", setup, teardown,
     test_release_distributed_lock_throws_when_invalid_args},
    {"test_release_distributed_lock_throws_when_lock_not_found", setup,
     teardown, test_release_distributed_lock_throws_when_lock_not_found},
    {"test_release_distributed_lock_throws_when_caller_not_holding_lock", setup,
     teardown,
     test_release_distributed_lock_throws_when_caller_not_holding_lock},

    {"test_no_data_race", setup, teardown, test_no_data_race},
    {"test_deadlock_double_acquire", setup, teardown,
     test_deadlock_double_acquire}
    // {"test_deadlock_two_threads"}
    // TODO: test success for both operations
    // TODO: test ordered locking
};

struct test_suite suite = {
    .name = "test_locking", .setup = NULL, .teardown = NULL};

int main() { run_suite(); }
