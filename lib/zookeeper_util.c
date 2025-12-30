#include "zookeeper_util.h"

#include <errno.h>
#include <string.h>

int zoo_deleteall(zhandle_t *zh, const char *path, int version) {
    if (!zh || !path) {
        errno = EINVAL;
        return -1;
    }

    struct String_vector children;
    int rc = zoo_get_children(zh, path, 0, &children);
    if (rc == ZNONODE) {
        errno = ENODATA;
        return -1;
    } else if (rc) {
        errno = EIO;
        return -1;
    }

    for (int i = 0; i < children.count; i++) {
        char child_path[512] = {0};
        snprintf(child_path, sizeof child_path, "%s/%s", path,
                 children.data[i]);
        if (zoo_deleteall(zh, child_path, version) < 0) {
            return -1;
        }
    }

    if (zoo_delete(zh, path, version)) {
        errno = EIO;
        return -1;
    }

    return 0;
}
