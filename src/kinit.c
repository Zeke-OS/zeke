/**
 *******************************************************************************
 * @file    kernel_init.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kerror.h>
#include <usrinit.h>
#include <sched.h>
#include <libkern.h>
#include <kinit.h>

extern void (*__hw_preinit_array_start[]) (void) __attribute__((weak));
extern void (*__hw_preinit_array_end[]) (void) __attribute__((weak));

extern void (*__hw_postinit_array_start[]) (void) __attribute__((weak));
extern void (*__hw_postinit_array_end[]) (void) __attribute__((weak));

extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));

extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

static char main_stack[configUSRINIT_SSIZE];

static void exec_array(void (*a []) (void), int n);

/**
 * Run all kernel module initializers.
 */
void exec_init_array(void)
{
    int n;

    KERROR(KERROR_LOG, "ZeKe PreInit");

    n = __hw_preinit_array_end - __hw_preinit_array_start;
    exec_array(__hw_preinit_array_start, n);

    n  = __init_array_end - __init_array_start;
    exec_array(__init_array_start, n);

    n = __hw_postinit_array_end - __hw_postinit_array_start;
    exec_array(__hw_postinit_array_start, n);
}

/**
 * Run all kernel module finalizers.
 */
void exec_fini_array(void)
{
    int n = __fini_array_end - __fini_array_start;
    exec_array(__fini_array_start, n);
}

/**
 * Initialize main thread.
 */
void kinit(void)
{
    SUBSYS_INIT();

    KERROR(KERROR_LOG, "Starting init");
    /* Create app_main thread */
    pthread_attr_t main_thread = {
        .tpriority = configUSRINIT_PRI,
        .stackAddr = main_stack,
        .stackSize = sizeof(main_stack)
    };
    pthread_t tid;
    pthread_create(&tid, &main_thread, &main, NULL);
}

static void random_init(void) __attribute__((constructor));
static void random_init(void)
{
    /* TODO Add some randomness ;) */
    KERROR(KERROR_LOG, "Seed random number generator");
    ksrandom(1);
}

/**
 * Exec intializer/finalizer array created by the linker.
 * @param pointer to the array.
 * @param n number of entries.
 */
static void exec_array(void (*a []) (void), int n)
{
    int i;

    for (i = 0; i < n; i++) {
        a[i]();
    }
}

