#ifndef API_H
#define API_H

#include "types.h"

#define MAX_TOPIC_NAME_LEN 32

/**
 * Initializes the client by establishing a connection to the metadata
 * server.
 *
 * @param server metadata server
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments
 * @throws `EALREADY` client already initialized
 * @throws `EIO` internal service error, likely establishing connection to
 * metadata server failed
 */
int client_init(const struct server *server);

/**
 * Closes connections initialized by the client.
 *
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` client not initialized
 */
int client_destroy(void);

/**
 * Creates a topic in the distributed message queue, allocating partitions as
 * requested. Initializes the topic's sequence id, shards, and elects
 * leader/replica partitions.
 *
 * @param topic the topic to create
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `EEXIST` topic already exists
 * @throws `ENODEV` partition allocation failure
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int create_topic(const struct topic *topic);

/**
 * Updates a topic in the distributed message queue, updating partition
 * allocation as requested.
 *
 * @param topic the topic to update
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `ENODATA` topic does not exist
 * @throws `ENODEV` partition allocation failure
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int update_topic(const struct topic *topic);

/**
 * Deletes a topic in the distributed message queue, freeing all allocated
 * partitions.
 *
 * @param topic_name name of the topic to delete
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `ENODATA` topic does not exist
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int delete_topic(const char *topic_name);

/**
 * Registers the current process as a consumer of the distributed message queue.
 *
 * @param topic_name name of the topic to delete
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `ENODATA` topic does not exist
 * @throws `EMFILE` topic's consumer limit reached
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int consumer_init(const char *topic_name);

/**
 * Pushes a message onto the distributed message queue by using Round-Robin to
 * distribute between shards.
 *
 * @param topic_name name of the topic to push to
 * @param buf the message to push
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `ENODATA` topic does not exist
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int push(const char *topic_name, const struct message *buf);

/**
 * Pops a message from the distributed message queue, only popping from the
 * shard assigned to the consumer.
 *
 * @param topic_name name of the topic to pop from
 * @param buf output param to return the message
 * @returns 0 if success, -1 if error with global `errno` set
 * @throws `EINVAL` invalid arguments or topic name too long (max 32 chars)
 * @throws `ENODATA` topic does not exist
 * @throws `EACCES` current process not assigned as a consumer of this topic
 * @throws `EIO` internal service error, likely connection to distributed system
 * servers fail
 */
int pop(const char *topic_name, struct message *buf);

#endif
