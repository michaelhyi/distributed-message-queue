#include "network.h"

int main() {
    // TODO: take port in as arg
    int res = init_server(8080);
    if (res < 0) {
        return 1;
    }

    return 0;
}