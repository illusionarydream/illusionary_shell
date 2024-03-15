#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// *parse the command line
int parseLine(char *line, char *command_array[]) {
    int i = 0;
    char *token = strtok(line, " ");

    while (token != NULL) {
        command_array[i] = token;
        token = strtok(NULL, " ");
        i++;
    }

    command_array[i] = NULL;
    return i;
}

int execute_command(int client_socket, char *segment_array[], int n) {
    // *create many pipes
    int all_pipes[100][2];

    // *if there is only one segment
    if (n == 1) {
        // *parse the command line
        char *command_array[100];
        int command_length = parseLine(segment_array[0], command_array);

        // ?debug
        for (int j = 0; j < command_length; j++) {
            printf("Command %d: %s\n", j, command_array[j]);
        }

        // *execute the exit command
        if (strcmp(command_array[0], "exit") == 0) {
            return 0;
        }

        // *create a child process
        pid_t pid = fork();
        switch (pid) {
            case -1:
                // *handle the error
                printf("Error: creating a child process\n");
                exit(0);

            case 0:
                dup2(client_socket, STDOUT_FILENO);
                // *execute the command
                if (execvp(command_array[0], command_array) == -1) {
                    printf("Error: executing the command: %s\n", segment_array[0]);
                }
                exit(0);

            default:
                // *wait for the child process to terminate
                waitpid(pid, NULL, 0);
        }
        return 1;
    }

    // *execute loop
    for (int i = 0; i < n; i++) {
        // *sparse the command line
        char *command_array[100];
        int command_length = parseLine(segment_array[i], command_array);

        // ?debug
        for (int j = 0; j < command_length; j++) {
            printf("Command %d: %s\n", j, command_array[j]);
        }

        // *create a pipe
        pipe(all_pipes[i]);

        // *create a child process
        pid_t pid = fork();
        switch (pid) {
            case -1:
                // *handle the error
                printf("Error: creating a child process\n");
                exit(0);

            case 0:
                // *redirect the standard input and output
                if (i == 0) {
                    // *the first segment
                    close(all_pipes[i][0]);                // close the read end
                    dup2(all_pipes[i][1], STDOUT_FILENO);  // redirect the standard output to the write end
                    close(all_pipes[i][1]);                // close the write end
                } else if (i == n - 1) {
                    // *the last segment
                    close(all_pipes[i][0]);
                    close(all_pipes[i][1]);
                    close(all_pipes[i - 1][1]);               // close the write end
                    dup2(all_pipes[i - 1][0], STDIN_FILENO);  // redirect the standard input to the read end
                    close(all_pipes[i - 1][0]);               // close the read end
                    dup2(client_socket, STDOUT_FILENO);       // redirect the standard output to the client
                } else {
                    // *the middle segment
                    close(all_pipes[i][0]);                   // close the read end
                    dup2(all_pipes[i][1], STDOUT_FILENO);     // redirect the standard output to the write end
                    close(all_pipes[i][1]);                   // close the write end
                    close(all_pipes[i - 1][1]);               // close the write end
                    dup2(all_pipes[i - 1][0], STDIN_FILENO);  // redirect the standard input to the read end
                    close(all_pipes[i - 1][0]);               // close the read end
                }

                // *execute the command
                if (execvp(command_array[0], command_array) == -1) {
                    printf("Error: executing the command: %s\n", segment_array[i]);
                }
                exit(0);
                break;

            default:
                // *close the pipe
                if (i == n - 1) {
                    close(all_pipes[n - 1][0]);
                    close(all_pipes[n - 1][1]);
                }
                if (i - 1 >= 0) {
                    close(all_pipes[i - 1][0]);
                    close(all_pipes[i - 1][1]);
                }

                // *wait for the child process to terminate
                waitpid(pid, NULL, 0);
                continue;
        }
    }
    return 1;
}

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

        // *remove the newline character
        int tmp_len = strlen(buffer);
        printf("Client: %d\n", tmp_len);
        buffer[tmp_len - 1] = '\0';
        buffer[tmp_len - 2] = '\0';

        // *split the command and the arguments
        char *segment_array[100];
        char *segment;
        segment = strtok(buffer, "|");
        int i = 0;
        while (segment != NULL) {
            segment_array[i] = segment;
            segment = strtok(NULL, "|");
            i++;
        }

        // ?debug
        for (int j = 0; j < i; j++) {
            printf("Segment %d: %s\n", j, segment_array[j]);
        }

        // *execute the command
        int if_exit = execute_command(client_socket, segment_array, i);

        // *if the command is exit
        if (!if_exit) {
            close(client_socket);
            break;
        }
        // // *close the socket
        // close(client_socket);
    }
}

int main() {
    // *build a socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // *bind the socket to an IP address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(12003);
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