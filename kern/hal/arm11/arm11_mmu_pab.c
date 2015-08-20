/**
 *******************************************************************************
 * @file    arm11_mmu_pab.c
 * @author  Olli Vanhoja
 * @brief   MMU control functions for ARM11 ARMv6 instruction set.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <ksignal.h>
#include <proc.h>
#include <hal/core.h>
#include <hal/mmu.h>

static pab_handler * const prefetch_aborts[];

static const char * pab_fsr_strerr[] = {
    "",
    "Alignment",
    "Instruction debug event",
    "Section AP fault",
    "", /* Not function */
    "Section translation",
    "Page AP fault",
    "Page translation",
    "Precise external abort",
    "Domain section fault",
    "",
    "Domain page fault",
    "Extrenal first-level abort",
    "Section permission fault",
    "External second-evel abort",
    "Page permission fault"
};

static int pab_fatal(uint32_t ifsr, uint32_t ifar, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread);
static int pab_buserr(uint32_t ifsr, uint32_t ifar, uint32_t psr, uint32_t lr,
                      struct proc_info * proc, struct thread_info * thread);

const char * get_pab_strerror(uint32_t ifsr)
{
    return pab_fsr_strerr[ifsr & FSR_STATUS_MASK];
}

static void pab_dump(uint32_t ifsr, uint32_t ifar, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread)
{
    KERROR(KERROR_CRIT,
           "Fatal PAB:\n"
           "pc: %x\n"
           "ifsr: %x (%s)\n"
           "ifar: %x\n"
           "proc info:\n"
           "pid: %i\n"
           "tid: %i\n"
           "insys: %i\n",
           lr,
           ifsr, get_pab_strerror(ifsr),
           ifar,
           (int32_t)((proc) ? proc->pid : -1),
           (int32_t)thread->id,
           (int32_t)thread_flags_is_set(thread, SCHED_INSYS_FLAG));
    stack_dump(current_thread->sframe[SCHED_SFRAME_ABO]);
}

/**
 * Prefetch abort handler.
 */
void mmu_prefetch_abort_handler(void)
{
    uint32_t ifsr; /*!< Fault status */
    uint32_t ifar; /*!< Fault address */
    const uint32_t spsr = current_thread->sframe[SCHED_SFRAME_ABO].psr;
    const uint32_t lr = current_thread->sframe[SCHED_SFRAME_ABO].pc;
#if 0
    const istate_t s_old = spsr & PSR_INT_MASK; /*!< Old interrupt state */
#endif
    istate_t s_entry; /*!< Int state in handler entry. */
    struct thread_info * const thread = (struct thread_info *)current_thread;
    struct proc_info * proc;
    pab_handler * handler;

    /* Get fault status */
    __asm__ volatile (
        "MRC p15, 0, %[reg], c5, c0, 1"
        : [reg]"=r" (ifsr));
    /*
     * Get fault address
     * TODO IFAR is not updated if FSR == 2 (debug abourt)
     */
    __asm__ volatile (
        "MRC p15, 0, %[reg], c6, c0, 2"
        : [reg]"=r" (ifar));

    if (!thread) {
        panic("Thread not set on PAB");
    }

    /* TODO Handle DAB in preemptible state. */
    /* Handle this data abort in pre-emptible state if possible. */
    if (ABO_WAS_USERMODE(spsr)) {
        s_entry = get_interrupt_state();
#if 0
        set_interrupt_state(s_old);
#endif
    }

    /*
     * TODO If the abort came from user space and it was BKPT then it was for
     *      a debugger.
     */

    /* RFE Might be enough to get curproc. */
    proc = proc_get_struct_l(thread->pid_owner); /* Can be NULL */
    handler = prefetch_aborts[ifsr & FSR_STATUS_MASK];
    if (handler) {
        int err;

        if ((err = handler(ifsr, ifar, spsr, lr, proc, thread))) {
            switch (err) {
            case -EACCES:
            case -EFAULT:
                pab_buserr(ifsr, ifar, spsr, lr, proc, thread);
                /* Doesn't return */
                break;
            default:
                KERROR(KERROR_CRIT, "PAB handling failed: %i\n", err);
                pab_fatal(ifsr, ifar, spsr, lr, proc, thread);
            }
        }
    } else {
       KERROR(KERROR_CRIT,
              "PAB handling failed, no sufficient handler found.\n");

       pab_fatal(ifsr, ifar, spsr, lr, proc, thread);
    }

    /*
     * TODO COR Support here
     * In the future we may wan't to support copy on read too
     * (ie. page swaping). To suppor cor, and actually anyway, we should test
     * if error appeared during reading or writing etc.
     */

    if (ABO_WAS_USERMODE(spsr)) {
        set_interrupt_state(s_entry);
    }
}

/**
 * PAB handler for fatal aborts.
 * @return Doesn't return.
 */
static int pab_fatal(uint32_t ifsr, uint32_t ifar, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread)
{
    pab_dump(ifsr, ifar, psr, lr, proc, thread);
    panic_halt();

    /* Doesn't return */
    return -ENOTRECOVERABLE;
}

static int pab_buserr(uint32_t ifsr, uint32_t ifar, uint32_t psr, uint32_t lr,
                      struct proc_info * proc, struct thread_info * thread)
{
    const struct ksignal_param sigparm = {
        .si_code = SEGV_MAPERR,
        .si_addr = (void *)ifar,
    };

    /* Some cases are always fatal */
    if (!ABO_WAS_USERMODE(psr) /* it happened in kernel mode */ ||
        (thread->pid_owner <= 1)) /* the proc is kernel or init */ {
        return -ENOTRECOVERABLE;
    }

    if (!proc)
        return -ESRCH;

#if configDEBUG >= KERROR_DEBUG
    pab_dump(ifsr, ifar, psr, lr, proc, thread);
    KERROR(KERROR_DEBUG, "%s: Send a fatal SIGSEGV\n", __func__);
#endif

    /* Deliver SIGSEGV. */
    ksignal_sendsig_fatal(proc, SIGSEGV, &sigparm);
    mmu_die_on_fatal_abort();

    return 0;
}

static pab_handler * const prefetch_aborts[] = {
    pab_fatal,  /* No function, reset value */
    pab_fatal,  /* Alignment fault */
    pab_fatal,  /* Debug event fault */
    pab_fatal,  /* Access Flag fault on Section */
    pab_fatal,  /* No function */
    pab_buserr, /* Translation fault on Section */
    pab_fatal,  /* Access Flag fault on Page */
    pab_fatal,  /* Translation fault on Page */
    pab_fatal,  /* Precise External Abort */
    pab_fatal,  /* Domain fault on Section */
    pab_fatal,  /* No function */
    pab_fatal,  /* Domain fault on Page */
    pab_fatal,  /* External abort on translation, first level */
    pab_fatal,  /* Permission fault on Section */
    pab_fatal,  /* External abort on translation, second level */
    pab_buserr  /* Permission fault on Page */
};
