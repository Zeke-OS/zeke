/**
 *******************************************************************************
 * @file arm1176jzf_s_interrupt.c
 * @author Olli Vanhoja
 * @brief Interrupt service routines.
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

/** @addtogroup HAL
* @{
*/

/** @addtogroup ARM1176JZF_S
* @{
*/

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <sched.h>
#include <syscall.h>
#include "../hal_core.h"
#include "arm11_interrupt.h"

void interrupt_init_module(void) __attribute__((constructor));
static inline void run_scheduler(void);

void interrupt_init_module(void)
{
    /* TODO */
    return 0; /* OK */
}

static inline void run_scheduler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        sched_handler();
        load_context();

        /* Return to the usr mode thread */
        __asm__ volatile ("movs, pc, lr\n");
    }
}

/**
* This function handles NMI exception.
*/
void NMI_Handler(void)
{
    /* @todo fixme */
}

/**
* This function handles SVCall exception.
*/
#pragma optimize = no_cse
void SVC_Handler(void)
{
    uint32_t type;
    void * p;

    __asm__ volatile(
        "MOV %[typ], r2\n" /* Read parameters from r2 & r3 */
        "MOV %[arg], r3\n"
        : [typ]"=r" (type), [arg]"=r" (p)
        :
        : "r2", "r3"
    );

    /* Call kernel's internal syscall handler */
    uint32_t result = _intSyscall_handler(type, p);

    /* Return value is stored to r4 */
    __asm__ volatile(
        "MOV r4, %0\n"
        :
        : "r" (result)
        : "r4"
    );
}

/**
* This function handles PendSVC exception.
*/
void PendSV_Handler(void)
{
    run_scheduler();
}

/**
* This function handles SysTick Handler.
*/
void SysTick_Handler(void)
{
    run_scheduler();
}

/**
* @}
*/

/**
* @}
*/
