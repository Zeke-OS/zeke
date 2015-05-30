/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny shell.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../utils.h"
#include "tish.h"

#define READ  0
#define WRITE 1

static int fork_count; /*!< number of forks. */
static char * args[512]; /*!< Args for exec. */

static struct tish_builtin * get_builtin(char * name)
{
    struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        if (!strcmp(name, (*cmd)->name))
            return *cmd;
    }

    return NULL;
}

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
    struct tish_builtin * builtin = get_builtin(args[0]);

    if (builtin && builtin->flags & TISH_NOFORK) {
        builtin->fn(args); /* TODO Handle return value */
        return input;
    }

    /*
     *  STDIN --> O --> O --> O --> STDOUT
     */

    fork_count++;
    pipe(pipettes);
    pid = fork();
    if (pid == -1) {
        perror("Fork failed");
    } else if (pid == 0) {
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

        /* Run builtin command */
        if (builtin) {
            _exit(builtin->fn(args));
        }

        /* Try exec */
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
static void cleanup(void)
{
    for (size_t i = 0; i < fork_count; ++i) {
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
    fork_count = 0;
}

static int cd(char * argv[])
{
    char * arg = (argv[1]) ? argv[1] : getenv("HOME");

    if (!arg) {
        fprintf(stderr, "cd: missing argument.\n");
    }
    if (chdir(arg)) {
        fprintf(stderr, "cd: no such file or directory: %s\n", arg);
    }

    return 0;
}
TISH_NOFORK_CMD(cd, "cd");

static int tish_exit(char * argv[])
{
    int code = 0;

    if (argv[1])
        code = atoi(argv[1]);

    exit(code);
}
TISH_NOFORK_CMD(tish_exit, "exit");

static int run(char * cmd, int input, int first, int last)
{
    split(cmd, args, num_elem(args));

    if (!args[0])
        return 0;

    if (args[0][0] == '#') {
        return input;
    }

    return command(input, first, last);
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

    cleanup();
    fflush(NULL);
}
