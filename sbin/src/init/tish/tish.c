/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <string.h>
#include <sys/types.h>
#include <zeke.h>
#include <errno.h>
#include <unistd.h>
#include <syscall.h>
#include "tish.h"

SET_DECLARE(tish_cmd, struct tish_builtin);

int tish_eof;

/* Static functions */
static char * gline(char * str, int num);

int tish(void)
{
    char * cmd_name;
    char line[MAX_LEN];
    char * strtok_lasts;
    int err;

    while (1) {
        printf("# ");
        if (!gline(line, MAX_LEN))
            break;

        if ((cmd_name = strtok_r(line, DELIMS, &strtok_lasts))) {
            struct tish_builtin ** cmd;
            errno = 0;

            SET_FOREACH(cmd, tish_cmd) {
                if(!strcmp(cmd_name, (*cmd)->name)) {
                    (*cmd)->fn(&strtok_lasts);
                    goto get_errno;
                }
            }

            printf("I don't know how to execute\n");

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

static void uptime(char ** args)
{
    uint32_t loads[3];

    syscall(SYSCALL_SCHED_GET_LOADAVG, loads);

    printf("load average: %u, %u, %u\n", loads[0], loads[1], loads[2]);
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

/* TODO Until we have some actual standard IO subsystem working the following
 * is the best hack I can think of. */
#define _ugetc() bcm2835_uart_ugetc()

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
        if (ch == 127) {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
            continue;
        }

        /* TODO Handle arrow keys and delete */

        /* Handle return */
        if (ch == '\n' || ch == '\r' || i == num) {
            str[i] = '\0';
            buf[0] = '\n';
            printf("%s", buf);
            return str;
        } else {
            str[i] = ch;
        }

        buf[0] = ch;
        printf("%s", buf);
        i++;
    }
}
