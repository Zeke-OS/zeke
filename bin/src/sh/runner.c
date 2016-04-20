/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny shell.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../utils.h"
#include "tish.h"

#define READ  0
#define WRITE 1

enum runner_state {
    CMD_FIRST,
    CMD_MIDDLE,
    CMD_LAST,
};

static int fork_count; /*!< number of forks. */
static char * args[256]; /*!< Args for exec. */

static const struct tish_builtin * get_builtin(char * name)
{
    const struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        if (!strcmp(name, (*cmd)->name))
            return *cmd;
    }

    return NULL;
}

/**
 * Handle commands separately.
 * @param input_fd is the return value from the previous call.
 */
static int command(int input_fd, enum runner_state state)
{
    int pipettes[2];
    pid_t pid;
    const struct tish_builtin * builtin = get_builtin(args[0]);

    if (builtin && builtin->flags & TISH_NOFORK) {
        builtin->fn(args); /* TODO Handle return value and piping */
        return input_fd;
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
        if (state == CMD_FIRST && input_fd == STDIN_FILENO) {
            /* First command */
            dup2(pipettes[WRITE], STDOUT_FILENO);
        } else if (state == CMD_MIDDLE && input_fd != STDIN_FILENO) {
            /* Middle command */
            dup2(input_fd, STDIN_FILENO);
            dup2(pipettes[WRITE], STDOUT_FILENO);
        } else {
            /* Last command */
            dup2(input_fd, STDIN_FILENO);
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

    if (input_fd != STDIN_FILENO)
        close(input_fd);

    /* Nothing more needs to be written. */
    close(pipettes[WRITE]);

    /* If it's the last command, nothing more needs to be read. */
    if (state == CMD_LAST)
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
            int sig = WTERMSIG(status);
            char errmsg[80];

            snprintf(errmsg, sizeof(errmsg),
                     "Child %d killed by signal %d (%s)%s",
                     (int)pid, sig, strsignal(sig),
                     (WCOREDUMP(status)) ? " (core dumped)" : NULL);
            psignal(sig, errmsg);
        }
    }
    fork_count = 0;
}

static int exec(char * argv[])
{
    if (!argv[1])
        return EXIT_FAILURE;

    if (execvp(argv[1], argv + 1))
        return EXIT_FAILURE;

    return 0;
}
TISH_CMD(exec, "exec", " <command>", TISH_NOFORK);

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
TISH_CMD(cd, "cd", " <dir>", TISH_NOFORK);

static int tish_exit(char * argv[])
{
    int code = 0;

    if (argv[1])
        code = atoi(argv[1]);

    exit(code);
}
TISH_CMD(tish_exit, "exit", NULL, TISH_NOFORK);

static void line_cleanup(char * line)
{
    char * next;
    char ch;

    next = line;
    while ((ch = *next) != '\0') {
        switch (ch) {
        case '#': /* Cut of from a comment marker. */
            *next = '\0';
            goto out;
            break;
        case '\n': /* Remove line endings. */
            *next = '\0';
            goto out;
        }
        next++;
    }
out:
    return;
}

void run_line(char * line)
{
    char * next;
    int input_fd = STDIN_FILENO;
    enum runner_state state = CMD_FIRST;

    line_cleanup(line);

    /* Skip if nothing to run. */
    if (strlen(line) == 0)
        return;

    next = line;
    do {
        char * cmd = next;

        next = strchr(next, '|');
        if (next)
            *next = '\0';
        else
            state = CMD_LAST;

        split(cmd, args, num_elem(args));
        if (args[0]) {
            input_fd = command(input_fd, state);
        }
        state = CMD_MIDDLE;
    } while (next++);

    cleanup();
    fflush(NULL);
}
