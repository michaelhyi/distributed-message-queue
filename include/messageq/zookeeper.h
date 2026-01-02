#ifndef ZOOKEEPER_H
#define ZOOKEEPER_H

#include <zookeeper/zookeeper.h>

/**
 * Copied from <zookeeper/zookeeper.h> with slight modifications.
 *
 * Synchronously create a handle to used communicate with zookeeper.
 *
 * @param host comma separated host:port pairs, each corresponding to a zk
 * server. e.g. "127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002"
 * @return a pointer to the opaque zhandle structure. If it fails to create
 * a new zhandle the function returns NULL and the errno variable
 * indicates the reason.
 * @throws `EINVAL` invalid args
 * @throws `ETIMEDOUT` conn timeout
 * @throws `ECONNREFUSED` conn failure
 */
zhandle_t *zoo_init(const char *host);

/**
 * Deletes a ZNode and its children recursively.
 *
 * @param zh the zookeeper handle obtained by a call to `zoo_init()`
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
