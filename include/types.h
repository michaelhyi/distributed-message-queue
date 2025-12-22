#ifndef TYPES_H
#define TYPES_H

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

#endif
