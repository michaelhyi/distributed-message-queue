#include "metadata.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

#define MAX_PORT_DIGITS 5

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

int metadata_init(const struct server *server) {
    if (server == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (zh != NULL) {
        errno = EALREADY;
        return -1;
    }

    char server_port[MAX_PORT_DIGITS + 2];
    memset(server_port, 0, sizeof(server_port));
    server_port[0] = ':';
    sprintf(server_port + 1, "%d", server->port);
    char *server_address = strcat(server->host, server_port);

    zh = zookeeper_init(server_address, watcher, 10000, 0, 0, 0);

    if (!zh) {
        return -1;
    }

    return 0;
}

int metadata_destroy(void) {
    if (zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    zookeeper_close(zh);
    return 0;
}

int metadata_get(const char *key, void *value) {
    if (key == NULL || value == NULL || zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct Stat stat;
    int rc = zoo_get(zh, key, 0, value, (int *)&MAX_METADATA_ENTRY_SIZE, &stat);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int metadata_set(const char *key, const void *value, unsigned int size,
                 int persistent) {
    if (key == NULL || value == NULL || zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    char buffer[512];
    struct ACL CREATE_ONLY_ACL[] = {{ZOO_PERM_CREATE, ZOO_AUTH_IDS}};
    struct ACL_vector CREATE_ONLY = {1, CREATE_ONLY_ACL};
    int rc = zoo_create(zh, key, value, size, &CREATE_ONLY,
                        persistent ? ZOO_PERSISTENT : ZOO_EPHEMERAL, buffer,
                        sizeof(buffer) - 1);

    if (rc == ZNODEEXISTS) {
        errno = EEXIST;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    return 0;
}
