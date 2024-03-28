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

void create_server_queues(char *pid_str, mqd_t *s_queue, mqd_t *d_queue, mqd_t *m_queue) {
    // Queue names with PID appended
    char s_queue_name[MAX_BUF_SIZE], d_queue_name[MAX_BUF_SIZE], m_queue_name[MAX_BUF_SIZE];

    sprintf(s_queue_name, "/%s_s", pid_str);
    sprintf(d_queue_name, "/%s_d", pid_str);
    sprintf(m_queue_name, "/%s_m", pid_str);

    // Create queues
    *s_queue = mq_open(s_queue_name, O_RDONLY, 0644, NULL);
    *d_queue = mq_open(d_queue_name, O_RDONLY, 0644, NULL);
    *m_queue = mq_open(m_queue_name, O_RDONLY, 0644, NULL);

    // Check if creation was successful
    if (*s_queue == (mqd_t)-1 || *d_queue == (mqd_t)-1 || *m_queue == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    // Print names of the queues
    printf("Server queues created:\n");
    printf("PID_s: %s\n", s_queue_name);
    printf("PID_d: %s\n", d_queue_name);
    printf("PID_m: %s\n", m_queue_name);
}

void destroy_server_queues(char *pid_str, mqd_t s_queue, mqd_t d_queue, mqd_t m_queue) {
    // Queue names with PID appended
    char s_queue_name[MAX_BUF_SIZE], d_queue_name[MAX_BUF_SIZE], m_queue_name[MAX_BUF_SIZE];

    sprintf(s_queue_name, "/%s_s", pid_str);
    sprintf(d_queue_name, "/%s_d", pid_str);
    sprintf(m_queue_name, "/%s_m", pid_str);

    // Close and unlink queues
    mq_close(s_queue);
    mq_close(d_queue);
    mq_close(m_queue);
    mq_unlink(s_queue_name);
    mq_unlink(d_queue_name);
    mq_unlink(m_queue_name);
}

void sigint_handler(int sig) {
    printf("Received SIGINT. Cleaning up and terminating.\n");
    exit(EXIT_SUCCESS);
}

int main() {
    // Register SIGINT handler
    signal(SIGINT, sigint_handler);

    // Get server PID
    pid_t pid = getpid();
    char pid_str[MAX_BUF_SIZE];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    mqd_t s_queue, d_queue, m_queue;

    // Create server queues
    create_server_queues(pid_str, &s_queue, &d_queue, &m_queue);

    // Read the first message from PID_s queue
    uint8_t s_message;
    mq_receive(s_queue, (char *)&s_message, 1, NULL);

    // Compute result for the first answer
    // For demonstration purposes, we'll just send back the received message as a response
    mq_send(s_queue, (const char *)&s_message, 1, 0);

    // Destroy server queues
    destroy_server_queues(pid_str, s_queue, d_queue, m_queue);

    return EXIT_SUCCESS;
}
