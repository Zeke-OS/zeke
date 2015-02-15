/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/types.h>
#include <sys/wait.h>
#include <zeke.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>
#include <syscall.h>
#include "tish.h"

SET_DECLARE(tish_cmd, struct tish_builtin);

int tish_eof;

/* Static functions */
static void forkexec(char * path, char ** args);
static char * gline(char * str, int num);

int tish(void)
{
    static char line[MAX_LEN];
    char * cmd_name;
    char * strtok_lasts;
    int err;

    while (1) {
        write(STDOUT_FILENO, "# ", 3);
        if (!gline(line, MAX_LEN))
            break;

        if ((cmd_name = strtok_r(line, DELIMS, &strtok_lasts))) {
            struct tish_builtin ** cmd;
            errno = 0;

            SET_FOREACH(cmd, tish_cmd) {
                if (!strcmp(cmd_name, (*cmd)->name)) {
                    (*cmd)->fn(&strtok_lasts);
                    goto get_errno;
                }
            }

            /* Assume fork & exec */
            forkexec(cmd_name, &strtok_lasts);

get_errno:
            if ((err = errno)) {
                printf("\nFailed, errno: %u\n", err);
            }

            if (tish_eof)
                return 0;
        }
    }

    return 0;
}

static void fmt_lavg(char * dst, double load)
{
    int x, y;

    x = (int)(load);
    y = (int)(load * 100.0) % 100;

    sprintf(dst, "%d.%.2d", x, y);
}

static void uptime(char ** args)
{
    double loads[3];
    char l1[5], l2[5], l3[5];
    getloadavg(loads, 3);

    fmt_lavg(l1, loads[0]);
    fmt_lavg(l2, loads[1]);
    fmt_lavg(l3, loads[2]);

    printf("load average: %s, %s, %s\n", l1, l2, l3);
}
TISH_CMD(uptime, "uptime");

static void reg(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);
    char buf[40] = "Invalid argument\n";

    if (!strcmp(arg, "sp")) {
        void * sp;

        __asm__ ("mov %[result], sp" : [result] "=r" (sp));
        snprintf(buf, sizeof(buf), "sp = %p\n", sp);
    } else if (!strcmp(arg, "cpsr")) {
        uint32_t mode;

        __asm__ ("mrs     %0, cpsr" : "=r" (mode));
        snprintf(buf, sizeof(buf), "cpsr = %x\n", mode);
    }

    printf("%s", buf);
}
TISH_CMD(reg, "reg");

static void tish_exit(char ** args)
{
    tish_eof = 1;
}
TISH_CMD(tish_exit, "exit");

static void help(char ** args)
{
    struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        printf("%s ", (*cmd)->name);
    }

    printf("\n");
}
TISH_CMD(help, "help");

static size_t parse_exec_args(char ** argv, char ** args, size_t arg_max)
{
    char * arg;
    char * an = NULL;
    int quote = 0;
    size_t i = 1;

    while ((arg = strtok_r(0, DELIMS, args)) || an) {
        const size_t len = strlen(arg) + 1;

        if (arg && (arg[0] == '\'' || arg[0] == '\"')) {
            arg++;
            quote ^= 1;
            if (*arg == '\0') {
                quote = 0;
                arg = NULL;
            }
        }

        if (quote) {
            size_t an_len;
            char * ann;

            if (!arg)
                break;

            ann = realloc(an, strlen(arg) + len);

            if (!ann) {
                perror("Error while parsing args");
                return 0;
            }
            if (!an) {
                ann[0] = '\0';
            } else {
                strcat(ann, " ");
            }
            an = ann;

            strcat(an, arg);

            an_len = strlen(an);
            if (an[an_len - 1] == '\'' || an[an_len - 1] == '\"') {
                an[an_len - 1] = '\0';
                quote = 0;
            }
        } else {
            if (an)
                argv[i++] = an;
            if (i == arg_max)
                break;

            if (!arg)
                break;

            an = malloc(strlen(arg) + 1);
            memcpy(an, arg, len);
            argv[i++] = an;
            an = NULL;
        }

        if (i == arg_max)
            break;
    }
    argv[i] = NULL;

    return i;
}

#define NARG_MAX 256
static void forkexec(char * path, char ** args)
{
    static char * argv[NARG_MAX];
    size_t argc;
    pid_t pid;
    const char failmsg[] = "Fork failed\n";

    argv[0] = path;
    argc = parse_exec_args(argv, args, num_elem(argv) - 1);
    if (argc == 0) {
        fprintf(stderr, "%s", failmsg);
        return;
    }

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "%s", failmsg);
    } else if (pid == 0) {
        int err;
        err = execvp(path, argv);

        if (err)
            perror("Exec failed");

        _exit(1); /* TODO Figure out why exit(1) fails */
    } else {
        int status;

        wait(&status);

        for (size_t i = 1; i < argc; i++) {
            free(argv[i]);
        }

        printf("status: %u\n", status);
    }
}

static char * gline(char * str, int num)
{
    int err, i = 0;
    char ch;
    char buf[2] = {'\0', '\0'};

    while (1) {
        err = read(STDIN_FILENO, &ch, sizeof(ch));
        if (err <= 0)
            continue;

        /* Handle backspace */
        if (ch == '\b') {
            if (i > 0) {
                i--;
                write(STDOUT_FILENO, "\b \b", 4);
            }
            continue;
        }

        /* TODO Handle arrow keys and delete */

        /* Handle return */
        if (ch == '\n' || ch == '\r' || i == num) {
            str[i] = '\0';
            buf[0] = '\n';
            write(STDOUT_FILENO, buf, sizeof(buf));

            return str;
        } else {
            str[i] = ch;
        }

        buf[0] = ch;
        write(STDOUT_FILENO, buf, sizeof(buf));
        i++;
    }
}
