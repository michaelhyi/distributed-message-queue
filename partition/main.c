#include <messageq/constants.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "partition.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [-s] [host:port]\n", argv[0]);
        return 1;
    }

    int opt;
    char service_discovery_host[MAX_HOST_LEN + 1];

    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
        case 's':
            strncpy(service_discovery_host, optarg, MAX_HOST_LEN);
            service_discovery_host[MAX_HOST_LEN] = '\0';
            break;
        default:
            fprintf(stderr, "Usage: %s [-s] [host:port]\n", argv[0]);
            return 1;
        }
    }

    if (start_partition(service_discovery_host) < 0) {
        fprintf(stderr, "Failed to start partition: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}
