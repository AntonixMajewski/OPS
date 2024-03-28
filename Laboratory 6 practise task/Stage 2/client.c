#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>

#define MAX_BUF_SIZE 256

void create_client_queue(char *pid_str, mqd_t *client_queue) {
    // Queue name with PID appended
    char client_queue_name[MAX_BUF_SIZE];
    sprintf(client_queue_name, "/%s", pid_str);

    // Create client queue
    *client_queue = mq_open(client_queue_name, O_WRONLY, 0644, NULL);

    // Check if creation was successful
    if (*client_queue == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("Client queue created: %s\n", client_queue_name);
}

void destroy_client_queue(char *pid_str, mqd_t client_queue) {
    // Queue name with PID appended
    char client_queue_name[MAX_BUF_SIZE];
    sprintf(client_queue_name, "/%s", pid_str);

    // Close and unlink client queue
    mq_close(client_queue);
    mq_unlink(client_queue_name);
}

void sigint_handler(int sig) {
    printf("Received SIGINT. Cleaning up and terminating.\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Check if server queue name is provided as argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_queue_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Register SIGINT handler
    signal(SIGINT, sigint_handler);

    // Get client PID
    pid_t pid = getpid();
    char pid_str[MAX_BUF_SIZE];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    mqd_t client_queue;

    // Create client queue
    create_client_queue(pid_str, &client_queue);

    // Read 2 integers from stdin
    int num1, num2;
    printf("Enter two integers: ");
    scanf("%d %d", &num1, &num2);

    // Send a single message to the server containing client PID and the two integers
    mq_send(client_queue, (const char *)&pid, sizeof(pid), 0);
    mq_send(client_queue, (const char *)&num1, sizeof(num1), 0);
    mq_send(client_queue, (const char *)&num2, sizeof(num2), 0);

    // Wait for the result from the server
    uint8_t response;
    mq_receive(client_queue, (char *)&response, 1, NULL);
    printf("Received response from server: %d\n", response);

    // Destroy client queue
    destroy_client_queue(pid_str, client_queue);

    return EXIT_SUCCESS;
}