#include "api.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

#include "zookeeper_util.h"

#define MAX_SERVER_ADDRESS_LEN 21 // ipv4:port fomat xxx.xxx.xxx.xxx:xxxxx
#define ZOOKEEPER_SEQ_ID_LEN 10
#define MAX_PATH_LEN 128

static zhandle_t *zh = NULL;

// TODO: impl
void watcher(zhandle_t *zzh, int type, int state, const char *path,
             void *watcherCtx) {
    (void)zzh;
    (void)type;
    (void)state;
    (void)path;
    (void)watcherCtx;
}

int client_init(const char *host) {
    if (host == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (zh != NULL) {
        errno = EALREADY;
        return -1;
    }

    zh = zookeeper_init(host, watcher, 10000, 0, 0, 0);
    if (!zh) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int client_destroy(void) {
    if (zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    int res = zookeeper_close(zh);
    if (res < 0) {
        zh = NULL;
        errno = EIO;
        return -1;
    }

    zh = NULL;
    return 0;
}

/**
 * Returns the number of free partitions by getting the data of the
 * `/partitions/free-count` ZNode.
 *
 * @returns the number of free partitions, -1 if error with global `errno` set
 * @throws `EIO` unexpected error
 */
static int get_num_free_partitions() {
    char buffer[16] = {0};
    int buffer_len = sizeof(buffer) - 1;
    int rc =
        zoo_get(zh, "/partitions/free-count", 0, buffer, &buffer_len, NULL);
    if (rc) {
        errno = EIO;
        return -1;
    }

    buffer[buffer_len] = '\0';
    char *endptr;
    long n = strtol(buffer, &endptr, 10);
    if (errno || endptr == buffer || *endptr != '\0') {
        errno = EIO;
        return -1;
    }

    return (int)n;
}

/**
 * Initializes the topic's metadata by creating the following ZNodes:
 * /topics/{topic_name}                  (PERSISTENT)
 * /topics/{topic_name}/sequence-id      (PERSISTENT)
 * /topics/{topic_name}/sequence-id/lock (PERSISTENT)
 * /topics/{topic_name}/shards           (PERSISTENT)
 *
 * Shard metadata is not initiialized.
 *
 * @param topic_name topic name
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args or `topic_name` exceeds max length
 * @throws `EEXIST` topic already exists
 * @throws `EIO` unexpected error
 */
static int init_topic_metadata(const char *topic_name) {
    if (topic_name == NULL || strnlen(topic_name, MAX_SERVER_ADDRESS_LEN + 1) >
                                  MAX_SERVER_ADDRESS_LEN) {
        errno = EINVAL;
        return -1;
    }

    char path[MAX_PATH_LEN];

    // /topics/{topic_name}
    int n = snprintf(path, sizeof(path), "/topics/%s", topic_name);
    if (n < 0) {
        errno = EIO;
        return -1;
    }
    int rc = zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT, NULL, 0);
    if (rc == ZNODEEXISTS) {
        errno = EEXIST;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/sequence-id
    n = snprintf(path, sizeof(path), "/topics/%s/sequence-id", topic_name);
    if (n < 0) {
        errno = EIO;
        return -1;
    }
    rc = zoo_create(zh, path, "0", 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT,
                    NULL, 0);
    if (rc) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/sequence-id/lock
    n = snprintf(path, sizeof(path), "/topics/%s/sequence-id/lock", topic_name);
    if (n < 0) {
        errno = EIO;
        return -1;
    }
    rc = zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT,
                    NULL, 0);
    if (rc) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/shards
    n = snprintf(path, sizeof(path), "/topics/%s/shards", topic_name);
    if (n < 0) {
        errno = EIO;
        return -1;
    }
    rc = zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT,
                    NULL, 0);
    if (rc) {
        errno = EIO;
        return -1;
    }

    return 0;
}

// TODO: this is slow, since we have to fetch all partitions each time this is
// called, and then iterate the list every time.
/**
 * Finds a free partition and returns its partition ID. The partition ID is
 * created by ZooKeeper sequentially, and it is simply the name of the child
 * ZNode under `/partitions`. Must be holding the distributed lock:
 * `/partitions/lock`.
 *
 * @param partition_id output param for returning the free partition's ID
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `ENODEV` no free partitions
 * @throws `EIO` unexpected error
 */
static int get_free_partition(char *partition_id) {
    if (partition_id == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct String_vector partitions;
    int rc = zoo_get_children(zh, "/partitions", 0, &partitions);
    if (rc) {
        errno = EIO;
        return -1;
    }

    for (int i = 0; i < partitions.count; i++) {
        char *partition = partitions.data[i];
        if (strcmp(partition, "free-count") == 0) {
            continue;
        }

        char path[MAX_PATH_LEN] = {0};
        int n = snprintf(path, sizeof(path), "/partitions/%s", partition);
        if (n < 0) {
            continue;
        }

        char buffer[512] = {0};
        int buffer_len = sizeof(buffer);
        rc = zoo_get(zh, path, 0, buffer, &buffer_len, NULL);
        if (rc) {
            continue;
        }
        buffer[buffer_len] = '\0';

        strtok(buffer, ";");
        char *allocated = strtok(NULL, ";");
        if (!allocated) {
            strncpy(partition_id, path, MAX_PATH_LEN);
            return 0;
        }
    }

    errno = ENODEV;
    return -1;
}

/**
 * Allocates a partition to a shard and establishes leader election within a
 * shard's replica set. Assigning partitions to the shard creates the following
 * ZNode:
 * /topics/{topic_name}/shards/shard-{sequence_id}/partitions/partition-{sequence_id}
 * (PERSISTENT & SEQUENTIAL)
 *
 * @param shard_path ZNode path of the shard
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `EIO` unexpected error
 */
static int create_replica(const char *shard_path) {
    if (shard_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    char partition_id[MAX_PATH_LEN] = {0};
    int res = get_free_partition(partition_id);
    if (res < 0) {
        if (errno != ENODEV) {
            errno = EIO;
        }

        return -1;
    }

    // /topics/{topic_name}/shards/shard-{sequence_id}/partitions/partition-{sequence_id}
    char path[MAX_PATH_LEN] = {0};
    strcat(path, shard_path);
    strcat(path, partition_id);
    int rc = zoo_create(zh, path, partition_id, strlen(partition_id),
                        &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL, 0);
    if (rc) {
        errno = EIO;
        return -1;
    }

    // get data of allocated partition and append its assigned shard
    char buffer[512] = {0};
    int buffer_len = sizeof(buffer) - 1;
    rc = zoo_get(zh, partition_id, 0, buffer, &buffer_len, NULL);
    if (rc) {
        errno = EIO;
        return -1;
    }
    buffer[buffer_len] = '\0';

    strcat(buffer, ";");
    strcat(buffer, shard_path);
    rc = zoo_set(zh, partition_id, buffer, strlen(buffer), -1);
    if (rc) {
        errno = EIO;
        return -1;
    }

    int free_cnt = get_num_free_partitions();
    if (free_cnt < 0) {
        errno = EIO;
        return -1;
    }

    char buf[16] = {0};
    int n = snprintf(buf, sizeof buf, "%d", free_cnt - 1);
    if (n < 0) {
        errno = EIO;
        return -1;
    }

    rc = zoo_set(zh, "/partitions/free-count", buf, strlen(buf), -1);
    if (rc) {
        errno = EIO;
        return -1;
    }

    // TODO:
    // if this shard has no partitions, p is the leader (lowest seq no.)
    // if this shard has partitions, then p must set a watch on the node
    // with highest seq no. the watch should promote itself to leader if
    // the previous node dies
    return 0;
}

/**
 * Creates shard & replica metadata for a topic by creating the following
 * ZNodes: /topics/{topic_name}/shards/shard-{sequence_id} (PERSISTENT &
 * SEQUENTIAL) /topics/{topic_name}/shards/shard-{sequence_id}/partitions
 * (PERSISTENT)
 * /topics/{topic_name}/shards/shard-{sequence_id}/partitions/partition-{sequence_id}
 * (PERSISTENT & SEQUENTIAL)
 *
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args or `topic_name` exceeds max length
 * @throws `EIO` unexpected error
 */
static int create_shard(const char *topic_name,
                        unsigned int replication_factor) {
    if (topic_name == NULL ||
        strnlen(topic_name, MAX_TOPIC_NAME_LEN + 1) > MAX_TOPIC_NAME_LEN ||
        replication_factor == 0) {
        errno = EINVAL;
        return -1;
    }

    char path[MAX_PATH_LEN];

    // /topics/{topic_name}/shards/shard-{sequence_id}
    int n =
        snprintf(path, sizeof(path), "/topics/%s/shards/shard-", topic_name);
    if (n < 0) {
        errno = EIO;
        return -1;
    }
    char path_buffer[MAX_PATH_LEN];
    int rc = zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
                        ZOO_PERSISTENT_SEQUENTIAL, path_buffer,
                        sizeof(path_buffer) - 1);
    if (rc) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/shards/shard-{sequence_id}/partitions
    char partitions_path_buffer[MAX_PATH_LEN] = {0};
    strcat(partitions_path_buffer, path_buffer);
    strcat(partitions_path_buffer, "/partitions");
    rc = zoo_create(zh, partitions_path_buffer, NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
                    ZOO_PERSISTENT, NULL, 0);
    if (rc) {
        errno = EIO;
        return -1;
    }

    for (unsigned int replica = 0; replica < replication_factor; replica++) {
        int res = create_replica(path_buffer);
        if (res < 0) {
            errno = EIO;
            return -1;
        }
    }

    return 0;
}

int create_topic(const struct topic *topic) {
    if (topic == NULL || topic->name == NULL ||
        strnlen(topic->name, MAX_TOPIC_NAME_LEN + 1) > MAX_TOPIC_NAME_LEN ||
        topic->shards == 0 || topic->replication_factor == 0) {
        errno = EINVAL;
        return -1;
    }

    // TODO: acquire distributed lock on partition list

    int num_free_partitions = get_num_free_partitions();
    if (num_free_partitions < 0) {
        errno = EIO;
        // TODO: relse distributed lock
        return -1;
    }

    unsigned int num_requested_partitions =
        topic->shards * topic->replication_factor;
    if ((unsigned int)num_free_partitions < num_requested_partitions) {
        errno = ENODEV;
        // TODO: relse distributed lock
        return -1;
    }

    int res = init_topic_metadata(topic->name);
    if (res < 0) {
        if (errno != EEXIST) {
            errno = EIO;
        }

        // TODO: do not jump to cleanup
        goto cleanup;
    }

    for (unsigned int shard = 0; shard < topic->shards; shard++) {
        res = create_shard(topic->name, topic->replication_factor);
        if (res < 0) {
            errno = EIO;
            goto cleanup;
        }
    }

    // TODO: release distributed lock
    return 0;

// TODO: need to fix this
cleanup:;
    // TODO: release distributed lock

    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/topics/%s", topic->name);
    zoo_deleteall(zh, path, -1);
    return -1;
}
