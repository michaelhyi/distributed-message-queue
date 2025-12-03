#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "network.h"

int main() {
    char *data = "Hello, World!";

    struct message_header header;
    header.flags = 0;
    header.type = PUSH;
    header.length = strlen(data);

    struct message message;
    message.header = header;
    message.payload = data;


    int client_socket = client_init("127.0.0.1", 8080);
    if (client_socket < 0) {
        return 1;
    }

    int res = send_message(client_socket, message);
    if (res < 0) {
        close(client_socket);
        return 1;
    }

    printf("sent message\n");
    close(client_socket);
    return 0;
}
