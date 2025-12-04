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
    if (message_size - sizeof(struct dmqp_header) <=
        dmqp_message.header.length) { // payload already received
        memcpy(dmqp_message.payload,
               (char *)message + sizeof(struct dmqp_header),
               dmqp_message.header.length);
    } else { // payload not fully received, request more bytes from
             // `conn_socket`
        memcpy(dmqp_message.payload,
               (char *)message + sizeof(struct dmqp_header),
               message_size - sizeof(struct dmqp_header));

        res = receive_message(conn_socket,
                              (char *)dmqp_message.payload + message_size -
                                  sizeof(struct dmqp_header),
                              dmqp_message.header.length - message_size +
                                  sizeof(struct dmqp_header));
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
