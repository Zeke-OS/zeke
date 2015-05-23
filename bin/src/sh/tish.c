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

#include <autoconf.h> /* for KERNEL_VERSION */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linenoise.h>
#include "../utils.h"
#include "tish.h"

#define READ  0
#define WRITE 1

/* TODO should be without prefix but FAT doesn't support ot */
#define HISTFILE "tish.histfile"

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";

static const char msg[] = "Zeke " KERNEL_VERSION;

static int n; /*!< number of calls to command(). */
static char * args[512]; /*!< Args for exec. */

SET_DECLARE(tish_cmd, struct tish_builtin);

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

static char * skipwhite(char * s)
{
    while (isspace(*s)) {
        ++s;
    }

    return s;
}

static void split(char * cmd)
{
    cmd = skipwhite(cmd);
    char * next;
    int i = 0;

    next = strchr(cmd, ' ');

    while (next) {
        next[0] = '\0';
        args[i] = cmd;
        ++i;
        cmd = skipwhite(next + 1);
        next = strchr(cmd, ' ');
    }

    if (cmd[0] != '\0') {
        args[i] = cmd;
        next = strchr(cmd, '\n');
        if (next)
            next[0] = '\0';
        ++i;
    }

    args[i] = NULL;
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
    split(cmd);

    if (args[0]) {
        if (strcmp(args[0], "exit") == 0)
            exit(0);
        if (strcmp(args[0], "cd") == 0) {
            cd(args);
            return input;
        }
        n++;

        return command(input, first, last);
    }

    return 0;
}

void init_hist(void)
{
    char * histsize;
    int len;

    histsize = getenv("HISTSIZE");
    if (histsize) {
        len = atoi(histsize);
    } else {
        len = 1000;
    }
    linenoiseHistorySetMaxLen(len);
    linenoiseHistoryLoad(HISTFILE);
}

int main(int argc, char * argv[], char * envp[])
{
    const char prompt[] = "# ";
    char * line;

    printf("args[%d]:", argc);
    for (char ** s = argv; *s; s++) {
        printf(" %s", *s);
    }

    printf("\nenviron:");
    for (char ** s = envp; *s; s++) {
        printf(" %s", *s);
    }
    printf("\n%s%s\n", banner, msg);

    init_hist();

    fflush(NULL);
    while ((line = linenoise(prompt)) != NULL) {
        char * next;
        int input = 0;
        int first = 1;

        linenoiseHistoryAdd(line);
        linenoiseHistorySave(HISTFILE);

        /*
         * Parse line.
         */
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

        cleanup(n);
        n = 0;
        free(line);
        fflush(NULL);
    }

    return 0;
}

static int help(char * argv[])
{
    struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        printf("%s ", (*cmd)->name);
    }

    printf("\n");

    return 0;
}
TISH_CMD(help, "help");
