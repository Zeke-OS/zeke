#define _XOPEN_SOURCE 700
#define _BSD_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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
    fd_set rfds;
    struct timeval timeout;
    char input;
    char * line = NULL;
    size_t n, len = 0;

    fp = fopen(file, "r");
    if (!fp) {
        perror("Failed to open commands file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int retval;

        FD_ZERO(&rfds);
        FD_SET(fildes, &rfds);
        timeout = (struct timeval){
            .tv_sec = 5,
            .tv_usec = 0,
        };

        retval = select(fildes + 1, &rfds, NULL, NULL, &timeout);
        if (retval == -1 || retval == 0)
            break;
        if (read(fildes, &input, sizeof(char)) <= 0)
            break;
        write(STDOUT_FILENO, &input, sizeof(char));

        if (input == '#' && (len = getline(&line, &n, fp)) != -1) {
            write(fildes, line, len);
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
    struct timespec timeout;
    sigset_t sigset;
    int sig, err;

    if (argc < 4) {
        printf("Usage: %s FILE TIMEOUT args\n", argv[0]);
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

        if (execvp(argv[3], argv + 3))
            perror("Exec failed");

        exit(EXIT_FAILURE);
    }

    close(pty[1]); /* Close the slave side */
    send_commands(pty[0], argv[1]);

    err = 0;
    timeout = (struct timespec){
        .tv_sec = atoi(argv[2]),
        .tv_nsec = 0,
    };
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);
    sig = sigtimedwait(&sigset, NULL, &timeout);
    if (sig < 0) {
        kill(pid, SIGINT);
        fprintf(stderr, "Timeout after %u sec\n", (unsigned)timeout.tv_sec);
        err = EXIT_FAILURE;
    }
    wait(NULL);
    close(pty[0]);

    return err;
}
