/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
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

#define KERNEL_INTERNAL
#include <stddef.h>
#include <autoconf.h>
#include <kstring.h>
#include <kerror.h>
#include <errno.h>
#include <sched.h>
#include <hal/core.h>

volatile uint32_t flag_kernel_tick = 0;

void init_stack_frame(struct _ds_pthread_create * thread_def,
        sw_stack_frame_t * sframe, int priv)
{
    /* Note that scheduler has to have the same mapping. */
    uint32_t stack_start = ((uint32_t)(thread_def->def->stackAddr)
            + thread_def->def->stackSize
            - sizeof(errno_t));

    sframe->r0  = (uint32_t)(thread_def->argument);
    sframe->r1  = 0;
    sframe->r2  = 0;
    sframe->r3  = 0;
    sframe->r12 = 0;
    sframe->sp  = stack_start;
    sframe->pc  = ((uint32_t)(thread_def->start)) + 4;
    sframe->lr  = (uint32_t)(thread_def->del_thread);
    sframe->psr = priv ? SYSTEM_PSR : USER_PSR;
}

void svc_setretval(intptr_t retval)
{
    current_thread->sframe[SCHED_SFRAME_SVC].r0 = retval;
}

istate_t get_interrupt_state(void)
{
    istate_t state;

    __asm__ volatile (
        "MRS %[reg], cpsr\n\t"
        "AND %[reg], %[reg], %[mask]"
        : [reg]"=r" (state)
        : [mask]"I" (PSR_INT_MASK));

    return state;
}

void set_interrupt_state(istate_t state)
{
    __asm__ volatile (
        "MRS r1, cpsr\n\t"
        "BIC r1, r1, %[mask]\n\t"
        "ORR r1, r1, %[ostate]\n\t"
        "MSR cpsr, r1"
        :
        : [mask]"I" (PSR_INT_MASK), [ostate]"r" (state)
        : "r1");
}

int test_lock(int * lock)
{
    int value;

    istate_t s_entry = get_interrupt_state();
    disable_interrupt();

    __asm__ volatile (
        "LDREX      %[val], [%[addr]]"
        : [val]"+r" (value)
        : [addr]"r" (lock));

    set_interrupt_state(s_entry);

    return value;
}

int test_and_set(int * lock)
{
    int err = 2; /* Initial value of error meaning already locked */

    istate_t s_entry = get_interrupt_state();
    disable_interrupt();

    /* We can use a lot simpler code for non-MP targets and infact the MP
     * version doesn't always work that well on non-MP hardware. */
#if configMP == 0
    err = *lock != 0 ? 2 : 0;
    *lock = 1;
#else
    __asm__ volatile (
        "MOV        r1, #1\n\t"             /* locked value to r1 */
        "try%=:\n\t"
        "LDREX      r2, [%[addr]]\n\t"      /* load value of the lock */
        "CMP        r2, #1\n\t"             /* if already set */
        "STREXNE    %[res], r1, [%[addr]]\n\t" /* Sets err = 0
                                                * if store op ok */
        "CMPEQ      %[res], #0\n\t"         /* Try again if strex failed */
        "BNE        try%="
        : [res]"+r" (err)
        : [addr]"r" (lock)
        : "r1", "r2"
    );
#endif

    set_interrupt_state(s_entry);

    return err;
}

/**
 * Invalidate all caches.
 */
void cpu_invalidate_caches(void)
{
    const uint32_t rd = 0; /* Cache operation. */

    __asm__ volatile (
        "MCR     p15, 0, %[rd], c7, c7, 0\n\t"  /* Invalidate I+D caches. */
        "MCR     p15, 0, %[rd], c8, c7, 0\n\t"  /* Invalidate all I+D TLBs. */
        "MCR     p15, 0, %[rd], c7, c10, 4\n\t" /* Drain write buffer. */
        : : [rd]"r" (rd)
    );
}

/**
 * Set Context ID.
 * @param cid new Context ID.
 */
void cpu_set_cid(uint32_t cid)
{
    const int rd = 0;
    uint32_t curr_cid;

    __asm__ volatile (
        "MRC    p15, 0, %[cid], c13, c0, 1" /* Read CID */
         : [cid]"=r" (curr_cid)
    );

    if (curr_cid != cid) {
        __asm__ volatile (
            "MCR    p15, 0, %[rd], c7, c10, 4\n\t"  /* DSB */
            "MCR    p15, 0, %[cid], c13, c0, 1\n\t" /* Set CID */
            "MCR    p15, 0, %[rd], c7, c5, 0\n\t"   /* Flush I cache & BTAC */
            : : [rd]"r" (rd), [cid]"r" (cid)
        );
    }
}

void stack_dump(sw_stack_frame_t frame)
{
    const char sdump[] = {
        "psr = %x\n"
        "r0  = %x\n"
        "r1  = %x\n"
        "r2  = %x\n"
        "r3  = %x\n"
        "r4  = %x\n"
        "r5  = %x\n"
        "r6  = %x\n"
        "r7  = %x\n"
        "r8  = %x\n"
        "r9  = %x\n"
        "r10 = %x\n"
        "r11 = %x\n"
        "r12 = %x\n"
        "sp  = %x\n"
        "lr  = %x\n"
        "pc  = %x"
    };
    char buf[sizeof(sdump) + 17 * 10];

    ksprintf(buf, sizeof(buf), sdump,
            frame.psr, frame.r0, frame.r1, frame.r2, frame.r3, frame.r4,
            frame.r5, frame.r6, frame.r7, frame.r8, frame.r9, frame.r10,
            frame.r11, frame.r12, frame.sp, frame.lr, frame.pc);
    KERROR(KERROR_ERR, buf);
}
