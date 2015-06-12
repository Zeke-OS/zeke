/**
 *******************************************************************************
 * @file    arm11_mmu_dab.c
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
#include <proc.h>
#include <hal/core.h>
#include <hal/mmu.h>

static dab_handler * const data_aborts[];

static const char * dab_fsr_strerr[] = {
    /* String                       FSR[10,3:0] */
    "TLB Miss",                     /* 0x000 */
    "Alignment",                    /* 0x001 */
    "Instruction debug event",      /* 0x002 */
    "Section AP fault",             /* 0x003 */
    "Icache maintenance op fault",  /* 0x004 */
    "Section translation",          /* 0x005 */
    "Page AP fault",                /* 0x006 */
    "Page translation",             /* 0x007 */
    "Precise external abort",       /* 0x008 */
    "Domain section fault",         /* 0x009 */
    "",                             /* 0x00A */
    "Domain page fault",            /* 0x00B */
    "Extrenal first-level abort",   /* 0x00C */
    "Section permission fault",     /* 0x00D */
    "External second-evel abort",   /* 0x00E */
    "Page permission fault",        /* 0x00F */
    "",                             /* 0x010 */
    "",                             /* 0x011 */
    "",                             /* 0x012 */
    "",                             /* 0x013 */
    "",                             /* 0x014 */
    "",                             /* 0x015 */
    "Imprecise external abort",     /* 0x406 */
    "",                             /* 0x017 */
    "Parity error exception, ns",   /* 0x408 */
    "",                             /* 0x019 */
    "",                             /* 0x01A */
    "",                             /* 0x01B */
    "",                             /* 0x01C */
    "",                             /* 0x01D */
    "",                             /* 0x01E */
    ""                              /* 0x01F */
};

static int dab_fatal(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread);
static int dab_buserr(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                      struct proc_info * proc, struct thread_info * thread);

static const char * get_dab_strerror(uint32_t fsr)
{
    uint32_t tmp;

    tmp = fsr & FSR_STATUS_MASK;
    tmp |= (fsr & 0x400) >> 6;

    return dab_fsr_strerr[tmp];
}

/**
 * Data abort handler.
 */
void mmu_data_abort_handler(void)
{
    uint32_t fsr; /*!< Fault status */
    uint32_t far; /*!< Fault address */
    const uint32_t spsr = current_thread->sframe[SCHED_SFRAME_ABO].psr;
    const uint32_t lr = current_thread->sframe[SCHED_SFRAME_ABO].pc;
#if 0
    const istate_t s_old = spsr & PSR_INT_MASK; /*!< Old interrupt state */
#endif
    istate_t s_entry; /*!< Int state in handler entry. */
    struct thread_info * const thread = (struct thread_info *)current_thread;
    struct proc_info * proc;
    dab_handler * handler;

    /* Get fault status */
    __asm__ volatile (
        "MRC p15, 0, %[reg], c5, c0, 0"
        : [reg]"=r" (fsr));
    /* Get fault address */
    __asm__ volatile (
        "MRC p15, 0, %[reg], c6, c0, 0"
        : [reg]"=r" (far));

    mmu_pf_event();

    if (!thread) {
        panic("Thread not set on DAB");
    }

    /*
     * RFE Block the thread owner
     * We may want to block the process owning this thread and possibly
     * make sure that this instance is the only one handling page fault of the
     * same kind.
     */

    /* TODO Handle DAB in preemptible state. */
    /* Handle this data abort in pre-emptible state if possible. */
    if (ABO_WAS_USERMODE(spsr)) {
        s_entry = get_interrupt_state();
#if 0
        set_interrupt_state(s_old);
#endif
    }

    /* RFE Might be enough to get curproc. */
    proc = proc_get_struct_l(thread->pid_owner); /* Can be NULL */
    handler = data_aborts[fsr & FSR_STATUS_MASK];
    if (handler) {
        int err;

        if ((err = handler(fsr, far, spsr, lr, proc, thread))) {
            switch (err) {
            case -EACCES:
            case -EFAULT:
                dab_buserr(fsr, far, spsr, lr, proc, thread);
                /* Doesn't return */
                break;
            default:
                KERROR(KERROR_CRIT, "DAB handling failed: %i\n", err);
                stack_dump(current_thread->sframe[SCHED_SFRAME_ABO]);
                dab_fatal(fsr, far, spsr, lr, proc, thread);
            }
        }
    } else {
       KERROR(KERROR_CRIT,
              "DAB handling failed, no sufficient handler found.\n");

       dab_fatal(fsr, far, spsr, lr, proc, thread);
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
 * DAB handler for fatal aborts.
 * @return Doesn't return.
 */
static int dab_fatal(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread)
{
    KERROR(KERROR_CRIT,
           "Fatal DAB:\n"
           "pc: %x\n"
           "fsr: %x (%s)\n"
           "far: %x\n"
           "proc info:\n"
           "pid: %i\n"
           "tid: %i\n"
           "insys: %i\n",
           lr,
           fsr, get_dab_strerror(fsr),
           far,
           (int32_t)((proc) ? proc->pid : -1),
           (int32_t)thread->id,
           (int32_t)thread_flags_is_set(thread, SCHED_INSYS_FLAG));
    panic("Can't handle data abort");

    /* Doesn't return */
    return -ENOTRECOVERABLE;
}

/**
 * DAB handler for alignment aborts.
 */
static int dab_align(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread)
{
    /* Some cases are always fatal if */
    if (!ABO_WAS_USERMODE(psr) /* it is a kernel mode alignment fault, */ ||
        (thread->pid_owner <= 1))  /* the proc is kernel or init */ {
        return -ENOTRECOVERABLE;
    }

    if (!proc)
        return -ESRCH;

#if configDEBUG >= KERROR_DEBUG
    KERROR(KERROR_DEBUG, "%s: Send a fatal SIGBUS\n", __func__);
#endif

    /*
     * Deliver SIGBUS.
     * TODO Instead of sending a signal we should probably try to handle the
     *      error first.
     */
    ksignal_sendsig_fatal(proc, SIGBUS);
    mmu_die_on_fatal_abort();

    return 0;
}

static int dab_buserr(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                      struct proc_info * proc, struct thread_info * thread)
{
    /* Some cases are always fatal */
    if (!ABO_WAS_USERMODE(psr) /* it happened in kernel mode */ ||
        (thread->pid_owner <= 1)) /* the proc is kernel or init */ {
        return -ENOTRECOVERABLE;
    }

    if (!proc)
        return -ESRCH;

#if configDEBUG >= KERROR_DEBUG
    KERROR(KERROR_DEBUG, "%s: Send a fatal SIGBUS\n", __func__);
#endif

    /* Deliver SIGBUS. */
    ksignal_sendsig_fatal(proc, SIGBUS);
    mmu_die_on_fatal_abort();

    return 0;
}

static dab_handler * const data_aborts[] = {
    dab_fatal,          /* no function, reset value */
    dab_align,          /* Alignment fault */
    dab_fatal,          /* Instruction debug event */
    proc_dab_handler,   /* Access bit fault on Section */
    dab_buserr,         /* ICache maintanance op fault */
    proc_dab_handler,   /* Translation Section Fault */
    proc_dab_handler,   /* Access bit fault on Page */
    proc_dab_handler,   /* Translation Page fault */
    dab_buserr,         /* Precise external abort */
    dab_buserr,         /* Domain Section fault */ /* TODO not really buserr */
    dab_fatal,          /* no function */
    dab_buserr,         /* Domain Page fault */ /* TODO Not really buserr */
    dab_buserr,         /* External abort on translation, first level */
    proc_dab_handler,   /* Permission Section fault */
    dab_buserr,         /* External abort on translation, second level */
    proc_dab_handler    /* Permission Page fault */
};
