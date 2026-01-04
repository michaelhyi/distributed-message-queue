#include "messageq/locking.h"
#include "messageq/constants.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <uuid/uuid.h>

static __thread uuid_t lock_holder_id;
static __thread int lock_holder_id_initialized = 0;
static __thread int unlocked = 0;
static __thread pthread_mutex_t unlocked_mutex;
static __thread pthread_cond_t unlocked_cond;
static __thread int sync_primitives_initialized = 0;

static void preceding_lock_node_watcher(zhandle_t *zzh, int type, int state,
                                        const char *path, void *watcherCtx) {
    (void)zzh;
    (void)state;
    (void)path;
    (void)watcherCtx;

    if (type == ZOO_DELETED_EVENT) {
        pthread_mutex_lock(&unlocked_mutex);
        unlocked = 1;
        pthread_mutex_unlock(&unlocked_mutex);
        pthread_cond_signal(&unlocked_cond);
    }
}

int acquire_distributed_lock(const char *lock, zhandle_t *zh) {
    if (!lock || !zh) {
        errno = EINVAL;
        return -1;
    }

    if (!lock_holder_id_initialized) {
        uuid_generate_time_safe(lock_holder_id);
        lock_holder_id_initialized = 1;
    }

    if (!sync_primitives_initialized) {
        pthread_mutex_init(&unlocked_mutex, NULL);
        pthread_cond_init(&unlocked_cond, NULL);
        sync_primitives_initialized = 1;
    }

    char path[MAX_PATH_LEN + 1];
    int path_len = sizeof path;
    snprintf(path, path_len, "%s/lock-", lock);
    int rc = zoo_create(zh, path, (char *)lock_holder_id, sizeof lock_holder_id,
                        &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL, path,
                        path_len);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    int lock_id = atoi(strchr(path, "-") + 1);
    for (;;) {
        int preceding_lock_id = -1;

        struct String_vector lock_nodes;
        if (zoo_get_children(zh, lock, 0, &lock_nodes)) {
            errno = EIO;
            goto cleanup;
        }

        for (int i = 0; i < lock_nodes.count; i++) {
            int curr_lock_id = atoi(strchr(lock_nodes.data[i], "-") + 1);

            if (curr_lock_id < lock_id &&
                (preceding_lock_id == -1 || curr_lock_id > preceding_lock_id)) {
                preceding_lock_id = curr_lock_id;
            }
        }

        if (preceding_lock_id == -1) {
            deallocate_String_vector(&lock_nodes);
            break;
        }

        char preceding_node[MAX_PATH_LEN + 1];
        snprintf(preceding_node, sizeof preceding_node, "%s/lock-%010d", lock,
                 preceding_lock_id);
        rc = zoo_wexists(zh, preceding_node, preceding_lock_node_watcher, NULL,
                         NULL);
        if (rc == ZNONODE) {
            deallocate_String_vector(&lock_nodes);
            continue;
        } else if (rc) {
            deallocate_String_vector(&lock_nodes);
            errno = EIO;
            goto cleanup;
        }

        pthread_mutex_lock(&unlocked_mutex);
        while (!unlocked) {
            pthread_cond_wait(&unlocked_cond, &unlocked_mutex);
        }
        unlocked = 0;
        pthread_mutex_unlock(&unlocked_mutex);

        deallocate_String_vector(&lock_nodes);
    }

    return 0;

cleanup:
    zoo_delete(zh, path, -1);
    return -1;
}

int release_distributed_lock(const char *lock, zhandle_t *zh) {
    if (!lock || !zh) {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;
    if (!lock_holder_id_initialized) {
        errno = EPERM;
        return -1;
    }

    // TODO: check every usage of strtok(), since it modifies original data
    // TODO: deallocate every instance of String_vector
    struct String_vector lock_nodes;
    int rc = zoo_get_children(zh, lock, 0, &lock_nodes);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    if (lock_nodes.count == 0) {
        errno = EPERM;
        ret = -1;
        goto cleanup;
    }

    int min_lock_id = -1;
    for (int i = 0; i < lock_nodes.count; i++) {
        int curr_lock_id = atoi(strchr(lock_nodes.data[i], "-") + 1);

        if (curr_lock_id < min_lock_id || min_lock_id == -1) {
            min_lock_id = curr_lock_id;
        }
    }

    char lock_holder[MAX_PATH_LEN + 1];
    snprintf(lock_holder, sizeof lock_holder, "%s/lock-%010d", lock,
             min_lock_id);
    char buf[512];
    int buf_len = sizeof buf;
    if (zoo_get(zh, lock_holder, 0, buf, &buf_len, NULL)) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

    if (memcmp(lock_holder_id, buf, sizeof lock_holder_id)) {
        errno = EPERM;
        ret = -1;
        goto cleanup;
    }

    if (zoo_delete(zh, lock_holder, -1)) {
        errno = EIO;
        ret = -1;
        goto cleanup;
    }

cleanup:
    deallocate_String_vector(&lock_nodes);
    return ret;
}
