#define _XOPEN_SOURCE 700
#define _BSD_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

int pty_timeout;

int getpty(int pty[2])
{
    int fdm, fds;

    fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
        return -1;

    if (grantpt(fdm))
        return -1;

    if (unlockpt(fdm))
        return -1;

    fds = open(ptsname(fdm), O_RDWR);

    pty[0] = fdm;
    pty[1] = fds;

    return 0;
}

void send_commands(int fildes, char * file)
{
    FILE * fp;
    char * line = NULL;

    fp = fopen(file, "r");
    if (!fp) {
        perror("Failed to open commands file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char input;
        size_t n, len;
        fd_set rfds;
        struct timeval timeout = { .tv_sec = 5, .tv_usec = 0, };
        int retval;

        FD_ZERO(&rfds);
        FD_SET(fildes, &rfds);

        retval = select(fildes + 1, &rfds, NULL, NULL, &timeout);
        if (retval == -1 || retval == 0) {
            pty_timeout = 1;
            break;
        }
        if (read(fildes, &input, sizeof(char)) <= 0)
            break;
        write(STDOUT_FILENO, &input, sizeof(char));

        if (input == '#' && (len = getline(&line, &n, fp)) != -1) {
            line[len - 1] = '\r';
            write(fildes, line, len);
            if (!strcmp(line, "exit\n"))
                break;
        } else if (len == -1) {
            break;
        }
    }

    free(line);
    fclose(fp);
}

int main(int argc, char * argv[])
{
    int pty[2];
    pid_t pid;

    if (argc < 3) {
        printf("Usage: %s FILE args\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (getpty(pty)) {
        fprintf(stderr, "Failed to get pty\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        struct termios new_term;

        close(pty[0]); /* Close the master side */
        cfmakeraw(&new_term);
        tcsetattr(pty[1], TCSANOW, &new_term);
        close(0);
        close(1);
        dup(pty[1]);
        dup(pty[1]);

        if (execvp(argv[2], argv + 2))
            perror("Exec failed");

        exit(EXIT_FAILURE);
    }

    close(pty[1]); /* Close the slave side */
    send_commands(pty[0], argv[1]);
    kill(pid, SIGINT);
    wait(NULL);
    close(pty[0]);

    return (pty_timeout) ? EXIT_FAILURE : 0;
}
