/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <stddef.h>
#include <sys/linker_set.h>
#include <kstring.h>
#include <kerror.h>
#include <errno.h>
#include <thread.h>
#include <hal/core.h>

volatile uint32_t flag_kernel_tick = 0;

__user void * init_stack_frame(struct _sched_pthread_create_args * thread_def,
                               sw_stack_frame_t * sframe, int priv)
{
    /* Note that the scheduler must have the same mapping. */
    uint32_t stack_start = ((uint32_t)(thread_def->stack_addr)
            + thread_def->stack_size
            - sizeof(struct _sched_tls_desc));

    sframe->r0  = (uint32_t)(thread_def->arg1);
    sframe->r1  = (uint32_t)(thread_def->arg2);
    sframe->r2  = (uint32_t)(thread_def->arg3);
    sframe->r3  = (uint32_t)(thread_def->arg4);
    sframe->r12 = 0;
    sframe->sp  = stack_start;
    sframe->pc  = ((uint32_t)(thread_def->start)) + 4;
    sframe->lr  = (uint32_t)(thread_def->del_thread);
    sframe->psr = priv ? SYSTEM_PSR : USER_PSR;

    /*
     * The user space address of thread local storage is at the end of
     * the thread stack area.
     */
    return (__user void *)stack_start;
}

/**
 * Fix sys stack frame on fork.
 */
static void fork_init_stack_frame(struct thread_info * th)
{
    th->sframe[SCHED_SFRAME_SYS].r0  = 0; /* retval of fork() */
    th->sframe[SCHED_SFRAME_SYS].pc += 4;
}
DATA_SET(thread_fork_handlers, fork_init_stack_frame);

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

/**
 * Invalidate all caches.
 */
void cpu_invalidate_caches(void)
{
    const uint32_t rd = 0; /* Cache operation. */

    __asm__ volatile (
        "MCR    p15, 0, %[rd], c7, c10, 0\n\t" /* Clean D cache. */
        "MCR    p15, 0, %[rd], c7, c10, 4\n\t" /* DSB. */
        "MCR    p15, 0, %[rd], c7, c7, 0\n\t"  /* Invalidate I+D caches. */
        "MCR    p15, 0, %[rd], c8, c7, 0\n\t"  /* Invalidate all I+D TLBs. */
        "MCR    p15, 0, %[rd], c7, c10, 4\n\t" /* DSB. */
#if 0
        "MCR    p15, 0, %[rd], c7, c5, 4"      /* Prefetch flush. */
#endif
        : : [rd]"r" (rd)
    );
}

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

uint32_t core_get_user_tls(void)
{
    uint32_t value;

    /* Read User Read/Write Thread and Proc. ID Register */
    __asm__ volatile (
        "MRC    p15, 0, %[value], c13, c0, 2"
        : [value]"=r" (value));

    return value;
}

void core_set_user_tls(uint32_t value)
{
    /* Write User Read/Write Thread and Proc. ID Register */
    __asm__ volatile (
        "MCR    p15, 0, %[value], c13, c0, 2"
        : : [value]"r" (value));
}

__user struct _sched_tls_desc * core_get_tls_addr(void)
{
    __user struct _sched_tls_desc * tls;

    /* Read User Read Only Thread and Proc. ID Register */
    __asm__ volatile (
        "MRC    p15, 0, %[tls], c13, c0, 3"
        :  [tls]"=r" (tls));

    return tls;
}

void core_set_tls_addr(__user struct _sched_tls_desc * tls)
{
    /* Write User Read Only Thread and Proc. ID Register */
    __asm__ volatile (
        "MCR    p15, 0, %[tls], c13, c0, 3"
        : : [tls]"r" (tls));
}

/*
 * HW TLS in this context means anything that needs to be thread local and
 * is stored in any of the ARM11 hardware registers like floating-point
 * registers and process id registers etc. that are not used in the kernel
 * but are needed by user space processes.
 */
static void arm11_sched_push_hw_tls(void)
{
    current_thread->tls_regs.utls = core_get_user_tls();
}
DATA_SET(pre_sched_tasks, arm11_sched_push_hw_tls);

static void arm11_sched_pop_hw_tls(void)
{
    core_set_user_tls(current_thread->tls_regs.utls);
    core_set_tls_addr(current_thread->tls_uaddr);
}
DATA_SET(post_sched_tasks, arm11_sched_pop_hw_tls);

static char stack_dump_buf[400];
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
        "pc  = %x\n"
    };

    ksprintf(stack_dump_buf, sizeof(stack_dump_buf), sdump,
             frame.psr, frame.r0, frame.r1, frame.r2, frame.r3, frame.r4,
             frame.r5, frame.r6, frame.r7, frame.r8, frame.r9, frame.r10,
             frame.r11, frame.r12, frame.sp, frame.lr, frame.pc);
    kputs(stack_dump_buf);
}
