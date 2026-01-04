#include "partition.h"

#include <messageq/network.h>
#include <messageq/zookeeper.h>

#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"

enum role role = FREE;
int partition_id = -1;
char partition_path[MAX_PATH_LEN + 1] = {0};
char assigned_topic[MAX_TOPIC_LEN + 1] = {0};
char assigned_shard[MAX_SHARD_LEN + 1] = {0};

static zhandle_t *zh;
static struct queue queue;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

struct targ {
    int result;
    int _errno;
};

static void *server_thread(void *arg) {
    struct targ *targ = (struct targ *)arg;
    targ->result = dmqp_server_init(0);
    targ->_errno = errno;
    return NULL;
}

/**
 * Starts a DMQP server on a separate thread.
 *
 * @returns the id of the server thread, `NULL` if error with `global` errno
 * set. must be freed once copied
 * @throws `ETIMEDOUT` DMQP server conn timeout
 */
static pthread_t *start_dmqp_server() {
    pthread_t *tid = malloc(sizeof(pthread_t));
    struct targ targ = {0};
    pthread_create(tid, NULL, server_thread, &targ);

    struct timeval tv;
    struct timespec ts = {0};
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + 10;

    pthread_mutex_lock(&server_lock);
    while (!server_running && targ.result >= 0) {
        if (pthread_cond_timedwait(&server_running_cond, &server_lock, &ts) ==
            ETIMEDOUT) {
            errno = ETIMEDOUT;
            free(tid);
            tid = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&server_lock);

    return tid;
}

static void leader_watcher(zhandle_t *zzh, int type, int state,
                           const char *path, void *watcherCtx) {
    (void)zzh;
    (void)state;
    (void)path;
    (void)watcherCtx;

    if (type == ZOO_DELETED_EVENT) {
        role = LEADER;
    }
}

static void partition_znode_watcher(zhandle_t *zzh, int type, int state,
                                    const char *path, void *watcherCtx) {
    (void)state;

    // TODO: proper resource cleanup on error
    if (type == ZOO_CHANGED_EVENT) {
        // TODO: acquire lock on partition list
        if (partition_id == -1) {
            return;
        }

        char buf[512];
        int buflen = sizeof buf;
        zoo_wget(zzh, path, partition_znode_watcher, watcherCtx, buf, &buflen,
                 NULL);
        buf[buflen] = '\0';

        // get topic/shard
        strtok(buf, ";");
        char *allocated = strtok(NULL, ";");
        if (!allocated) {
            return;
        }

        strtok(allocated, "/");
        char *topic = strtok(NULL, "/");
        strncpy(assigned_topic, topic, MAX_TOPIC_LEN);
        strtok(NULL, "/");
        char *shard = strtok(NULL, "/");
        strncpy(assigned_shard, shard, MAX_SHARD_LEN);

        // get all partitiosn assigned to same shard
        char shardpath[MAX_PATH_LEN];
        snprintf(shardpath, sizeof shardpath, "/topics/%s/shards/%s/partitions",
                 topic, shard);
        struct String_vector partitions;
        zoo_get_children(zzh, shardpath, 0, &partitions);

        int preceding_id = -1;
        char *preceding_znode = NULL;
        for (int i = 0; i < partitions.count; i++) {
            int id = atoi(partitions.data[i] + 10);
            if (id < partition_id &&
                (preceding_id == -1 || id > preceding_id)) {
                preceding_id = id;
                preceding_znode = partitions.data[i];
            }
        }

        if (preceding_id == -1) {
            role = LEADER;
        } else {
            role = REPLICA;

            char preceding_partition[MAX_PATH_LEN];
            snprintf(preceding_partition, sizeof preceding_partition,
                     "/partitions/%s", preceding_znode);
            char buf[512];
            int buflen = sizeof buf;
            zoo_wget(zzh, preceding_partition, leader_watcher, NULL, buf,
                     &buflen, NULL);
        }

        // TODO: release /partitions distributed lock
    }
}

/**
 * Registers a partition into the service registry.
 */
static void reigster_partition() {
    // TODO: acquire /partitions distributed lock

    // TODO: must dynamically detect host in the future
    char host[MAX_HOST_LEN + 1];
    snprintf(host, sizeof host, "127.0.0.1:%d", server_port);

    zoo_create(zh, "/partitions/partition-", host, strlen(host),
               &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL, partition_path,
               sizeof partition_path);
    partition_id = atoi(partition_path + 22);

    char buf[512];
    int buflen = sizeof buf;
    zoo_wget(zh, partition_path, partition_znode_watcher, NULL, buf, &buflen,
             NULL);

    // TODO: release /partitions distributed lock
}

int start_partition(char *service_discovery_host) {
    if (!service_discovery_host) {
        errno = EINVAL;
        return -1;
    }

    int ret = 0;

    queue_init(&queue);
    if (!(zh = zoo_init(service_discovery_host))) {
        ret = -1;
        goto cleanup_queue;
    }

    pthread_t *server_tid = start_dmqp_server();
    if (!server_tid) {
        ret = -1;
        goto cleanup_zookeeper;
    }

    reigster_partition();
    pthread_join(*server_tid, NULL);
    free(server_tid);

cleanup_zookeeper:
    zookeeper_close(zh);
cleanup_queue:
    queue_destroy(&queue);
    return ret;
}

// TODO: test all of these DMQP handlers
void handle_dmqp_push(const struct dmqp_message *message, int client) {
    if (!message || client < 0 || role == FREE || partition_id < 0 ||
        !partition_path[0] || !assigned_topic[0] || !assigned_shard[0]) {
        return;
    }

    // TODO: acquire /topics/{topic_name}/sequence-id/lock distributed lock

    char path[MAX_PATH_LEN + 1];
    snprintf(path, sizeof path, "/topics/%s/sequence-id", assigned_topic);
    char buf[512];
    int buflen = sizeof buf;
    zoo_get(zh, path, 0, buf, &buflen, NULL);

    unsigned int seqid = atoi(buf);

    struct dmqp_header res_header;
    struct dmqp_message res_message;
    if (message->header.sequence_id != seqid) {
        res_header.sequence_id = 0;
        res_header.length = 0;
        res_header.method = DMQP_RESPONSE;
        res_header.status_code = EINVAL;

        res_message.header = res_header;
        res_message.payload = NULL;
        send_dmqp_message(client, &res_message, 0);
        goto cleanup;
    }

    pthread_mutex_lock(&queue_lock);
    struct queue_entry entry = {
        .id = message->header.sequence_id,
        .data = message->payload,
        .size = message->header.length,
    };
    queue_push(&queue, &entry);
    pthread_mutex_unlock(&queue_lock);

    res_header.sequence_id = 0;
    res_header.length = 0;
    res_header.method = DMQP_RESPONSE;
    res_header.status_code = 0;
    res_message.header = res_header;
    send_dmqp_message(client, &res_message, 0);

cleanup:;
    // TODO: release /topics/{topic_name}/sequence-id/lock distributed lock
}

void handle_dmqp_pop(const struct dmqp_message *message, int client) {
    if (!message || client < 0 || role == FREE || partition_id < 0 ||
        !partition_path[0] || !assigned_topic[0] || !assigned_shard[0]) {
        return;
    }

    pthread_mutex_lock(&queue_lock);
    struct queue_entry *entry = queue_pop(&queue);

    struct dmqp_header res_header;
    if (!entry) {
        res_header.sequence_id = 0;
        res_header.length = 0;
        res_header.method = DMQP_RESPONSE;
        res_header.status_code = ENODATA;

        struct dmqp_message res_message = {.header = res_header,
                                           .payload = NULL};
        send_dmqp_message(client, &res_message, 0);
        goto cleanup;
    }

    res_header.sequence_id = entry->id;
    res_header.length = entry->size;
    res_header.method = DMQP_RESPONSE;
    res_header.status_code = errno;
    struct dmqp_message res_message = {.header = res_header,
                                       .payload = entry->data};
    send_dmqp_message(client, &res_message, 0);

cleanup:
    if (entry) {
        if (entry->data) {
            free(entry->data);
        }

        free(entry);
    }
    pthread_mutex_unlock(&queue_lock);
}

void handle_dmqp_peek_sequence_id(const struct dmqp_message *message,
                                  int client) {
    if (!message || client < 0 || role == FREE || partition_id < 0 ||
        !partition_path[0] || !assigned_topic[0] || !assigned_shard[0]) {
        return;
    }

    pthread_mutex_lock(&queue_lock);
    int seqid = queue_peek_id(&queue);
    pthread_mutex_unlock(&queue_lock);

    struct dmqp_header res_header = {0};
    if (seqid < 0) {
        res_header.method = DMQP_RESPONSE;
        res_header.status_code = ENODATA;
    } else {
        res_header.sequence_id = seqid;
        res_header.method = DMQP_RESPONSE;
    }

    struct dmqp_message res_message = {.header = res_header, .payload = NULL};
    send_dmqp_message(client, &res_message, 0);
}

void handle_dmqp_response(const struct dmqp_message *message, int client) {
    if (!message || client < 0 || role == FREE || partition_id < 0 ||
        !partition_path[0] || !assigned_topic[0] || !assigned_shard[0]) {
        return;
    }

    struct dmqp_header res_header = {0};
    res_header.method = DMQP_RESPONSE;
    res_header.status_code = EPROTO;
    struct dmqp_message res_message = {.header = res_header, .payload = NULL};
    send_dmqp_message(client, &res_message, 0);
}
