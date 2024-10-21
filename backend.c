#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>  
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define BUFFER_SIZE 1024

volatile int running = 1;
int update_rate = 1; // Default update rate in seconds

void handle_sigint(int sig) {
    running = 0;
    printf("\nShutting down server...\n");
}

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);

    char buffer[BUFFER_SIZE];
    int read_size;

    // Read request
    read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (read_size > 0) {
        buffer[read_size] = '\0';
        // Check if the request is for CPU usage data
        if (strstr(buffer, "GET /backend/cpu_usage")) {
            printf("Client requested CPU usage data.\n");

            // Send headers
            char headers[] = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n";
            send(sock, headers, strlen(headers), 0);

            // Send CPU usage data periodically
            while (running) {
                FILE *fp;
                char cpu_info[128];
                double cpu_usage = 0.0;

                // Read CPU usage from /proc/stat
                fp = fopen("/proc/stat", "r");
                if (fp != NULL) {
                    unsigned long long int user, nice, system, idle;
                    fgets(cpu_info, sizeof(cpu_info), fp);
                    sscanf(cpu_info, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle);
                    fclose(fp);

                    static unsigned long long int prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;

                    unsigned long long int diff_user = user - prev_user;
                    unsigned long long int diff_nice = nice - prev_nice;
                    unsigned long long int diff_system = system - prev_system;
                    unsigned long long int diff_idle = idle - prev_idle;

                    unsigned long long int total_diff = diff_user + diff_nice + diff_system + diff_idle;
                    unsigned long long int idle_diff = diff_idle;

                    if (total_diff != 0) {
                        cpu_usage = (double)(total_diff - idle_diff) / total_diff * 100.0;
                    }

                    prev_user = user;
                    prev_nice = nice;
                    prev_system = system;
                    prev_idle = idle;
                }

                char data[128];
                snprintf(data, sizeof(data), "data: %.2f\n\n", cpu_usage);
                send(sock, data, strlen(data), 0);

                printf("Sent data to client: %s", data);  // Verbose print

                sleep(update_rate);
            }
        } else {
            // Send 404 Not Found
            char response[] = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(sock, response, strlen(response), 0);
            printf("Client requested unknown resource. Sent 404.\n");
        }
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_fd, client_sock, c;
    struct sockaddr_in server, client;

    // Handle Ctrl+C
    signal(SIGINT, handle_sigint);

    // Allow adjustable update rate via command line argument
    if (argc > 1) {
        update_rate = atoi(argv[1]);
        if (update_rate <= 0) {
            update_rate = 1; // Reset to default if invalid
        }
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Listen
    listen(server_fd, 3);

    printf("Backend server listening on port %d\n", PORT);
    printf("Update rate: %d seconds\n", update_rate);

    c = sizeof(struct sockaddr_in);

    // Accept incoming connections
    while (running) {
        client_sock = accept(server_fd, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0) {
            if (running) {
                perror("Accept failed");
            }
            break;
        }

        // Handle client in a new thread
        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        if (pthread_create(&client_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
        pthread_detach(client_thread);
    }

    close(server_fd);
    printf("Server shut down.\n");
    return 0;
}

