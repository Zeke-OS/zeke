/**
 *******************************************************************************
 * @file    tish_debug.c
 * @author  Olli Vanhoja
 * @brief   Various debug tools for tish/Zeke.
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

static int create_debug_thread(void);
static void * test_thread(void * arg);
static void thread_stat(void);

static char invalid_arg[] = "Invalid argument\n";

static int debug(char * argv[])
{
    char * arg = argv[1];

    if (!arg) {
        goto fail;
    }

    /* Thread debug commands */
    else if (!strcmp(arg, "thread")) {
        arg = argv[2];

        if (arg && !strcmp(arg, "create")) {
            int err = create_debug_thread();
            if (err)
                return EXIT_FAILURE;
        } else {
            fprintf(stderr, "%s", invalid_arg);
        }

        return 0;
    /* Data Abort Commands */
    } else if (!strcmp(arg, "dab")) {
        arg = argv[2];

        if (arg && !strcmp(arg, "fatal")) {
            printf("Trying fatal DAB\n");
            int * x = (void *)0xfffffff;
            *x = 1;
        } else {
            fprintf(stderr, "%s", invalid_arg);
        }

        return 0;
    }

fail:
    fprintf(stderr, "Invalid subcommand\n");
    return EXIT_FAILURE;
}
TISH_CMD(debug, "debug");

static int create_debug_thread(void)
{
    pthread_attr_t attr;
    static pthread_t test_tid;
    char * stack;
    const size_t stack_size = 4096;

    errno = 0;
    stack = malloc(stack_size);
    if (!stack) {
        printf("Failed to create a stack\n");

        errno = ENOMEM;
        return -1;
    }
    printf("New stack @ %p\n", stack);

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, stack_size);

    errno = 0;
    if (pthread_create(&test_tid, &attr, test_thread, 0)) {
        printf("Thread creation failed\n");
        return -1;
    }
    printf("Thread created with id: %u and stack: %p\n", test_tid, stack);

    return 0;
}

static void * test_thread(void * arg)
{
    while (1) {
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
