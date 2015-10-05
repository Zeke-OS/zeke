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

static abo_handler * const prefetch_aborts[];

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

static int pab_fatal(const struct mmu_abo_param * restrict abo);

const char * get_pab_strerror(uint32_t ifsr)
{
    return pab_fsr_strerr[ifsr & FSR_STATUS_MASK];
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
    struct mmu_abo_param abo;
    abo_handler * handler;

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

    /* TODO Handle PAB in preemptible state. */
    /* Handle this prefetch abort in pre-emptible state if possible. */
    if (ABO_WAS_USERMODE(spsr)) {
        s_entry = get_interrupt_state();
#if 0
        set_interrupt_state(s_old);
#endif
    }

    /*
     * TODO If the abort came from user space and it was BKPT then it was meant
     *      for a debugger.
     */

    /* RFE Might be enough to get curproc. */
    handler = prefetch_aborts[ifsr & FSR_STATUS_MASK];
    abo = (struct mmu_abo_param){
        .abo_type = MMU_ABO_PREFETCH,
        .fsr = ifsr,
        .far = ifar,
        .psr = spsr,
        .lr = lr,
        .proc = proc_get_struct_l(thread->pid_owner), /* Can be NULL */
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
                KERROR(KERROR_CRIT, "PAB handling failed: %i\n", err);
                pab_fatal(&abo);
            }
        }
    } else {
       KERROR(KERROR_CRIT,
              "PAB handling failed, no sufficient handler found.\n");

       pab_fatal(&abo);
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
static int pab_fatal(const struct mmu_abo_param * restrict abo)
{
    arm11_abo_dump(abo);
    panic_halt();

    /* Doesn't return */
    return -ENOTRECOVERABLE;
}

static abo_handler * const prefetch_aborts[] = {
    pab_fatal,          /* No function, reset value */
    pab_fatal,          /* Alignment fault */
    pab_fatal,          /* Debug event fault */
    proc_abo_handler,   /* Access Flag fault on Section */
    pab_fatal,          /* No function */
    proc_abo_handler,   /* Translation fault on Section */
    proc_abo_handler,   /* Access Flag fault on Page */
    proc_abo_handler,   /* Translation fault on Page. */
    pab_fatal,          /* Precise External Abort */
    pab_fatal,          /* Domain fault on Section */
    pab_fatal,          /* No function */
    pab_fatal,          /* Domain fault on Page */
    pab_fatal,          /* External abort on translation, first level */
    proc_abo_handler,   /* Permission fault on Section */
    pab_fatal,          /* External abort on translation, second level */
    proc_abo_handler    /* Permission fault on Page */
};
