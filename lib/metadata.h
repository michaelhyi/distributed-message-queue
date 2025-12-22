#ifndef METADATA_H
#define METADATA_H

#include "api.h"

const int MAX_METADATA_ENTRY_SIZE = 512;

/**
 * Initializes the metadata service by establishing a connection to the metadata
 * server.
 *
 * @param server metadata server
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments
 * @throws `EALREADY` metadata service already initialized
 * @throws `EINVAL` metadata service not initialized
 * @throws `EIO` internal service error, likely establishing connection to
 * metadata server failed
 */
int metadata_init(const struct server *server);

/**
 * Cleans up the metadata service.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` metadata service not initialized
 */
int metadata_destroy(void);

/**
 * Gets a metadata entry's value by key.
 *
 * @param key key of metadata entry
 * @param value output param of metadata entry's value. must be a buffer with a
 * min size of 512 bytes
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `ENODATA` if metadata entry does not exist
 * @throws `EIO` internal service error
 */
int metadata_get(const char *key, void *value);

/**
 * Creates a metadata entry.
 *
 * @param key key of metadata entry
 * @param value value to store in metadata entry
 * @param size number of bytes to store in metadata entry
 * @param persistent if non-zero, creates a persistent metadata entry.
 * otherwise, metadata entries will be deleted if the client that created them
 * dies
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `EEXIST` if metadata entry with same key already exists
 */
int metadata_set(const char *key, const void *value, unsigned int size,
                 int persistent);

#endif
