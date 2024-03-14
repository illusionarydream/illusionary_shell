#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void handle_client(int client_socket) {
    // *read and write data
    char buffer[1024];
    while (1) {
        // *initialize the buffer
        memset(buffer, 0, sizeof(buffer));

        // *read from the client
        int nread = read(client_socket, buffer, sizeof(buffer));

        // *handle the error
        if (nread < 0) {
            perror("read");
            break;
        }

        write(client_socket, buffer, nread);
    }
    close(client_socket);
}

int main() {
    // *build a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // *bind the socket to an IP address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    inet_pton(AF_INET, "127.0.0.1", &(server_address.sin_addr));
    bind(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // *listen for connections
    listen(network_socket, 5);

    // *the main loop
    while (1) {
        // *accept a connection
        int client_socket;
        struct sockaddr_in client_address;
        int len = sizeof(client_address);
        client_socket = accept(network_socket, (struct sockaddr *)&client_address, &len);

        // *print out the client's IP address
        printf("Connection established\n");
        printf("Client IP: %s\n", inet_ntoa(client_address.sin_addr));

        // *handle the client
        handle_client(client_socket);
    }
    return 0;
}