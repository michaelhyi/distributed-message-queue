#include "messageq/locking.h"
#include "messageq/constants.h"

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <uuid/uuid.h>

static __thread uuid_t lock_holder_id;
static __thread int lock_holder_id_initialized = 0;

struct lock_context {
    int unlocked;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

static void preceding_lock_node_watcher(zhandle_t *zzh, int type, int state,
                                        const char *path, void *watcherCtx) {
    (void)zzh;
    (void)state;
    (void)path;

    struct lock_context *context = (struct lock_context *)watcherCtx;

    if (type == ZOO_DELETED_EVENT) {
        pthread_mutex_lock(&context->mutex);
        context->unlocked = 1;
        pthread_mutex_unlock(&context->mutex);
        pthread_cond_signal(&context->cond);
    }
}

int acquire_distributed_lock(const char *lock, zhandle_t *zh) {
    if (!lock || !lock[0] || !zh) {
        errno = EINVAL;
        return -1;
    }

    if (!lock_holder_id_initialized) {
        uuid_generate_time_safe(lock_holder_id);
        lock_holder_id_initialized = 1;
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

    int lock_id = atoi(strrchr(path, '-') + 1);
    for (;;) {
        int preceding_lock_id = -1;

        struct String_vector lock_nodes;
        if (zoo_get_children(zh, lock, 0, &lock_nodes)) {
            errno = EIO;
            goto cleanup;
        }

        for (int i = 0; i < lock_nodes.count; i++) {
            int curr_lock_id = atoi(strrchr(lock_nodes.data[i], '-') + 1);

            if (curr_lock_id < lock_id &&
                (preceding_lock_id == -1 || curr_lock_id > preceding_lock_id)) {
                preceding_lock_id = curr_lock_id;
            }
        }

        if (preceding_lock_id == -1) {
            deallocate_String_vector(&lock_nodes);
            break;
        }

        struct lock_context context = {0};
        pthread_mutex_init(&context.mutex, NULL);
        pthread_cond_init(&context.cond, NULL);

        char preceding_node[MAX_PATH_LEN + 1];
        snprintf(preceding_node, sizeof preceding_node, "%s/lock-%010d", lock,
                 preceding_lock_id);
        rc = zoo_wexists(zh, preceding_node, preceding_lock_node_watcher,
                         &context, NULL);
        if (rc == ZNONODE) {
            deallocate_String_vector(&lock_nodes);
            continue;
        } else if (rc) {
            deallocate_String_vector(&lock_nodes);
            errno = EIO;
            goto cleanup;
        }

        pthread_mutex_lock(&context.mutex);
        while (!context.unlocked) {
            pthread_cond_wait(&context.cond, &context.mutex);
        }
        context.unlocked = 0;
        pthread_mutex_unlock(&context.mutex);

        deallocate_String_vector(&lock_nodes);
    }

    return 0;

cleanup:
    zoo_delete(zh, path, -1);
    return -1;
}

int release_distributed_lock(const char *lock, zhandle_t *zh) {
    if (!lock || !lock[0] || !zh) {
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
        int curr_lock_id = atoi(strrchr(lock_nodes.data[i], '-') + 1);

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
