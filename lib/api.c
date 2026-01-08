#include "messageq/api.h"
#include "messageq/constants.h"
#include "messageq/locking.h"
#include "messageq/zookeeper.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

static zhandle_t *zh = NULL;

int client_init(const char *host) {
    if (!host) {
        errno = EINVAL;
        return -1;
    }

    if (zh) {
        errno = EALREADY;
        return -1;
    }

    if (!(zh = zoo_init(host))) {
        errno = EIO;
        return -1;
    }

    return 0;
}

void client_destroy(void) {
    if (!zh) {
        return;
    }

    zookeeper_close(zh);
    zh = NULL;
}

/**
 * Returns the number of free partitions by getting the data of the
 * `/partitions/free-count` ZNode. Must be holding /partitions distributed lock.
 *
 * @returns the number of free partitions, -1 if error with global `errno` set
 * @throws `EIO` unexpected error
 */
static int get_num_free_partitions() {
    char buffer[16] = {0};
    int buffer_len = sizeof buffer - 1;
    if (zoo_get(zh, "/partitions/free-count", 0, buffer, &buffer_len, NULL)) {
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
    char path[MAX_PATH_LEN];

    // /topics/{topic_name}
    snprintf(path, sizeof path, "/topics/%s", topic_name);
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
    snprintf(path, sizeof path, "/topics/%s/sequence-id", topic_name);
    if (zoo_create(zh, path, "0", 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL,
                   0)) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/sequence-id/lock
    snprintf(path, sizeof path, "/topics/%s/sequence-id/lock", topic_name);
    if (zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT,
                   NULL, 0)) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/shards
    snprintf(path, sizeof path, "/topics/%s/shards", topic_name);
    if (zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT,
                   NULL, 0)) {
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
    struct String_vector partitions;
    if (zoo_get_children(zh, "/partitions", 0, &partitions)) {
        errno = EIO;
        return -1;
    }

    for (int i = 0; i < partitions.count; i++) {
        char *partition = partitions.data[i];
        if (strcmp(partition, "free-count") == 0 ||
            strcmp(partition, "lock") == 0) {
            continue;
        }

        char path[MAX_PATH_LEN] = {0};
        snprintf(path, sizeof(path), "/partitions/%s", partition);

        char buffer[512] = {0};
        int buffer_len = sizeof(buffer);
        if (zoo_get(zh, path, 0, buffer, &buffer_len, NULL)) {
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
    char partition_id[MAX_PATH_LEN] = {0};
    get_free_partition(partition_id);

    // /topics/{topic_name}/shards/shard-{sequence_id}/partitions/partition-{sequence_id}
    char path[MAX_PATH_LEN] = {0};
    strcat(path, shard_path);
    strcat(path, partition_id);
    if (zoo_create(zh, path, partition_id, strlen(partition_id),
                   &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL, 0)) {
        errno = EIO;
        return -1;
    }

    // get data of allocated partition and append its assigned shard
    char buffer[512] = {0};
    int buffer_len = sizeof(buffer) - 1;
    if (zoo_get(zh, partition_id, 0, buffer, &buffer_len, NULL)) {
        errno = EIO;
        return -1;
    }
    buffer[buffer_len] = '\0';

    strcat(buffer, ";");
    strcat(buffer, shard_path);
    if (zoo_set(zh, partition_id, buffer, strlen(buffer), -1)) {
        errno = EIO;
        return -1;
    }

    int free_cnt = get_num_free_partitions();
    char buf[16] = {0};
    snprintf(buf, sizeof buf, "%d", free_cnt - 1);
    if (zoo_set(zh, "/partitions/free-count", buf, strlen(buf), -1)) {
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
 * Must be holding the /partitions distributed lock.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args or `topic_name` exceeds max length
 * @throws `EIO` unexpected error
 */
static int create_shard(const char *topic_name,
                        unsigned int replication_factor) {
    char path[MAX_PATH_LEN];

    // /topics/{topic_name}/shards/shard-{sequence_id}
    snprintf(path, sizeof(path), "/topics/%s/shards/shard-", topic_name);
    char path_buffer[MAX_PATH_LEN];
    if (zoo_create(zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_PERSISTENT_SEQUENTIAL, path_buffer,
                   sizeof(path_buffer) - 1)) {
        errno = EIO;
        return -1;
    }

    // /topics/{topic_name}/shards/shard-{sequence_id}/partitions
    char partitions_path_buffer[MAX_PATH_LEN] = {0};
    strcat(partitions_path_buffer, path_buffer);
    strcat(partitions_path_buffer, "/partitions");
    if (zoo_create(zh, partitions_path_buffer, NULL, -1, &ZOO_OPEN_ACL_UNSAFE,
                   ZOO_PERSISTENT, NULL, 0)) {
        errno = EIO;
        return -1;
    }

    for (unsigned int replica = 0; replica < replication_factor; replica++) {
        if (create_replica(path_buffer) < 0) {
            errno = EIO;
            return -1;
        }
    }

    return 0;
}

int create_topic(const struct topic *topic) {
    if (!zh || !topic || !topic->name ||
        strnlen(topic->name, MAX_TOPIC_LEN + 1) > MAX_TOPIC_LEN ||
        !topic->shards || !topic->replication_factor) {
        errno = EINVAL;
        return -1;
    }

    acquire_distributed_lock("/partitions/lock", zh);

    int num_free_partitions = get_num_free_partitions();
    if (num_free_partitions < 0) {
        release_distributed_lock("/partitions/lock", zh);
        errno = EIO;
        return -1;
    }

    unsigned int num_requested_partitions =
        topic->shards * topic->replication_factor;
    if ((unsigned int)num_free_partitions < num_requested_partitions) {
        release_distributed_lock("/partitions/lock", zh);
        errno = ENODEV;
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

    release_distributed_lock("/partitions/lock", zh);
    return 0;

// TODO: need to fix this
cleanup:
    release_distributed_lock("/partitions/lock", zh);

    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "/topics/%s", topic->name);
    zoo_deleteall(zh, path, -1);
    return -1;
}
