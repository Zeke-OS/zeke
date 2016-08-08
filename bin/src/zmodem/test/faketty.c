#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int fd = open(argv[1], O_RDWR);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDIN_FILENO);
    execv(argv[2], argv + 2);
    perror("Failed to exec");
    return 1;
}
