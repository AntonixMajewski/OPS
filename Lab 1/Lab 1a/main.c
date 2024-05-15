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
    fprintf(stderr, "USAGE: %s n m\n n >= 1 - number of players\n m >= 100 - money", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int n, m, i, j;
    int* pids;
    if (argc != 3)
        usage(argv[0]);

    n = atoi(argv[1]);
    if (n < 1)
        usage(argv[0]);

    m = atoi(argv[2]);
    if (m < 100)
        usage(argv[0]);

    pids = (int *)malloc(sizeof(int) * n);

    int croupier_write[n][2];
    int player_write[n][2];

    for (i = 0; i < n; i++)
    {
        if (pipe(player_write[i]) == -1)
            ERR("pipe");

        if(pipe(croupier_write[i]) == -1)
            ERR("pipe");
    }

    for (i = 0; i < n; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
            ERR("fork");
        
        if (pids[i] == 0)
        {
            for (j = 0; j < n; j++)
            {
                close(player_write[j][0]);
                close(croupier_write[j][1]);
                if (j != i)
                {
                    close(player_write[j][1]);
                    close(croupier_write[j][0]);
                }
            }

            int pid = getpid();
            srand(time(NULL) * pid);
            
            printf("%d: I got %d money and I'm going to play roulette\n", pid, m);

            while (m > 0)
            {
                int data_to_send[3];
                int bet = rand() % m + 1;
                m -= bet;
                int number = rand() % 37;
                int lucky_number;

                data_to_send[0] = pid;
                data_to_send[1] = bet;
                data_to_send[2] = number;

                if (write(player_write[i][1], data_to_send, sizeof(data_to_send)) == -1)
                    ERR("write");
                if (read(croupier_write[i][0], &lucky_number, sizeof(int)) == -1)
                    ERR("read");

                if (lucky_number == number)
                {
                    m = m + bet * 35;
                    printf("%d: Whoa, I won %d!\n", pid, bet*35);
                }
            }

            printf("%d: I'm broke\n", pid);
            close(player_write[i][1]);
            close(croupier_write[i][0]);

            return 0;
        }
    }

    for (j = 0; j < n; j++)
    {
        close(player_write[i][1]);
        close(croupier_write[i][0]);
    }

    srand(time(NULL) * getpid());

    int lucky_number = rand() % 37;

    while (1)
    {
        for (i = 0; i < n; i++)
        {
            int bet[3];
            if (read(player_write[i][0], bet, sizeof(bet)) == -1)
                ERR("read");
            printf("Croupier: %d placed %d on a %d\n", bet[0], bet[1], bet[2]);
            if (write(croupier_write[i][1], &lucky_number, sizeof(int)) == -1)
                ERR("write");
        }

        printf("Croupier: %d is a lucky number\n", lucky_number);
    }
    
    for (i = 0; i < n; i++)
    {
        wait(NULL);
    }

    return EXIT_SUCCESS;
}