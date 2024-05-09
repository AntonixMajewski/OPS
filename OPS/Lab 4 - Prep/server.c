#include "common.h"

#define BACKLOG 3
#define MAX_EVENTS 16

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig) { do_work = 0; }

void usage(char *name) { fprintf(stderr, "USAGE: %s port\n", name); }

int max_sum = 0;

void calculate(int16_t data, int16_t *sum)
{
    int16_t pid;

    printf("Data: %d\n", data);

    pid = data;
    char pid_str[20];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);

    *sum = 0;

    for (int i = 0; pid_str[i] != '\0'; ++i) {
        *sum += pid_str[i] - '0';
    }

    data = *sum;

    printf("PID: %d\nSUM: %d\n", pid, *sum);

    if (*sum > max_sum)
    {
        max_sum = *sum;
    }
}

void doServer(int tcp_listen_socket)
{
    int epoll_descriptor;
    if ((epoll_descriptor = epoll_create1(0)) < 0)
    {
        ERR("epoll_create:");
    }
    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = tcp_listen_socket;
    if (epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, tcp_listen_socket, &event) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int nfds;
    int16_t data;
    ssize_t size;
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (do_work)
    {
        if ((nfds = epoll_pwait(epoll_descriptor, events, MAX_EVENTS, -1, &oldmask)) > 0)
        {
            for (int n = 0; n < nfds; n++)
            {
                int client_socket = add_new_client(events[n].data.fd);
                if ((size = bulk_read(client_socket, (char *)&data, sizeof(int16_t))) < 0)
                    ERR("read:");
                if (size == (int)sizeof(int16_t))
                {
                    int16_t sum = 0;
                    calculate(data, &sum);
                    if (bulk_write(client_socket, (char *)&sum, sizeof(int16_t)) < 0 && errno != EPIPE)
                        ERR("write:");
                }
                if (TEMP_FAILURE_RETRY(close(client_socket)) < 0)
                    ERR("close");
            }
        }
        else
        {
            if (errno == EINTR)
                continue;
            ERR("epoll_pwait");
        }
    }
    if (TEMP_FAILURE_RETRY(close(epoll_descriptor)) < 0)
        ERR("close");
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char **argv)
{
    int tcp_listen_socket;
    int new_flags;

    if (argc != 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Seting SIGPIPE:");
    if (sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");

    tcp_listen_socket = bind_tcp_socket(atoi(argv[1]), BACKLOG);
    new_flags = fcntl(tcp_listen_socket, F_GETFL) | O_NONBLOCK;
    fcntl(tcp_listen_socket, F_SETFL, new_flags);

    doServer(tcp_listen_socket);
    if (TEMP_FAILURE_RETRY(close(tcp_listen_socket)) < 0)
        ERR("close");
    
    fprintf(stderr, "HIGH SUM: %d\n", max_sum);
    return EXIT_SUCCESS;
}