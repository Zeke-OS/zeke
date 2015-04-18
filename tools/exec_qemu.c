#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void send_commands(int dest_fildes, char * file)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;

    fp = fopen(file, "r");
    if (!fp) {
        perror("Failed to open commands file");
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &len, fp) != -1) {
        if (strcmp(line, "exit")) {
            line[len - 1] = '\n';
        }
        write(dest_fildes, line, len);
    }

    free(line);
    fclose(fp);
}

int main(int argc, char * argv[])
{
    int pip[2];
    pid_t pid;
    int err, sig;
    struct timespec timeout;
    sigset_t sigset;

    if (argc < 4) {
        printf("Usage: %s FILE TIMEOUT args\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    err = pipe(pip);
    if (err) {
        perror("Failed to create a pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(STDIN_FILENO);
        dup2(pip[0], STDIN_FILENO);

        err = execvp(argv[3], argv + 3);
        if (err)
            perror("Exec failed");

        exit(EXIT_FAILURE);
    }

    close(pip[0]);
    send_commands(pip[1], argv[1]);

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
        err = EXIT_FAILURE;
    }
    wait(NULL);
    close(pip[1]);

    return err;
}
