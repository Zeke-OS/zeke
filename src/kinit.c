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
#include <app_main.h>
#include "sched.h"

extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));
extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

static char main_Stack[configAPP_MAIN_SSIZE];

/**
 * Run all kernel module initializers.
 */
void exec_init_array(void)
{
    int n, i;

    n = __init_array_end - __init_array_start;
    for (i = 0; i < n; i++) {
        __init_array_start[i] ();
    }
}

/**
 * Initialize main thread.
 */
int kinit(void)
{
    /* Create app_main thread */
    osThreadDef_t main_thread = {
        .pthread   = (os_pthread)(&main),
        .tpriority = configAPP_MAIN_PRI,
        .stackAddr = main_Stack,
        .stackSize = sizeof(main_Stack)/sizeof(char)
    };
    osThreadCreate(&main_thread, NULL);
}


/**
 * Run all kernel module finalizers.
 */
void exec_fini_array(void)
{
    int n, i;

    n = __fini_array_end - __fini_array_start;
    for (i = 0; i < n; i++) {
        __fini_array_start[i] ();
    }
}

