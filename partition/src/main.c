#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "network.h"

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

    int res = server_init(server_port);
    if (res < 0) {
        return 1;
    }

    return 0;
}
