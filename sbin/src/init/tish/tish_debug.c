/**
 *******************************************************************************
 * @file    tish_debug.c
 * @author  Olli Vanhoja
 * @brief   Various debug tools for tish/Zeke.
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> /* file test */
#include <time.h> /* file test */
#include <sys/stat.h> /* file test */
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <zeke.h>
#include "tish.h"

static void create_debug_thread(void);
static void * test_thread(void * arg);
static void thread_stat(void);

static char invalid_arg[] = "Invalid argument\n";

static void debug(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);

    /* Thread debug commands */
    if (!strcmp(arg, "thread")) {
        arg = strtok_r(0, DELIMS, args);
        if (!strcmp(arg, "create")) {
            create_debug_thread();
        } else {
            printf("%s", invalid_arg);
        }
    /* Process debug commands */
    } else if (!strcmp(arg, "proc")) {
        arg = strtok_r(0, DELIMS, args);
        if (!strcmp(arg, "fork")) {
            pid_t pid = fork();
            if (pid == -1) {
                printf("fork() failed\n");
            } else if (pid == 0) {
                printf("Hello from the child process\n");
                for (int i = 0; i < 10; i++) {
                    printf(".");
                    msleep(500);
                }
                exit(0);
            } else {
                int status;

                printf("original\n");
                wait(&status);
                printf("status: %u\n", status);
            }
        } else {
            puts(invalid_arg);
        }
    /* Data Abort Commands */
    } else if (!strcmp(arg, "dab")) {
        arg = strtok_r(0, DELIMS, args);
        if (!strcmp(arg, "fatal")) {
            printf("Trying fatal DAB\n");
            int * x = (void *)0xfffffff;
            *x = 1;
        } else {
            printf("%s", invalid_arg);
        }
    } else if (!strcmp(arg, "ioctl")) {
        arg = strtok_r(0, DELIMS, args);
        if (!strcmp(arg, "termios")) {
            struct termios term;
            int err;

            err = tcgetattr(STDOUT_FILENO, &term);
            if (err)
                return;

            printf("cflags: %u\nispeed: %u\nospeed: %u\n",
                   term.c_cflag, term.c_ispeed, term.c_ospeed);
        } else {
            printf("%s", invalid_arg);
        }
    } else if (!strcmp(arg, "file")) {
        char buf[80];
        const char text[] = "This is a test.\n";
        int fildes;

        fildes = open("file", O_RDWR | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fildes < 0)
            return;

        write(fildes, text, sizeof(text));
        lseek(fildes, 0, SEEK_SET);
        read(fildes, buf, sizeof(buf));
        close(fildes);

        printf("%s", buf);
    } else {
        printf("Invalid subcommand\n");
        errno = EINVAL;
    }
}
TISH_CMD(debug, "debug");

static void create_debug_thread(void)
{
    static pthread_t test_tid;
    char * newstack;

    errno = 0;
    if ((newstack = sbrk(1024)) == (void *)-1) {
        printf("Failed to create a stack\n");
        return;
    }
    printf("New stack @ %p\n", newstack);

    pthread_attr_t attr = {
        .tpriority  = 0,
        .stackAddr  = newstack,
        .stackSize  = 1024
    };

    errno = 0;
    if (pthread_create(&test_tid, &attr, test_thread, 0)) {
        printf("Thread creation failed\n");
        return;
    }
    printf("Thread created with id: %u and stack: %p\n", test_tid, newstack);
}

static void * test_thread(void * arg)
{
    while(1) {
        sleep(1);
        thread_stat();
    }
}

static void thread_stat(void)
{
    /* Print thread id & cpu mode */
    uint32_t mode, sp;
    pthread_t id = pthread_self();

    __asm__ volatile (
        "mrs     %0, cpsr\n\t"
        "mov     %1, sp"
        : "=r" (mode), "=r" (sp));
    printf("My id: %u, sp: %x, my mode: %x\n", id, sp, mode);
}
