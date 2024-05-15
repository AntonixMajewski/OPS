// Things to remember:

// 1. If inappropriate file descriptors are closed, you are most likely iterating over i, not j.
// 2. Remember to close all of the pipes also after executing your code.
// 3. If you want to iterate over i as long as there is open write end, you can compare read() == 0 (if it's 0, write end closed).
// 4. If using malloc, remember to free these resources at the end with free();

// `Empty matrices

//             read[0]         write[1]
// [0]         XXXXXX          XXXXXX
// [1]         XXXXXX          XXXXXX
// [2]         XXXXXX          XXXXXX
// [3]         XXXXXX          XXXXXX
// [4]         XXXXXX          XXXXXX

// Useful code snippets


// Initializing 2n array


for (int i = 1; i <= 2 * n; i++) {
    cards[2 * i - 2] = i;
    cards[2 * i - 1] = i;
}


// Shuffle n size array


void shuffle(int *array, int size) {
    srand(time(NULL));
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}


// Creating fifo


if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
    if (errno != EEXIST)
        ERR("create fifo");
if ((fifo = open(argv[1], O_RDWR)) < 0)
    ERR("open");
read_from_fifo(fifo);
if (close(fifo) < 0)
    ERR("close fifo:");


// Creating buffer_size string


#define BUFFER_SIZE 16

char message[] = "new_round";

char buffer[BUFFER_SIZE];
memset(buffer, 0, BUFFER_SIZE);
strncpy(buffer, message, strlen(message));


// Example of signal handling


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Signal handler function for SIGINT (Ctrl+C)
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Caught SIGINT (Ctrl+C), exiting...\n");
        exit(EXIT_SUCCESS); // Exit the program gracefully
    }
}

int main() {
    struct sigaction sa;  // Define a struct sigaction to specify the action to be taken on receipt of a signal

    sa.sa_handler = handle_signal;  // Set the signal handler function to handle_signal
    sigemptyset(&sa.sa_mask);  // Initialize the signal set to empty, meaning no signals will be blocked during the execution of the handler
    sa.sa_flags = 0;  // No special flags are set

    // Register the signal handler for SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");  // If sigaction fails, print an error message
        exit(EXIT_FAILURE);  // Exit the program with a failure status
    }

    // Main loop to keep the program running
    while (1) {
        printf("Running... Press Ctrl+C to interrupt.\n");
        sleep(1);  // Sleep for 1 second
    }

    return 0;  // Although this line will never be reached, it's good practice to include a return statement in main
}


// Very generic signal handling


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Error handling macro
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Generic signal handler function
void handle_signal(int sig) {
    switch (sig) {
        case SIGINT:
            // Handle SIGINT (Ctrl+C)
            printf("Caught SIGINT (Ctrl+C), exiting...\n");
            exit(EXIT_SUCCESS);
            break;
        case SIGCHLD:
            // Handle SIGCHLD (child process terminated)
            while (waitpid(-1, NULL, WNOHANG) > 0) {
                // Reap all terminated child processes
            }
            break;
        default:
            printf("Unhandled signal %d\n", sig);
            break;
    }
}

// Function to set up a signal handler
void set_signal_handler(int sig, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction)); // Clear the struct
    sa.sa_handler = handler;                  // Set the handler function
    sigemptyset(&sa.sa_mask);                 // Clear the signal mask (no signals are blocked during the handler)
    sa.sa_flags = 0;                          // No special flags

    // Register the signal handler
    if (sigaction(sig, &sa, NULL) == -1) {
        ERR("sigaction");
    }
}

int main() {
    // Set up signal handlers
    set_signal_handler(SIGINT, handle_signal);
    set_signal_handler(SIGCHLD, handle_signal);

    // Main program loop
    while (1) {
        printf("Running... Press Ctrl+C to interrupt.\n");
        sleep(1); // Simulate work
    }

    return 0;
}


// Kill children signal


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

// Global variable to store the PIDs of the child processes
pid_t *child_pids;
int num_children;

// Signal handler function for SIGINT
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Caught SIGINT (Ctrl+C), terminating children...\n");
        // Send SIGTERM to all child processes
        for (int i = 0; i < num_children; i++) {
            if (child_pids[i] > 0) { // Check if the child process is still running
                if (kill(child_pids[i], SIGTERM) == -1) {
                    ERR("kill");
                }
            }
        }
        // Wait for all child processes to terminate
        for (int i = 0; i < num_children; i++) {
            if (child_pids[i] > 0) {
                waitpid(child_pids[i], NULL, 0);
            }
        }
        exit(EXIT_SUCCESS);
    }
}

// Function to set up a signal handler
void set_signal_handler(int sig, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(sig, &sa, NULL) == -1) {
        ERR("sigaction");
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    num_children = atoi(argv[1]);
    if (num_children <= 0) {
        fprintf(stderr, "Number of children should be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for child PIDs
    child_pids = malloc(num_children * sizeof(pid_t));
    if (child_pids == NULL) {
        ERR("malloc");
    }

    // Set up the SIGINT signal handler
    set_signal_handler(SIGINT, handle_signal);

    // Create child processes
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            ERR("fork");
        } else if (pid == 0) {
            // Child process
            while (1) {
                printf("Child %d (PID %d) is running...\n", i, getpid());
                sleep(1); // Simulate work
            }
            exit(EXIT_SUCCESS); // This line is never reached
        } else {
            // Parent process
            child_pids[i] = pid;
        }
    }

    // Parent process waits for SIGINT
    while (1) {
        pause(); // Wait for signals
    }

    // Clean up
    free(child_pids);
    return EXIT_SUCCESS;
}


// Read from pipe


void read_from_pipe(int fd) {
    char buffer[128];
    ssize_t count = read(fd, buffer, sizeof(buffer) - 1);
    if (count == -1) {
        ERR("read");
    }
    buffer[count] = '\0';
    printf("Read from pipe: %s\n", buffer);
}


// Write to pipe


void write_to_pipe(int fd, const char *message) {
    if (write(fd, message, strlen(message)) == -1) {
        ERR("write");
    }
}


// 