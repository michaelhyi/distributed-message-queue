#ifndef PARTITION_H
#define PARTITION_H

#include <messageq/constants.h>

enum role { LEADER, REPLICA, FREE };

extern enum role role;
extern int partition_id;
extern char partition_path[MAX_PATH_LEN + 1];
extern char assigned_topic[MAX_TOPIC_LEN + 1];
extern char assigned_shard[MAX_SHARD_LEN + 1];

/**
 * Starts a partition.
 *
 * @param service_discovery_host host of metadata (zookeeper) server
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `ECONNREFUSED` conn failure to metadata server
 * @throws `ETIMEDOUT` conn timeout to metadata server
 */
int start_partition(char *service_discovery_host);

#endif
