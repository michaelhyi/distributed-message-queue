#include <messageq/network.h>

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zookeeper/zookeeper.h>

#include "queue.h"

#define MAX_HOST_LEN 21 // ipv4:port fomat xxx.xxx.xxx.xxx:xxxxx

struct queue queue;
pthread_mutex_t queue_lock;

static void watcher(zhandle_t *zzh, int type, int state, const char *path,
                    void *watcherCtx) {
    (void)zzh;
    (void)type;
    (void)state;
    (void)path;
    (void)watcherCtx;
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
    zhandle_t *zh =
        zookeeper_init(service_discovery_host, watcher, 10000, 0, 0, 0);
    if (!zh) {
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

    // TODO: obtain lock
    // TODO: use localhost for now, must dynamically detect in the future
    // 16 max buf len
    char host[16];
    snprintf(host, sizeof host, "127.0.0.1:%d", server_port);
    if (zoo_create(zh, "/partitions/partition-", host, strlen(host),
                   &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL_SEQUENTIAL, NULL, 0)) {
        // TODO: release lock
        ret = 1;
        // TODO: on error, cleanup dmqp server
        goto cleanup_zookeeper;
    }
    // TODO: release lock

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
