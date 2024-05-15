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

#define BUFFER_SIZE 16

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s n m\n 1 < n < 6 - number of players\n 4 < m < 11 - nubmer of cards", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    if (argc != 3)
        usage(argv[0]);

    int n = atoi(argv[1]);
    if (n < 2 || n > 5)
        usage(argv[0]);

    int m = atoi(argv[2]);
    if (m < 5 || m > 10)
        usage(argv[0]);

    int parent_send[n][2];
    int child_send[n][2];

    int child[n], i, j;

    for (i = 0; i < n; i++)
    {
        if (pipe(parent_send[i]) == -1)
            ERR("pipe");

        if (pipe(child_send[i]) == -1)
            ERR("pipe");
    }

    for (i = 0; i < n; i++)
    {
        child[i] = fork();
        if (child[i] == -1)
            ERR("fork");

        if (child[i] == 0)
        {
            int cards[m];
            
            for (j = 0; j < n; j++)
            {
                close(parent_send[j][1]);
                close(child_send[j][0]);
                if (j != i)
                {
                    close(parent_send[j][0]);
                    close(child_send[j][1]);
                }
            }

            for (j = 0; j < m; j++)
            {
                cards[j] = j + 1;
                // printf("PID %d got card %d\n", getpid(), cards[j]);
            }

            int pid = getpid();
            srand(time(NULL) * pid);

            // Perform action

            for (j = 0; j < m; j++)
            {
                int rand_index = rand() % m;
                int rand_card = cards[rand_index];

                char buffer[BUFFER_SIZE];
                int r = read(parent_send[i][0], buffer, BUFFER_SIZE);
                printf("PID: %d received %s\n", pid, buffer);
                write(child_send[i][1], &rand_card, sizeof(int));
            }

            close(parent_send[i][0]);
            close(child_send[i][1]);

            return 0;
        }
    }

    for (i = 0; i < n; i++)
    {
        close(parent_send[i][0]);
        close(child_send[i][1]);
    }

    // Perform

    char message[] = "new_round";

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    strncpy(buffer, message, strlen(message));

    for (i = 0; i < m; i++)
    {
        printf("NEW ROUND\n");

        for (j = 0; j < n; j++)
        {
            int recv_num;
            // printf("Parent sends: %s\n", buffer);
            write(parent_send[j][1], buffer, BUFFER_SIZE);
            read(child_send[j][0], &recv_num, sizeof(int));
            printf("Received number %d\n", recv_num);
        }
    }


    for (i = 0; i < n; i++)
    {
        wait(NULL);
        close(parent_send[i][1]);
        close(child_send[i][0]);
    }

    return EXIT_SUCCESS;
}

//                 parent_send                                                 child_send

//             read[0]         write[1]                                    read[0]         write[1]
// [0]         child 0         parent                                      parent          child 0
// [1]         child 1         parent                                      parent          child 1
// [2]         child 2         parent                                      parent          child 2
// [3]         child 3         parent                                      parent          child 3