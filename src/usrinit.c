/**
 *******************************************************************************
 * @file    usrinit.c
 * @author  Olli Vanhoja
 * @brief   First user scope process.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/_kservices.h>
#include <syscall.h>
#include <kerror.h> /* Strictly kernel space thing */
#include <kstring.h> /* -"- */
#include <sys/sysctl.h>
#include "usrinit.h"

dev_t dev_tty0 = DEV_MMTODEV(2, 0);

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";

static void test_thread(void *);
static void print_message(const char * message);
static int usr_name2oid(char * name, int * oidp);
static void thread_stat(void);

/**
 * main thread; main process.
 */
void * main(void * arg)
{
    int mib_tot[10];
    int mib_free[10];
    int len;
    int old_value_tot, old_value_free;
    int old_len = sizeof(old_value_tot);
    char buf[80];
    pthread_attr_t attr = {
    };
    pthread_t thread_id;

    //pthread_create(&thread_id, &attr, test_thread, 0);

    KERROR(KERROR_DEBUG, "Init v0.0.1");
    len = usr_name2oid("vm.dynmem_tot", mib_tot);
    usr_name2oid("vm.dynmem_free", mib_free);

    while(1) {
        //bcm2835_uputc('T');
        thread_stat();
        if(sysctl(mib_tot, len, &old_value_tot, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            KERROR(KERROR_ERR, buf);
        }
        if(sysctl(mib_free, len, &old_value_free, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            KERROR(KERROR_ERR, buf);
        }
        ksprintf(buf, sizeof(buf), "dynmem used: %u/%u",
                old_value_tot - old_value_free, old_value_tot);
        KERROR(KERROR_LOG, buf);
        osDelay(5000);
    }
}

static void test_thread(void * arg)
{
    while(1) {
        /* TODO Nicely any call seems to cause data abort :) */
        osDelay(2000);
        thread_stat();
    }
}

static void print_message(const char * message)
{
    size_t i = 0;

    while (message[i] != '\0') {
        //osDevCwrite(message[i++], dev_tty0);
    }
}

static int usr_name2oid(char * name, int * oidp)
{
    int oid[2];
    int i;
    size_t j;

    oid[0] = 0;
    oid[1] = 3;

    j = CTL_MAXNAME * sizeof(int);
    i = sysctl(oid, 2, oidp, &j, name, strlenn(name, 80));
    if (i < 0)
        return (i);
    j /= sizeof(int);
    return (j);
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
    KERROR(KERROR_LOG, buf);
}
