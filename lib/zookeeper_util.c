#include "zookeeper_util.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t connected_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t connected_cond = PTHREAD_COND_INITIALIZER;
static int session = 0;
static int connected = 0;
static int expired = 0;

static void watcher(zhandle_t *zzh, int type, int state, const char *path,
                    void *watcherCtx) {
    (void)zzh;
    (void)path;
    (void)watcherCtx;

    if (type == ZOO_SESSION_EVENT) {
        pthread_mutex_lock(&connected_lock);
        session = 1;

        if (state == ZOO_CONNECTED_STATE) {
            connected = 1;
        } else if (state == ZOO_NOTCONNECTED_STATE) {
            connected = 0;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            expired = 1;
            connected = 0;
        }

        pthread_mutex_unlock(&connected_lock);
        pthread_cond_broadcast(&connected_cond);
    }
}

zhandle_t *zoo_init(const char *host) {
    if (!host) {
        errno = EINVAL;
        return NULL;
    }

    zhandle_t *ret = NULL;
    pthread_mutex_lock(&connected_lock);
    session = 0;
    connected = 0;
    expired = 0;

    zoo_set_debug_level(0);
    zhandle_t *zh = zookeeper_init(host, watcher, 10000, 0, 0, 0);
    if (!zh) {
        errno = ECONNREFUSED;
        ret = NULL;
        goto cleanup;
    }

    struct timeval tv;
    struct timespec ts = {0};
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + 10;
    while (!session) {
        if (pthread_cond_timedwait(&connected_cond, &connected_lock, &ts) ==
            ETIMEDOUT) {
            errno = ETIMEDOUT;
            ret = NULL;
            break;
        }
    }

    if (!session || !connected) {
        zookeeper_close(zh);

        if (errno && errno != ETIMEDOUT) {
            errno = ECONNREFUSED;
        }

        ret = NULL;
        goto cleanup;
    }

    ret = zh;

cleanup:
    pthread_mutex_unlock(&connected_lock);
    return ret;
}

int zoo_deleteall(zhandle_t *zh, const char *path, int version) {
    if (!zh || !path) {
        errno = EINVAL;
        return -1;
    }

    struct String_vector children;
    int rc = zoo_get_children(zh, path, 0, &children);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    for (int i = 0; i < children.count; i++) {
        char child_path[512] = {0};
        snprintf(child_path, sizeof child_path, "%s/%s", path,
                 children.data[i]);
        if (zoo_deleteall(zh, child_path, version) < 0) {
            return -1;
        }
    }

    if (zoo_delete(zh, path, version)) {
        errno = EIO;
        return -1;
    }

    return 0;
}
