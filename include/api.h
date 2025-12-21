#ifndef API_H
#define API_H

struct server {
    char *host;
    unsigned short port;
};

struct topic {
    char *name;
    unsigned int shards;
    unsigned int replication_factor;
};

struct message {
    void *data;
    unsigned int size;
};

/**
 * Creates a topic in the distributed message queue, allocating partitions as
 * requested.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, the topic already exists, or partition allocation
 * fails.
 *
 * @param server the distributed system's metadata server
 * @param topic the topic to create
 * @returns 0 if success, -1 if error with global `errno` set
 */
int create_topic(const struct server *server, const struct topic *topic);

/**
 * Updates a topic in the distributed message queue, updating partition
 * allocation as requested.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, the topic does not exist, or partition allocation
 * fails.
 *
 * @param server the distributed system's metadata server
 * @param topic the topic to update
 * @returns 0 if success, -1 if error with global `errno` set
 */
int update_topic(const struct server *server, const struct topic *topic);

/**
 * Deletes a topic in the distributed message queue, freeing all allocated
 * partitions.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, or the topic does not exist.
 *
 * @param server the distributed system's metadata server
 * @param topic_name name of the topic to delete
 * @returns 0 if success, -1 if error with global `errno` set
 */
int delete_topic(const struct server *server, const char *topic_name);

/**
 * Registers the current process as a consumer of the distributed message queue.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, the topic does not exist, or the topic's consumer
 * limit has been reached.
 *
 * @param server the distributed system's metadata server
 * @param topic_name name of the topic to delete
 * @returns 0 if success, -1 if error with global `errno` set
 */
int consumer_init(const struct server *server, const char *topic_name);

/**
 * Pushes a message onto the distributed message queue by using Round-Robin to
 * distribute between shards.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, or the topic does not exist.
 *
 * @param server the distributed system's metadata server
 * @param topic_name name of the topic to push to
 * @param buf the message to push
 * @returns 0 if success, -1 if error with global `errno` set
 */
int push(const struct server *server, const char *topic_name,
         const struct message *buf);

/**
 * Pops a message from the distributed message queue, only popping from the
 * shard assigned to the consumer.
 *
 * Throws an error if arguments are null, connections to the distributed
 * system's servers fail, the topic does not exist, or the current process is
 * not assigned a shard from this topic.
 *
 * @param server the distributed system's metadata server
 * @param topic_name name of the topic to pop from
 * @param buf output param to return the message
 * @returns 0 if success, -1 if error with global `errno` set
 */
int pop(const struct server *server, const char *topic_name,
        struct message *buf);

#endif
