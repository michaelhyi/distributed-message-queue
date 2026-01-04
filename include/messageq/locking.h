#ifndef LOCKING_H
#define LOCKING_H

#include <zookeeper/zookeeper.h>

// TODO: test these functions

/**
 * Acquires a distributed lock, blocking until acquired.
 *
 * @param lock znode path of distributed lock
 * @param zh zookeeper handle for acquiring lock
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `ENODATA` lock not found
 * @throws `EIO` unexpected error
 */
int acquire_distributed_lock(const char *lock, zhandle_t *zh);

/**
 * Releases a distributed lock.
 *
 * @param lock znode path of distributed lock
 * @param zh zookeeper handle for releasing lock
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid args
 * @throws `ENODATA` lock not found
 * @throws `EPERM` caller not holding lock
 * @throws `EIO` unexpected error
 */
int release_distributed_lock(const char *lock, zhandle_t *zh);

#endif
