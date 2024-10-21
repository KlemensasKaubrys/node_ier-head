#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>  
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <errno.h>

#define PORT 9000
#define BUFFER_SIZE 4096  // Increased buffer size to handle longer URLs

volatile int running = 1;

void handle_sigint(int sig) {
    running = 0;
    printf("\nShutting down server...\n");
}

// Function to parse the 'rate' query parameter from the request
int parse_update_rate(const char *buffer) {
    int rate = 1000; // Default rate in milliseconds
    char *rate_str = NULL;
    char *query_start = strstr(buffer, "GET /cpu_usage?");
    if (query_start) {
        char *query = query_start + strlen("GET /cpu_usage?");
        char *query_end = strchr(query, ' ');
        if (query_end) {
            char query_string[1024];
            strncpy(query_string, query, query_end - query);
            query_string[query_end - query] = '\0';

            // Parse query parameters
            char *token = strtok(query_string, "&");
            while (token != NULL) {
                if (strncmp(token, "rate=", 5) == 0) {
                    rate_str = token + 5;
                    break;
                }
                token = strtok(NULL, "&");
            }

            if (rate_str != NULL) {
                int parsed_rate = atoi(rate_str);
                if (parsed_rate > 0) {
                    rate = parsed_rate;
                }
            }
        }
    }
    return rate;
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
        if (strstr(buffer, "GET /cpu_usage")) {
            printf("Client requested CPU usage data.\n");

            // Parse the update rate from the query parameters
            int client_update_rate = parse_update_rate(buffer);
            printf("Client update rate: %d milliseconds\n", client_update_rate);

            // Send headers
            char headers[] = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/event-stream\r\n"
                             "Cache-Control: no-cache\r\n"
                             "Connection: keep-alive\r\n\r\n";
            send(sock, headers, strlen(headers), 0);

            // Initialize CPU usage variables
            unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;

            // Send CPU usage data periodically
            while (running) {
                FILE *fp;
                char cpu_info[128];
                double cpu_usage = 0.0;

                // Read CPU usage from /proc/stat
                fp = fopen("/proc/stat", "r");
                if (fp != NULL) {
                    unsigned long long user, nice, system, idle;
                    fgets(cpu_info, sizeof(cpu_info), fp);
                    sscanf(cpu_info, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle);
                    fclose(fp);

                    unsigned long long diff_user = user - prev_user;
                    unsigned long long diff_nice = nice - prev_nice;
                    unsigned long long diff_system = system - prev_system;
                    unsigned long long diff_idle = idle - prev_idle;

                    unsigned long long total_diff = diff_user + diff_nice + diff_system + diff_idle;
                    unsigned long long idle_diff = diff_idle;

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

                ssize_t bytes_sent = send(sock, data, strlen(data), 0);
                if (bytes_sent == -1) {
                    perror("send failed");
                    break; // Exit the loop if send fails
                }

                printf("Sent data to client: %s", data);

                usleep(client_update_rate * 1000); // Sleep for specified milliseconds
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

    // Handle Ctrl+C and SIGPIPE
    signal(SIGINT, handle_sigint);
    signal(SIGPIPE, SIG_IGN);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Set SO_REUSEADDR to reuse the port immediately after the program exits
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
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
    listen(server_fd, 5);

    printf("Backend server listening on port %d\n", PORT);

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
            free(new_sock);
            close(client_sock);
            continue;
        }
        pthread_detach(client_thread);
    }

    close(server_fd);
    printf("Server shut down.\n");
    return 0;
}

