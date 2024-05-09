#include "common.h"

void usage(char *name) { fprintf(stderr, "USAGE: %s domain port\n", name); }

void prepare_request(int16_t data)
{
    data = htons(getpid());
}

void print_answer(int16_t data)
{
    if (htons(data))
        printf("SUM = %d\n", data);
    else
        printf("Operation impossible\n");
}

int main(int argc, char **argv)
{
    int fd;
    int16_t data = getpid();

    printf("PID: %d\n", data);

    if (argc != 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    fd = connect_tcp_socket(argv[1], argv[2]);
    prepare_request(data);
    if (bulk_write(fd, (char *)&data, sizeof(int32_t)) < 0)
        ERR("write:");
    if (bulk_read(fd, (char *)&data, sizeof(int16_t)) < (int)sizeof(int16_t))
        ERR("read:");
    print_answer(data);
    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}
