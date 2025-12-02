#include "network.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    unsigned int server_port = DEFAULT_SERVER_PORT;

    if (argc >= 3 && strcmp(argv[1], "-p") == 0) {
        server_port = atoi(argv[2]);
    }

    int res = init_server(server_port);
    if (res < 0) {
        return 1;
    }

    return 0;
}