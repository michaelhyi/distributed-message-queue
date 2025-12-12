#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "dmqp.h"
#include "network.h"

int main() {
    int fd = client_init("127.0.0.1", 8080);
    if (fd < 0) {
        fprintf(stderr, "failed to init partition client\n");
        return 1;
    }

    int res;
    struct dmqp_header header = {.flags = 0, .queue_entry_timestamp = 0};
    struct dmqp_message message;
    char *data[] = {"Michael Yi", "Christian Beckering", "Philip Mitchell"};

    for (int i = 0; i < 3; i++) {
        header.method = PUSH;
        header.length = strlen(data[i]);
        message.header = header;
        message.payload = data[i];

        res = send_dmqp_message(fd, &message, 0);
        if (res < 0) {
            fprintf(stderr, "failed to send message %d\n", i);
            return 1;
        }

        printf("pushed: %s\n", (char *)message.payload);

        res = read_dmqp_message(fd, &message);
        if (res < 0) {
            fprintf(stderr, "failed to receive message %d\n", i);
            return 1;
        }

        printf("received message:\n");
        printf("method: %d\n", message.header.method);
        printf("flags: %d\n", message.header.flags);
        printf("queue_entry_timestamp: %ld\n",
               message.header.queue_entry_timestamp);
        printf("length: %d\n", message.header.length);
        printf("payload: %s\n", (char *)message.payload);
    }

    header.method = PEEK;
    header.length = 0;
    message.header = header;
    message.payload = NULL;
    res = send_dmqp_message(fd, &message, 0);
    if (res < 0) {
        fprintf(stderr, "failed to send message\n");
        return 1;
    }

    printf("peeked\n");

    res = read_dmqp_message(fd, &message);
    if (res < 0) {
        fprintf(stderr, "failed to receive message\n");
        return 1;
    }

    printf("received message:\n");
    printf("method: %d\n", message.header.method);
    printf("flags: %d\n", message.header.flags);
    printf("queue_entry_timestamp: %ld\n",
           message.header.queue_entry_timestamp);
    printf("length: %d\n", message.header.length);
    printf("payload: %s\n", (char *)message.payload);

    header.method = POP;
    header.length = 0;
    message.header = header;
    message.payload = NULL;
    res = send_dmqp_message(fd, &message, 0);
    if (res < 0) {
        fprintf(stderr, "failed to send message\n");
        return 1;
    }

    printf("popped\n");

    res = read_dmqp_message(fd, &message);
    if (res < 0) {
        fprintf(stderr, "failed to receive message\n");
        return 1;
    }

    printf("received message:\n");
    printf("method: %d\n", message.header.method);
    printf("flags: %d\n", message.header.flags);
    printf("queue_entry_timestamp: %ld\n",
           message.header.queue_entry_timestamp);
    printf("length: %d\n", message.header.length);
    printf("payload: %s\n", (char *)message.payload);

    header.method = PEEK;
    header.length = 0;
    message.header = header;
    message.payload = NULL;
    res = send_dmqp_message(fd, &message, 0);
    if (res < 0) {
        fprintf(stderr, "failed to send message\n");
        return 1;
    }

    printf("peeked\n");

    res = read_dmqp_message(fd, &message);
    if (res < 0) {
        fprintf(stderr, "failed to receive message\n");
        return 1;
    }

    printf("received message:\n");
    printf("method: %d\n", message.header.method);
    printf("flags: %d\n", message.header.flags);
    printf("queue_entry_timestamp: %ld\n",
           message.header.queue_entry_timestamp);
    printf("length: %d\n", message.header.length);
    printf("payload: %s\n", (char *)message.payload);

    header.method = POP;
    header.length = 0;
    message.header = header;
    message.payload = NULL;
    res = send_dmqp_message(fd, &message, 0);
    if (res < 0) {
        fprintf(stderr, "failed to send message\n");
        return 1;
    }

    printf("popped\n");

    res = read_dmqp_message(fd, &message);
    if (res < 0) {
        fprintf(stderr, "failed to send message\n");
        return 1;
    }

    printf("received message:\n");
    printf("method: %d\n", message.header.method);
    printf("flags: %d\n", message.header.flags);
    printf("queue_entry_timestamp: %ld\n",
           message.header.queue_entry_timestamp);
    printf("length: %d\n", message.header.length);
    printf("payload: %s\n", (char *)message.payload);
    return 0;
}
