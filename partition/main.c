#include "partition.h"

int main() {
    if (partition_init() < 0) {
        return 1;
    }

    partition_destroy();
    return 0;
}
