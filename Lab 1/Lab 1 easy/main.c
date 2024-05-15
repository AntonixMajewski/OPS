#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n, n \n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int n = 2, i, j;

    int fds[n + 1][2];

    for (i = 0; i < n + 1; i++)
    {
        if (pipe(fds[i]) == -1)
            ERR("pipe");
    }

    for (i = 0; i < n; i++)
    {
        int child = fork();
        if (child == -1)
            ERR("fork");

        if (child == 0)
        {
            for (j = 0; j < n; j++)
            {
                if (j != i) // Remember to always put j !!!!!
                {
                    close(fds[j][0]);
                    close(fds[j + 1][1]);
                }
            }

            int pid = getpid();
            srand(time(NULL) * pid);
            int gen_int_num = rand() % 100;
            int recv_int_num;

            if (read(fds[i][0], &recv_int_num, sizeof(int)) == -1)
                ERR("read");
            if (write(fds[i + 1][1], &gen_int_num, sizeof(int)) == -1)
                ERR("write");
            printf("%d received number %d\n", pid, recv_int_num);

            close(fds[i][0]);
            close(fds[i + 1][1]);

            return 0;
        }
    }

    for (j = 0; j < n + 1; j++)
    {
        if (j != 0)
        {
            close(fds[j][1]);
        }
        if (j != n)
        {
            close(fds[j][0]);
        }
    }

    int pid = getpid();
    srand(time(NULL) * pid);
    int gen_int_num = rand() % 100;
    int recv_int_num;

    if (write(fds[0][1], &gen_int_num, sizeof(int)) == -1)
        ERR("write");
    if (read(fds[n][0], &recv_int_num, sizeof(int)) == -1)
        ERR("read");
    printf("%d received number %d\n", pid, recv_int_num);

    for (i = 0; i < n; i++)
    {
        wait(NULL);
    }

    close(fds[0][1]);
    close(fds[n][0]);

    return EXIT_SUCCESS;
}

//             read[0]         write[1]
// [0]         ch1             parent
// [1]         ch2             ch1
// [2]         parent          ch2