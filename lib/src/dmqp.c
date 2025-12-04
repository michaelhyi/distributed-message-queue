#include "dmqp.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"

int handle_server_message(void *message, unsigned int message_size,
                          int conn_socket) {
    if (message == NULL || message_size < sizeof(struct dmqp_header) ||
        conn_socket < 0) {
        errno = EINVAL;
        return -1;
    }

    struct dmqp_message dmqp_message;
    dmqp_message.header = *(struct dmqp_header *)message;
    if (dmqp_message.header.length > MAX_PAYLOAD_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    dmqp_message.payload = malloc(dmqp_message.header.length);
    if (dmqp_message.payload == NULL) {
        errno = ENOMEM;
        return -1;
    }

    int res;
    char *payload = (char *)message + sizeof(struct dmqp_header);
    unsigned int available_bytes = message_size - sizeof(struct dmqp_header);
    unsigned int required_bytes = dmqp_message.header.length;

    if (available_bytes >= required_bytes) { // payload already received
        memcpy(dmqp_message.payload, payload, dmqp_message.header.length);
    } else { // payload not fully received, request more bytes from
             // `conn_socket`
        memcpy(dmqp_message.payload, payload, available_bytes);

        unsigned int remaining_bytes = required_bytes - available_bytes;
        res = receive_message(conn_socket,
                              (char *)dmqp_message.payload + available_bytes,
                              remaining_bytes);
        if (res < 0) {
            free(dmqp_message.payload);
            return -1;
        }
    }

    res = handle_dmqp_message(dmqp_message);
    if (res < 0) {
        free(dmqp_message.payload);
        return -1;
    }

    free(dmqp_message.payload);
    return 0;
}
