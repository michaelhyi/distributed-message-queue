#include <stdlib.h>
#include <string.h>

#include "network.h"

int main(int argc, char *argv[]) {
    unsigned int server_port = DEFAULT_SERVER_PORT;

    // TODO: use getopts
    if (argc >= 3 && strcmp(argv[1], "-p") == 0) {
        server_port = atoi(argv[2]);
    }

    int res = server_init(server_port);
    if (res < 0) {
        return 1;
    }

    return 0;
}