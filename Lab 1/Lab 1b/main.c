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
    fprintf(stderr, "USAGE: %s n, n < 5\n", name);
    exit(EXIT_FAILURE);
}

void shuffle(int *array, int size) {
    srand(time(NULL));
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

int main(int argc, char** argv)
{
    int n, child_players, i, j;

    if (argc != 2)
        usage(argv[0]);

    n = atoi(argv[1]);

    // Cards initialization

    int cards[4 * n + 1];
    srand(time(NULL) * getpid());
    int rand_index = rand() % (2 * n);
    int ctr_amount = 1;

    for (int i = 1; i <= 2 * n; i++) {
        cards[2 * i - 2] = i;
        cards[2 * i - 1] = i;
    }

    cards[4 * n] = 0;

    shuffle(cards, 4 * n + 1);

    // Program

    int fds[n][2];

    if (n > 5 || n < 2)
        usage(argv[0]);

    child_players = n - 1;

    for (i = 0; i < child_players; i++)
    {
        //int player_cards[2];
        int child = fork();
        if (child == -1)
            ERR("fork");

        if (child == 0)
        {
            int player_cards[5];
            for (j = 0; j < 4; j ++)
            {
                player_cards[j] = cards[i + child_players * j];
            }
            printf("Hello from player %d - my cards are %d %d %d %d\n", getpid(), player_cards[0], player_cards[1], player_cards[2], player_cards[3]);
            return 0;
        }
    }

    printf("Hello from player %d\n", getpid());

    for (i = 0; i < 4 * n + 1; i++)
    {
        printf("Card %d is %d\n", i, cards[i]);
    }

    return EXIT_SUCCESS;
}


//                      read[0]             write[1]         read[0]         write[1]
// child[0]             chil1               parent             
// child[1]             chil2               chil1
// child[2]             chil3               chil2
// child[3]             parent              chil3