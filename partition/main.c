#include <messageq/api.h> // TODO: shouldn't have to import the api to get macros
#include <messageq/network.h>
#include <messageq/zookeeper.h>

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>

#include "queue.h"

#define MAX_SHARD_LEN 16 // shard znode name format: shard-xxxxxxxxxx
#define MAX_HOST_LEN 21  // ipv4:port format xxx.xxx.xxx.xxx:xxxxx
#define MAX_PATH_LEN 128

static zhandle_t *zh;
enum { LEADER, REPLICA, FREE } role = FREE;
int partition_id = -1;
char assigned_topic[MAX_TOPIC_NAME_LEN + 1] = {0};
char assigned_shard[MAX_SHARD_LEN + 1] = {0};

struct queue queue;
pthread_mutex_t queue_lock;

static void partition_znode_watcher(zhandle_t *zzh, int type, int state,
                                    const char *path, void *watcherCtx) {
    (void)state;

    // TODO: proper resource cleanup on error
    // TODO: validate partition_id properly set
    if (type == ZOO_CHANGED_EVENT) {
        // TODO: acquire lock on partition list

        char buf[512];
        int buflen = sizeof buf;
        if (zoo_wget(zzh, path, partition_znode_watcher, watcherCtx, buf,
                     &buflen, NULL)) {
            return;
        }
        // TODO: make sure you're nul terminating every zoo buf
        buf[buflen] = '\0';

        strtok(buf, ";");
        char *allocated = strtok(NULL, ";");
        if (!allocated) {
            return;
        }

        strtok(allocated, "/");
        char *topic = strtok(NULL, "/");
        strncpy(assigned_topic, topic, MAX_TOPIC_NAME_LEN);

        strtok(NULL, "/");
        char *shard = strtok(NULL, "/");
        strncpy(assigned_shard, shard, MAX_SHARD_LEN);

        char shardpath[MAX_PATH_LEN];
        snprintf(shardpath, sizeof shardpath, "/topics/%s/shards/%s/partitions",
                 topic, shard);
        struct String_vector partitions;
        if (zoo_get_children(zzh, shardpath, 0, &partitions)) {
            return;
        }

        int min_id = partition_id;
        for (int i = 0; i < partitions.count; i++) {
            int id = atoi(partitions.data[i] + 10);
            if (id < min_id) {
                min_id = id;
                break;
            }
        }

        if (min_id == partition_id) {
            role = LEADER;
        } else {
            role = REPLICA;
        }

        // TODO: must set a watch on the node with the next smallest id

        // TODO:
        // unlock distributed lock on partition list, esp every time on return
    }
}

struct targ {
    int result;
    int _errno;
};

static void *start_dmqp_server(void *arg) {
    struct targ *targ = (struct targ *)arg;
    targ->result = dmqp_server_init(0);
    targ->_errno = errno;
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [-s] [host:port]\n", argv[0]);
        return 1;
    }

    int opt;
    char service_discovery_host[MAX_HOST_LEN + 1];

    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
        case 's':
            strcpy(service_discovery_host, optarg);
            service_discovery_host[MAX_HOST_LEN] = '\0';
            break;
        default:
            fprintf(stderr, "Usage: %s [-s] [host:port]\n", argv[0]);
            return 1;
        }
    }

    int ret = 0;

    queue_init(&queue);
    pthread_mutex_init(&queue_lock, NULL);

    zoo_set_debug_level(0);
    if (!(zh = zoo_init(service_discovery_host))) {
        ret = 1;
        goto cleanup;
    }

    pthread_t tid;
    struct targ targ = {0};
    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&server_running_cond, NULL);
    pthread_create(&tid, NULL, start_dmqp_server, &targ);

    pthread_mutex_lock(&server_lock);
    while (!server_running && targ.result >= 0) {
        // TODO: should timeout
        pthread_cond_wait(&server_running_cond, &server_lock);
    }
    pthread_mutex_unlock(&server_lock);

    // TODO: acquire /partitions distributed lock

    // TODO: must dynamically detect host in the future
    char host[MAX_HOST_LEN + 1];
    snprintf(host, sizeof host, "127.0.0.1:%d", server_port);

    char path[MAX_PATH_LEN + 1];
    int pathlen = sizeof path;
    if (zoo_create(zh, "/partitions/partition-", host, strlen(host),
                   &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL, path,
                   pathlen)) {
        // TODO: release /partitions distributed lock
        ret = 1;
        // TODO: on error, cleanup dmqp server
        goto cleanup_zookeeper;
    }
    partition_id = atoi(path + 22);

    if (zoo_wexists(zh, path, partition_znode_watcher, NULL, NULL)) {
        ret = 1;
        goto cleanup_zookeeper;
    }

    // TODO: release /partitions distributed lock

    pthread_join(tid, NULL);

cleanup_zookeeper:
    zookeeper_close(zh);
    pthread_cond_destroy(&server_running_cond);
    pthread_mutex_destroy(&server_lock);
cleanup:
    pthread_mutex_destroy(&queue_lock);
    queue_destroy(&queue);
    return ret;
}

// // // TODO: test and fix all these
// void handle_dmqp_push(const struct dmqp_message *message, int client) {
//     struct dmqp_header res_header;

//     // TODO: establish what topic this partition is part of
//     // TODO: acquire seq id distributed lock
//     // get seq id
//     // if message->header.sequence_id doesnt match the topic's expected seq
//     id,
//     //

//     pthread_mutex_lock(&queue_lock);
//     struct queue_entry entry = {.data = message->payload,
//                                 .size = message->header.length,
//                                 .timestamp = message->header.sequence_id};

//     if (queue_push(&queue, &entry) < 0) {
//         if (errno != ENODATA) {
//             errno = EIO;
//         }
//     }

//     pthread_mutex_unlock(&queue_lock);

//     res_header.method = DMQP_RESPONSE;
//     res_header.status_code = errno;
//     res_header.length = 0;
//     res_header.sequence_id = 0;
//     struct dmqp_message res_message = {.header = res_header};

//     send_dmqp_message(client, &res_message, 0);
// }

// void handle_dmqp_pop(const struct dmqp_message *message, int client) {
//     (void)message;
//     struct queue_entry entry = {.data = NULL, .size = 0, .timestamp = 0};
//     struct dmqp_header res_header;

//     pthread_mutex_lock(&queue_lock);

//     if (queue_pop(&queue, &entry) < 0) {
//         if (errno != ENODATA) {
//             errno = EIO;
//         }
//     }

//     pthread_mutex_unlock(&queue_lock);

//     res_header.method = DMQP_RESPONSE;
//     res_header.status_code = errno;
//     res_header.length = entry.size;
//     res_header.sequence_id = entry.timestamp;
//     struct dmqp_message res_message = {.header = res_header,
//                                        .payload = entry.data};

//     send_dmqp_message(client, &res_message, 0);

//     if (entry.data != NULL) {
//         free(entry.data);
//     }
// }

// void handle_dmqp_peek_sequence_id(const struct dmqp_message *message,
//                                   int client) {
//     (void)message;
//     struct dmqp_header res_header;

//     pthread_mutex_lock(&queue_lock);

//     long timestamp;
//     if (queue_peek_timestamp(&queue, &timestamp) < 0) {
//         if (errno != ENODATA) {
//             errno = EIO;
//         }
//     }

//     pthread_mutex_unlock(&queue_lock);

//     res_header.method = DMQP_RESPONSE;
//     res_header.status_code = errno;
//     res_header.length = 0;
//     res_header.sequence_id = timestamp;
//     struct dmqp_message res_message = {.header = res_header, .payload =
//     NULL};

//     send_dmqp_message(client, &res_message, 0);
// }

// void handle_dmqp_response(const struct dmqp_message *message, int client) {
//     (void)message;
//     struct dmqp_header res_header = {.method = DMQP_RESPONSE,
//                                      .status_code = EPROTO,
//                                      .length = 0,
//                                      .sequence_id = 0};
//     struct dmqp_message res_message = {.header = res_header};

//     send_dmqp_message(client, &res_message, 0);
//     errno = EPROTO;
// }
