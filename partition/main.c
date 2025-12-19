#include <getopt.h>
#include <stdlib.h>

#include "dmqp.h"
#include "partition.h"

int main(int argc, char *argv[]) {
    unsigned int server_port = DEFAULT_SERVER_PORT;

    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            server_port = atoi(optarg);
            break;
        case '?':
            return 1;
        default:
            break;
        }
    }

    int res = partition_init(server_port);
    if (res < 0) {
        return 1;
    }

    res = partition_destroy();
    if (res < 0) {
        return 1;
    }

    return 0;
}
