#include "metadata.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zookeeper/zookeeper.h>

#define OCTET_LENGTH 3 // 0-255 has max 3 digits
// ipv4 format: octet.octet.octet.octet
#define IPV4_LENGTH 4 * OCTET_LENGTH + 3

#define PORT_LENGTH 5 // 0-65535 has max 5 digits
// server addr format: ipv4:port
#define SERVER_ADDRESS_LENGTH IPV4_LENGTH + 1 + PORT_LENGTH

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

    char server_address[SERVER_ADDRESS_LENGTH + 1];
    memset(server_address, 0, sizeof(server_address));

    int host_len = strnlen(server->host, IPV4_LENGTH);
    strncpy(server_address, server->host, host_len);
    server_address[host_len] = ':';
    sprintf(server_address + host_len + 1, "%d", server->port);

    zh = zookeeper_init(server_address, watcher, 10000, 0, 0, 0);
    if (!zh) {
        errno = EIO;
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
    zh = NULL;
    return 0;
}

int metadata_get(const char *key, void *value) {
    if (key == NULL || value == NULL || zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct Stat stat;
    int buffer_len = MAX_METADATA_ENTRY_SIZE;
    int rc = zoo_get(zh, key, 0, value, &buffer_len, &stat);
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
    if (key == NULL || (value == NULL && size != 0) || zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    char buffer[MAX_METADATA_ENTRY_SIZE];
    int zookeeper_flags = persistent ? ZOO_PERSISTENT : ZOO_EPHEMERAL;
    int rc = zoo_create(zh, key, value, size, &ZOO_OPEN_ACL_UNSAFE,
                        zookeeper_flags, buffer, sizeof(buffer) - 1);

    if (rc == ZNODEEXISTS) {
        errno = EEXIST;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int metadata_delete(const char *key) {
    if (key == NULL || zh == NULL) {
        errno = EINVAL;
        return -1;
    }

    int rc = zoo_delete(zh, key, -1);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    return 0;
}
