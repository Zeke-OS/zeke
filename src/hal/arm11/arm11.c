/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
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

/** @addtogroup ARM11
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <stddef.h>
#include <sched.h>
#include "../hal_mcu.h"
#include "arm11.h"

uint32_t flag_kernel_tick = 0;


void init_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread)
{
    sw_stack_frame_t * thread_frame;

    /* Pointer to the thread stack frame */
    thread_frame = (sw_stack_frame_t *)((uint32_t)(thread_def->stackAddr)
                    + thread_def->stackSize - sizeof(sw_stack_frame_t));

    thread_frame->r0 = (uint32_t)(argument);
    thread_frame->r1 = 0;
    thread_frame->r2 = 0;
    thread_frame->r3 = 0;
    thread_frame->r12 = 0;
    thread_frame->pc = ((uint32_t)(thread_def->pthread));
    thread_frame->lr = a_del_thread;
    thread_frame->psr = DEFAULT_PSR;
}

uint32_t syscall(uint32_t type, void * p)
{
    uint32_t scratch;

    __asm__ volatile (
            /* Lets expect that parameters are already in r0 & r1 */
            "SVC    #0\n\t"
            "MOV    %[res], r0\n\t"
            : [res]"=r" (scratch)
            : [typ]"r" (type), [arg]"r" (p)
            : "r2", "r3", "r4");

    return scratch;
}

int test_and_set(int * lock) {
    int err = 2; /* Initial value of error meaning already locked */

    __asm__ volatile (
            "MOV      r1, #1\n"         /* locked value to r1 */
            "LDREX    r2, [%[addr]]\n"  /* load value of lock */
            "CMP      r2, #1\n"         /* if already set */
            "STREXNE  %[res], r1, [%[addr]]\n" /* Sets err = 0 if store op ok */
            : [res]"+r" (err)   /* + makes LLVM think that err is also read in
                                 * the inline asm. Otherwise it would expand
                                 * previous line to:  strexne r2,r1,r2 */
            : [addr]"r" (lock)
            : "r1", "r2"
    );

    return err;
}

/* HardFault Handling *********************************************************/

void HardFault_Handler(void)
{
    /* TODO */

    while (1) { }

    /* Kill the current thread */
    //sched_thread_terminate(current_thread->id);

    /* Return to the scheduler ASAP */
    //req_context_switch();
}

/* End of Fault Handling ******************************************************/

/**
  * @}
  */

/**
  * @}
  */
