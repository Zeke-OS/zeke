/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

__user void * init_stack_frame(struct _sched_pthread_create_args * thread_def,
                               thread_stack_frames_t * tsf, int priv)
{
    sw_stack_frame_t * sframe = &tsf->s[SCHED_SFRAME_SYS];
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
void init_stack_frame_on_fork(struct thread_info * new_thread,
                              struct thread_info * old_thread)
{
    sw_stack_frame_t * sframe = &new_thread->sframe.s[SCHED_SFRAME_SYS];

    /*
     * We wan't to return directly to the user space.
     */
    memcpy(sframe, &old_thread->sframe.s[SCHED_SFRAME_SVC],
           sizeof(sw_stack_frame_t));

    sframe->r0  = 0; /* retval of fork(). */
    sframe->pc += 4; /* ctx switch will substract 4 from the PC. */
}

static inline int is_psr_mode(uint32_t psr, uint32_t mode)
{
    return (psr & mode) == mode;
}

sw_stack_frame_t * get_usr_sframe(struct thread_info * thread)
{
    sw_stack_frame_t * tsf = thread->sframe.s;

    /*
     * We hope one of these stack frames can be reliably recognized as
     * the stack frame returning to the user mode, otherwise we are pretty
     * much screwed.
     */
    if (thread_flags_is_set(thread, SCHED_INSYS_FLAG) &&
        is_psr_mode(tsf[SCHED_SFRAME_SVC].psr, PSR_MODE_USER)) {
        return &tsf[SCHED_SFRAME_SVC];
    } else if (thread_flags_is_set(thread, SCHED_INABO_FLAG) &&
               is_psr_mode(tsf[SCHED_SFRAME_SVC].psr, PSR_MODE_USER)) {
        return &tsf[SCHED_SFRAME_ABO];
    } else if (is_psr_mode(tsf[SCHED_SFRAME_SYS].psr, PSR_MODE_USER)) {
        return &tsf[SCHED_SFRAME_SYS];
    }

    return NULL;
}

/**
 * Set the stack frame of the current thread to a privileged register.
 * This is an optimization that makes fetching the stack frame address
 * of the current thread a little bit faster.
 */
void arm11_set_current_thread_stackframe(void)
{
    sw_stack_frame_t * sfarr;

    sfarr = (!current_thread) ? NULL : current_thread->sframe.s;

    __asm__ volatile (
        "MCR    p15, 0, %[sfarr], c13, c0, 4"
        : : [sfarr]"r" (sfarr)
    );
}

/**
 * Get a specific stack frame of the current thread.
 * @param ind is the stack frame index.
 * @return  Returns an address to the stack frame of the current thread;
 *          Or NULL if current_thread is not set.
 */
void * arm11_get_current_thread_stackframe(size_t ind)
{
    sw_stack_frame_t * sfarr;

    __asm__ volatile (
        "MRC    p15, 0, %[sfarr], c13, c0, 4"
        : [sfarr]"=r" (sfarr)
    );

    return (sfarr) ? &sfarr[ind] : NULL;
}

void svc_getargs(uint32_t * type, uintptr_t * p)
{
    sw_stack_frame_t * sframe = &current_thread->sframe.s[SCHED_SFRAME_SVC];

    *type = sframe->r0;
    *p = sframe->r1;
}

void svc_setretval(intptr_t retval)
{
    current_thread->sframe.s[SCHED_SFRAME_SVC].r0 = retval;
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

/**
 * Set Context ID.
 * Should be only called from ARM11 specific interrupt handlers.
 * @param cid new Context ID.
 */
void arm11_set_cid(uint32_t cid)
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
        : [tls]"=r" (tls));

    return tls;
}

void core_set_tls_addr(__user struct _sched_tls_desc * tls)
{
    /* Write User Read Only Thread and Proc. ID Register */
    __asm__ volatile (
        "MCR    p15, 0, %[tls], c13, c0, 3"
        : : [tls]"r" (tls));
}

static void fork_init_tls(struct thread_info * th)
{
    th->tls_regs = (struct tls_regs){
#ifdef configUSE_HFP
        .fpscr = 0,
        .fpexc = 0,
        .fpinst = 0xEE000A00,
        .fpinst2 = 0,
#endif
    };

#ifdef configUSE_HFP
#define INIT_VFP_REG(i) do {        \
    __asm__ volatile (              \
        "FMDLR  d" #i ", %[r]\n\t"  \
        "FMDHR  d" #i ", %[r]"      \
        : : [r]"r" (0));            \
} while (0)

    INIT_VFP_REG(0);
    INIT_VFP_REG(1);
    INIT_VFP_REG(2);
    INIT_VFP_REG(3);
    INIT_VFP_REG(4);
    INIT_VFP_REG(5);
    INIT_VFP_REG(6);
    INIT_VFP_REG(7);
    INIT_VFP_REG(8);
    INIT_VFP_REG(9);
    INIT_VFP_REG(10);
    INIT_VFP_REG(11);
    INIT_VFP_REG(12);
    INIT_VFP_REG(13);
    INIT_VFP_REG(14);
    INIT_VFP_REG(15);
#undef INIT_VFP_REG
#endif
}
DATA_SET(thread_fork_handlers, fork_init_tls);

/*
 * HW TLS in this context means anything that needs to be thread local and
 * is stored in any of the ARM11 hardware registers like floating-point
 * registers and process id registers etc. that are not used in the kernel
 * but are needed by user space processes.
 */
static void arm11_sched_push_hw_tls(void)
{
    current_thread->tls_regs.utls = core_get_user_tls();

#ifdef configUSE_HFP
    __asm__ volatile (
        "FMRX   %[fpscr], FPSCR\n\t"
        "FMRX   %[fpexc], FPEXC\n\t"
        "FMRX   %[fpinst], FPINST\n\t"
        "FMRX   %[fpinst2], FPINST2"
        : : [fpscr]"r" (current_thread->tls_regs.fpscr),
            [fpexc]"r" (current_thread->tls_regs.fpexc),
            [fpinst]"r" (current_thread->tls_regs.fpinst),
            [fpinst2]"r" (current_thread->tls_regs.fpinst2));

#define SAVE_VFP_REG(i) do {                                \
    __asm__ volatile (                                      \
        "FMRRD  %[l], %[h], d" #i                           \
        : : [l]"r" (current_thread->tls_regs.dreg[i]),      \
            [h]"r" (current_thread->tls_regs.dreg[i + 1])); \
} while (0)

    SAVE_VFP_REG(0);
    SAVE_VFP_REG(1);
    SAVE_VFP_REG(2);
    SAVE_VFP_REG(3);
    SAVE_VFP_REG(4);
    SAVE_VFP_REG(5);
    SAVE_VFP_REG(6);
    SAVE_VFP_REG(7);
    SAVE_VFP_REG(8);
    SAVE_VFP_REG(9);
    SAVE_VFP_REG(10);
    SAVE_VFP_REG(11);
    SAVE_VFP_REG(12);
    SAVE_VFP_REG(13);
    SAVE_VFP_REG(14);
    SAVE_VFP_REG(15);
#undef SAVE_VFP_REG
#endif
}
DATA_SET(pre_sched_tasks, arm11_sched_push_hw_tls);

static void arm11_sched_pop_hw_tls(void)
{
    core_set_user_tls(current_thread->tls_regs.utls);
    core_set_tls_addr(current_thread->tls_uaddr);

#ifdef configUSE_HFP
    __asm__ volatile (
        "FMXR   FPSCR, %[fpscr]\n\t"
        "FMXR   FPEXC, %[fpexc]\n\t"
        "FMXR   FPINST, %[fpinst]\n\t"
        "FMXR   FPINST2, %[fpinst2]" :
        [fpscr]"=r" (current_thread->tls_regs.fpscr),
        [fpexc]"=r" (current_thread->tls_regs.fpexc),
        [fpinst]"=r" (current_thread->tls_regs.fpinst),
        [fpinst2]"=r" (current_thread->tls_regs.fpinst2));

#define LOAD_VFP_REG(i) do {                                \
    __asm__ volatile (                                      \
        "FMDRR d" #i ", %[l], %[h]"                         \
        : [l]"=r" (current_thread->tls_regs.dreg[i]),       \
          [h]"=r" (current_thread->tls_regs.dreg[i + 1]));  \
} while (0)

    LOAD_VFP_REG(0);
    LOAD_VFP_REG(1);
    LOAD_VFP_REG(2);
    LOAD_VFP_REG(3);
    LOAD_VFP_REG(4);
    LOAD_VFP_REG(5);
    LOAD_VFP_REG(6);
    LOAD_VFP_REG(7);
    LOAD_VFP_REG(8);
    LOAD_VFP_REG(9);
    LOAD_VFP_REG(10);
    LOAD_VFP_REG(11);
    LOAD_VFP_REG(12);
    LOAD_VFP_REG(13);
    LOAD_VFP_REG(14);
    LOAD_VFP_REG(15);
#undef LOAD_VFP_REG
#endif
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
