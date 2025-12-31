#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "partition.h"

#define MAX_HOST_LEN 21 // ipv4:port fomat xxx.xxx.xxx.xxx:xxxxx

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
            strcpy(service_discovery_host, optarg);
            service_discovery_host[MAX_HOST_LEN] = '\0';
            break;
        default:
            fprintf(stderr, "Usage: %s [-s] [host:port]\n", argv[0]);
            return 1;
        }
    }

    if (partition_init(service_discovery_host) < 0) {
        return 1;
    }

    partition_destroy();
    return 0;
}
