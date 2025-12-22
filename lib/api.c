#include "api.h"

#include <errno.h>
#include <stdlib.h>

int create_topic(const struct server *server, const struct topic *topic) {
    if (server == NULL || server->host == NULL || topic == NULL ||
        topic->name == NULL) {
        errno = EINVAL;
        return -1;
    }

    // if (topic_exists()) {
    //      return -1;
    // }

    // if (num_of_available_partitions() < topic->shards *
    // topic->replication_factor) { return -1 }
    return 0;
}
