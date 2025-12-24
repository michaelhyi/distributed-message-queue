// Defines a metadata service backed by a key-value store, representing data in
// the format of a tree.

#ifndef METADATA_H
#define METADATA_H

#include "types.h"

#define PERSISTENT 1
#define EPHEMERAL 0

#define MAX_METADATA_ENTRY_SIZE 512

/**
 * Initializes the metadata service by establishing a connection to the metadata
 * server.
 *
 * @param server metadata server
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments
 * @throws `EALREADY` metadata service already initialized
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
 * Gets the keys of all child entries given a parent entry's key.
 *
 * @param key parent entry's key
 * @param child_entries output param to return the keys of child entries
 * @param num_child_entries output param to return the number of child entries
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `ENODATA` metadata entry does not exist
 * @throws `EIO` internal service error
 */
int metadata_get_children(const char *key, char ***child_entries,
                          unsigned int *num_child_entries);

/**
 * Creates a metadata entry.
 *
 * @param key key of metadata entry
 * @param value value to store in metadata entry
 * @param size number of bytes to store in metadata entry
 * @param persistent if PERSISTENT (non-zero), creates a persistent metadata
 * entry. if EPHEMERAL (zero), metadata entries will be deleted if the client
 * that created them dies
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `EEXIST` if metadata entry with same key already exists
 * @throws `EIO` internal service error
 */
int metadata_set(const char *key, const void *value, unsigned int size,
                 int persistent);

/**
 * Deletes a metadata entry.
 *
 * @param key key of metadata entry
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `ENODATA` if metadata entry does not exist
 * @throws `EIO` internal service error
 */
int metadata_delete(const char *key);

/**
 * Deletes a metadata entry and its children recursively.
 *
 * @param key key of metadata entry
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or metadata service not initialized
 * @throws `ENODATA` if metadata entry does not exist
 * @throws `EIO` internal service error
 */
int metadata_delete_recursive(const char *key);

#endif
