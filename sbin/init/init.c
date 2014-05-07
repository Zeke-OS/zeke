/**
 *******************************************************************************
 * @file    init.c
 * @author  Olli Vanhoja
 * @brief   First user scope process.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <kernel.h>
#include <syscall.h>
#include <kstring.h> /* -"- */
#include <sys/sysctl.h>
#include <unistd.h>
#include "init.h"

dev_t dev_tty0 = DEV_MMTODEV(2, 0);

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";

static void * test_thread(void *);
static void print_message(const char * message);
static void thread_stat(void);

static char main_stack2[8192];

void * main(void * arg)
{
    int mib_tot[3];
    int mib_free[3];
    int len;
    int old_value_tot, old_value_free;
    size_t old_len = sizeof(old_value_tot);
    char buf[80];

    pthread_attr_t attr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = main_stack2,
        .stackSize  = sizeof(main_stack2)
    };
    pthread_t thread_id;

    print_message("Init v0.0.1");

#if 0
    pid_t pid = fork();
    if (pid == -1) {
        print_message("Failed");
        while(1);
    } else if (pid == 0) {
        print_message("Hello");
        while(1);
    } else {
        print_message("original");
    }
#endif

    pthread_create(&thread_id, &attr, test_thread, 0);

    len = sysctlnametomib("vm.dynmem_tot", mib_tot, 3);
    sysctlnametomib("vm.dynmem_free", mib_free, 3);

    while(1) {
        thread_stat();
        if(sysctl(mib_tot, len, &old_value_tot, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            print_message(buf);
        }
        if(sysctl(mib_free, len, &old_value_free, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            print_message(buf);
        }
        ksprintf(buf, sizeof(buf), "dynmem allocated: %u/%u",
                old_value_tot - old_value_free, old_value_tot);
        print_message(buf);
        sleep(5);
    }
}

static void * test_thread(void * arg)
{
    while(1) {
        sleep(10);
        thread_stat();
    }
}

static void print_message(const char * message)
{
    write(2, message, strlenn(message, 80));
}

static void thread_stat(void)
{
    /* Print thread id & cpu mode */
    int id;
    uint32_t mode;
    char buf[80];

    id = (int)syscall(SYSCALL_SCHED_THREAD_GETTID, NULL);
    __asm__ volatile ("mrs     %0, cpsr" : "=r" (mode));
    ksprintf(buf, sizeof(buf), "My id: %u, my mode: %x", id, mode);
    print_message(buf);
}
