#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../utils.h"
#include "tish.h"

#define READ  0
#define WRITE 1

static int run_count; /*!< number of calls to command(). */
static char * args[512]; /*!< Args for exec. */

/**
 * Handle commands separately.
 * @param input is the return value from previous command.
 * @param first should be set 1 if first command in pipe-sequence.
 * @param last should be set 1 if last command in pipe-sequence.
 */
static int command(int input, int first, int last)
{
    int pipettes[2];
    pid_t pid;

    /*
     *  STDIN --> O --> O --> O --> STDOUT
     */

    pipe(pipettes);
    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
    } else if (pid == 0) {
        struct tish_builtin ** cmd;
        int err;

        if (first == 1 && last == 0 && input == 0) {
            /* First command */
            dup2(pipettes[WRITE], STDOUT_FILENO);
        } else if (first == 0 && last == 0 && input != 0) {
            /* Middle command */
            dup2(input, STDIN_FILENO);
            dup2(pipettes[WRITE], STDOUT_FILENO);
        } else {
            /* Last command */
            dup2(input, STDIN_FILENO);
        }

        /* Try builtin command. */
        SET_FOREACH(cmd, tish_cmd) {
            if (!strcmp(args[0], (*cmd)->name)) {
                err = (*cmd)->fn(args);
                _exit(err);
            }
        }

        /* Try exec. */
        if (execvp(args[0], args)) {
            _exit(EXIT_FAILURE);
        }
    }

    if (input != 0)
        close(input);

    /* Nothing more needs to be written. */
    close(pipettes[WRITE]);

    /* If it's the last command, nothing more needs to be read. */
    if (last == 1)
        close(pipettes[READ]);
    return pipettes[READ];
}

/**
 * Final cleanup, wait() for processes to terminate.
 * @param n is the number of times command() was invoked.
 */
static void cleanup(int n)
{
    for (size_t i = 0; i < n; ++i) {
        pid_t pid;
        int status;
        int exit_status;

        pid = wait(&status);
        exit_status = WEXITSTATUS(status);

        if (WIFEXITED(status) && exit_status != 0) {
            fprintf(stderr, "Child %d ret: %d\n", (int)pid, exit_status);
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child %d killed by signal %d\n", (int)pid,
                    WTERMSIG(status));
        }
    }
}

static void cd(char * argv[])
{
    char * arg = (argv[1]) ? argv[1] : getenv("HOME");

    if (!arg) {
        fprintf(stderr, "cd: missing argument.\n");
    }
    if (chdir(arg)) {
        fprintf(stderr, "cd: no such file or directory: %s\n", arg);
    }
}

static int run(char * cmd, int input, int first, int last)
{
    split(cmd, args, num_elem(args));

    if (args[0]) {
        if (strcmp(args[0], "exit") == 0)
            exit(0);
        if (strcmp(args[0], "cd") == 0) {
            cd(args);
            return input;
        }
        run_count++;

        return command(input, first, last);
    }

    return 0;
}

void run_line(char * line)
{
    char * next;
    int input = 0;
    int first = 1;

    next = line;
    do {
        char * cmd = next;
        int last = 0;

        next = strchr(next, '|');
        if (next)
            *next = '\0';
        else
            last = 1;

        input = run(cmd, input, first, last);

        first = 0;
    } while (next++);

    cleanup(run_count);
    run_count = 0;
    fflush(NULL);
}
