#define _GNU_SOURCE
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t children_left = 0;

void sethandler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
    // Declare a structure to hold information about the signal handler
    struct sigaction act;
    // Clear the memory of the structure to ensure all members are initialized to zero
    memset(&act, 0, sizeof(struct sigaction));

    // Set the signal handler function in the structure
    act.sa_sigaction = f;
    // Indicate that the signal handler expects additional information
    act.sa_flags = SA_SIGINFO;

    // Call sigaction to set the signal handler for the specified signal
    if (-1 == sigaction(sigNo, &act, NULL))
        // If sigaction fails, report the error using the ERR macro
        ERR("sigaction");
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    mqd_t *pin; // Declare a pointer to hold the message queue descriptor
    uint8_t ni; // Declare a variable to store the received message
    unsigned msg_prio; // Declare a variable to store the priority of the received message

    // Extract the message queue descriptor from the siginfo structure
    pin = (mqd_t *)info->si_value.sival_ptr;

    // Set up a notification to be delivered when a message arrives in the queue
    static struct sigevent not;
    not.sigev_notify = SIGEV_SIGNAL;
    not.sigev_signo = SIGRTMIN;
    not.sigev_value.sival_ptr = pin;
    // Register the notification using mq_notify
    if (mq_notify(*pin, &not) < 0)
        ERR("mq_notify");

    // Infinite loop to continuously receive messages from the queue
    for (;;)
    {
        // Receive a message from the queue
        if (mq_receive(*pin, (char *)&ni, 1, &msg_prio) < 1)
        {
            // If mq_receive fails, check if it's due to EAGAIN (no message available)
            if (errno == EAGAIN)
                break; // Break the loop if no message is available
            else
                ERR("mq_receive"); // Report an error for other mq_receive failures
        }
        // Check the priority of the received message
        if (0 == msg_prio)
            printf("MQ: got timeout from %d.\n", ni); // Print a message if it's a timeout message
        else
            printf("MQ:%d is a bingo number!\n", ni); // Print a message if it's a bingo number
    }
}

void child_work(int n, mqd_t pin, mqd_t pout)
// This function takes three arguments:

//     n: An integer representing the process number or identifier.
//     pin: The message queue descriptor for receiving messages from children.
//     pout: The message queue descriptor for sending messages to children.
{
    int life; // Declare a variable to represent the lifespan of the child process
    uint8_t ni; // Declare a variable to store the received number from the parent process
    uint8_t my_bingo; // Declare a variable to store the randomly chosen bingo number for the child process

    // Seed the random number generator with the process ID
    srand(getpid());

    // Generate a random lifespan for the child process
    life = rand() % LIFE_SPAN + 1;

    // Generate a random bingo number for the child process
    my_bingo = (uint8_t)(rand() % MAX_NUM);

    // Loop until the child process reaches the end of its lifespan
    while (life--)
    {
        // Receive a number from the parent process through the message queue
        if (TEMP_FAILURE_RETRY(mq_receive(pout, (char *)&ni, 1, NULL)) < 1)
            ERR("mq_receive");
        
        // Print the received number along with the process ID
        printf("[%d] Received %d\n", getpid(), ni);

        // Check if the received number matches the child's bingo number
        if (my_bingo == ni)
        {
            // If a match is found, send the bingo number to the parent process through the message queue
            if (TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&my_bingo, 1, 1)))
                ERR("mq_send");
            
            // Terminate the child process after sending the bingo number
            return;
        }
    }

    // If the child process reaches the end of its lifespan without finding a bingo number,
    // send the process number to the parent process through the message queue
    if (TEMP_FAILURE_RETRY(mq_send(pin, (const char *)&n, 1, 0)))
        ERR("mq_send");
}

void sigchld_handler(int sig, siginfo_t *s, void *p)
// This function serves as the signal handler for the SIGCHLD signal, which is 
// sent by the operating system to the parent process when a child process 
// terminates.
{
    pid_t pid; // Declare a variable to store the process ID of terminated child processes

    // Infinite loop to continuously wait for child process termination
    for (;;)
    {
        // Call waitpid to check if any child processes have terminated
        pid = waitpid(0, NULL, WNOHANG);

        // If waitpid returns 0, it means there are no terminated child processes to wait for
        if (pid == 0)
            return;

        // If waitpid returns a negative value, it indicates an error
        if (pid <= 0)
        {
            // If there are no child processes to wait for (ECHILD error), return from the function
            if (errno == ECHILD)
                return;
            
            // Otherwise, report an error using the ERR macro
            ERR("waitpid");
        }

        // Decrement the count of remaining child processes
        children_left--;
    }
}

void parent_work(mqd_t pout)
// This function takes a single argument:
//     pout: The message queue descriptor for sending messages to children.
{
    // Seed the random number generator with the process ID
    srand(getpid());

    uint8_t ni; // Declare a variable to store the randomly generated number

    // Loop until all child processes have terminated
    while (children_left)
    {
        // Generate a random number within the range [0, MAX_NUM)
        ni = (uint8_t)(rand() % MAX_NUM);

        // Send the generated number to the children through the message queue
        if (TEMP_FAILURE_RETRY(mq_send(pout, (const char *)&ni, 1, 0)))
            ERR("mq_send");

        // Sleep for 1 second before sending the next number
        sleep(1);
    }

    // Print a termination message once all child processes have terminated
    printf("[PARENT] Terminates \n");
}

void create_children(int n, mqd_t pin, mqd_t pout)

    // This function takes three arguments:
    //     n: An integer representing the number of child processes to create.
    //     pin: The message queue descriptor for receiving messages from children.
    //     pout: The message queue descriptor for sending messages to children.

{
    // Loop to create the specified number of child processes
    while (n-- > 0)
    {
        // Fork a new process
        switch (fork())
        {
            // In the child process (fork returns 0)
            case 0:
                // Call the child_work function to perform the work of the child process
                child_work(n, pin, pout);
                // Exit the child process with success status
                exit(EXIT_SUCCESS);
            // If fork fails (returns -1), report an error
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
        // Increment the count of remaining child processes
        children_left++;
    }
}

void usage(void)
{
    // Print usage instructions to the standard error stream (stderr)
    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr, "100 >n > 0 - number of children\n");
    // Exit the program with a failure status
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)

    // This is the entry point of the program, which takes command-line arguments argc 
    // (the number of arguments) and argv (an array of strings representing the arguments).

{
    int n; // Declare a variable to store the number of children

    // Check if the number of command-line arguments is incorrect
    if (argc != 2)
        // If so, call the usage function to display usage instructions and exit the program
        usage();

    // Convert the argument representing the number of children to an integer
    n = atoi(argv[1]);

    // Check if the number of children is out of range
    if (n <= 0 || n >= 100)
        // If so, call the usage function to display usage instructions and exit the program
        usage();

    mqd_t pin, pout; // Declare message queue descriptors
    struct mq_attr attr; // Declare a structure to hold message queue attributes

    // Set the maximum number of messages and the maximum size of each message in the attribute structure
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;

    // Open the message queues for communication between parent and child processes
    if ((pin = TEMP_FAILURE_RETRY(mq_open("/bingo_in", O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open in");
    if ((pout = TEMP_FAILURE_RETRY(mq_open("/bingo_out", O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1)
        ERR("mq open out");

        // Set signal handlers for SIGCHLD and SIGRTMIN (for message queue notification)
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(mq_handler, SIGRTMIN);

    // Create the specified number of child processes
    create_children(n, pin, pout);

    // Set up message queue notification for the input message queue
    static struct sigevent noti;
    noti.sigev_notify = SIGEV_SIGNAL;
    noti.sigev_signo = SIGRTMIN;
    noti.sigev_value.sival_ptr = &pin;
    if (mq_notify(pin, &noti) < 0)
        ERR("mq_notify");

    // Perform the work of the parent process
    parent_work(pout);

    // Close the message queues
    mq_close(pin);
    mq_close(pout);

    // Unlink (remove) the message queues
    if (mq_unlink("/bingo_in"))
        ERR("mq unlink");
    if (mq_unlink("/bingo_out"))
        ERR("mq unlink");

    // Exit the program with success status
    return EXIT_SUCCESS;
}