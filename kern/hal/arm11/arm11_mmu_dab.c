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

static abo_handler * const data_aborts[];

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

static int dab_fatal(const struct mmu_abo_param * restrict abo);

const char * arm11_get_dab_strerror(uint32_t fsr)
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
    struct mmu_abo_param abo;
    abo_handler * handler;

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
    handler = data_aborts[fsr & FSR_STATUS_MASK];

    abo = (struct mmu_abo_param){
        .abo_type = MMU_ABO_DATA,
        .fsr = fsr,
        .far = far,
        .psr = spsr,
        .lr = lr,
        .proc = proc_ref(thread->pid_owner, PROC_NOT_LOCKED), /* Can be NULL */
        .thread = thread,
    };

    if (handler) {
        int err;

        if ((err = handler(&abo))) {
            switch (err) {
            case -EACCES:
            case -EFAULT:
                arm11_abo_buser(&abo);
                /* Doesn't return */
                break;
            default:
                KERROR(KERROR_CRIT, "DAB handling failed: %i\n", err);
                dab_fatal(&abo);
            }
        }
    } else {
       KERROR(KERROR_CRIT,
              "DAB handling failed, no sufficient handler found.\n");

       dab_fatal(&abo);
    }

    /*
     * TODO COR Support here
     * In the future we may wan't to support copy on read too
     * (ie. page swaping). To suppor cor, and actually anyway, we should test
     * if error appeared during reading or writing etc.
     */

    proc_unref(abo.proc);

    if (ABO_WAS_USERMODE(spsr)) {
        set_interrupt_state(s_entry);
    }
}

/**
 * DAB handler for fatal aborts.
 * @return Doesn't return.
 */
static int dab_fatal(const struct mmu_abo_param * restrict abo)
{
    mmu_abo_dump(abo);
    panic("Can't handle data abort");

    /* Doesn't return */
    return -ENOTRECOVERABLE;
}

/**
 * DAB handler for alignment aborts.
 */
static int dab_align(const struct mmu_abo_param * restrict abo)
{
    const struct ksignal_param sigparm = {
        .si_code = BUS_ADRALN,
        .si_addr = (void *)abo->far,
    };

    /* Some cases are always fatal if */
    if (!ABO_WAS_USERMODE(abo->psr) /* it is a kernel mode alignment fault */ ||
        (abo->thread->pid_owner <= 1))  /* the proc is kernel or init */ {
        return -ENOTRECOVERABLE;
    }

    if (!abo->proc)
        return -ESRCH;

    mmu_abo_dump(abo);
    KERROR(KERROR_DEBUG, "%s: Send a fatal SIGBUS to %d\n",
           __func__, abo->proc->pid);

    /*
     * Deliver SIGBUS.
     * TODO Instead of sending a signal we should probably try to handle the
     *      error first.
     */
    ksignal_sendsig_fatal(abo->proc, SIGBUS, &sigparm);
    mmu_die_on_fatal_abort();

    return 0;
}

static abo_handler * const data_aborts[] = {
    dab_fatal,          /* no function, reset value */
    dab_align,          /* Alignment fault */
    dab_fatal,          /* Instruction debug event */
    proc_abo_handler,   /* Access bit fault on Section */
    arm11_abo_buser,    /* ICache maintanance op fault */
    proc_abo_handler,   /* Translation Section Fault */
    proc_abo_handler,   /* Access bit fault on Page */
    proc_abo_handler,   /* Translation Page fault */
    arm11_abo_buser,    /* Precise external abort */
    arm11_abo_buser,    /* Domain Section fault */ /* TODO not really buserr */
    dab_fatal,          /* no function */
    arm11_abo_buser,    /* Domain Page fault */ /* TODO Not really buserr */
    arm11_abo_buser,    /* External abort on translation, first level */
    proc_abo_handler,   /* Permission Section fault */
    arm11_abo_buser,    /* External abort on translation, second level */
    proc_abo_handler    /* Permission Page fault */
};
