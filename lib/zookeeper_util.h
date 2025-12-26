#ifndef ZOOKEEPER_UTIL_H
#define ZOOKEEPER_UTIL_H

#include <zookeeper/zookeeper.h>

/**
 * Deletes a ZNode and its children recursively.
 *
 * @param zh the zookeeper handle obtained by a call to `zookeeper_init`
 * @param path path of ZNode
 * @param version the expected version of the node. The function will fail if
 * the actual version of the node does not match the expected version. If -1 is
 * used the version check will not take place.
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments
 * @throws `ENODATA` if ZNode does not exist
 * @throws `EIO` internal service error
 */
int zoo_deleteall(zhandle_t *zh, const char *path, int version);

#endif
