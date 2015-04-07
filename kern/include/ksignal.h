/**
 *******************************************************************************
 * @file    ksignal.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for thread Signal Management in kernel (ksignal.c).
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy
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

#pragma once
#ifndef KSIGNAL_H
#define KSIGNAL_H

#include <sys/queue.h>
#include <sys/tree.h>
#include <stdint.h>
#include <klocks.h>
#include <signal.h>

struct proc_info;

/**
 * Kernel signal info struct type that is used to deliver signals to
 * processes and threads.
 */
struct ksiginfo {
    siginfo_t siginfo;
    STAILQ_ENTRY(ksiginfo) _entry;
};

/**
 * Kernel signal action descriptor.
 */
struct ksigaction {
    int ks_signum;
    struct sigaction ks_action;
    RB_ENTRY(ksigaction) _entry; /* Should be the last entry. */
};

STAILQ_HEAD(sigwait_queue, ksiginfo);
RB_HEAD(sigaction_tree, ksigaction);

/**
 * Special lock type for ksignal.
 */
typedef struct _ksigmtx_ {
    mtx_t l;
} ksigmtx_t;

/**
 * Thread signals struct.
 */
struct signals {
    sigset_t s_block;                   /*!< List of blocked signals. */
    sigset_t s_wait;                    /*!< Signal wait mask. */
    sigset_t s_running;                 /*!< Signals running mask. */
    struct sigwait_queue s_pendqueue;   /*!< Signals pending for handling. */
    struct sigaction_tree sa_tree;      /*!< Configured signal actions. */
    uintptr_t s_usigret;    /*!< Address of the sigret() function in uspace. */
    ksigmtx_t s_lock;
};

struct thread_info;

RB_PROTOTYPE(sigaction_tree, ksigaction, _entry, signum_comp);
int signum_comp(struct ksigaction * a, struct ksigaction * b);

/**
 * Sigs struct constructor.
 */
void ksignal_signals_ctor(struct signals * sigs);

/**
 * Re-init sigs on thread fork.
 */
void ksignal_signals_fork_reinit(struct signals * sigs);

/**
 * Send signal to a process or thread.
 * @param sigs is a sigs struct owned by a process or thread.
 * @param signum is the signal number to be signald.
 * @param si_code is a SI_ code indentifying the sender of the signal.
 */
int ksignal_sendsig(struct signals * sigs, int signum, int si_code);

/**
 * Kill process by sending a fatal signal that can't be blocked.
 * @param p is the process to be signaled.
 * @param signum is the signum used.
 * @returns Returns 0 if succeed; Otherwise a negative error code is returned.
 */
int ksignal_sendsig_fatal(struct proc_info * p, int signum);

/**
 * Wait for signal(s) specified in set.
 */
int ksignal_sigwait(siginfo_t * retval, const sigset_t * restrict set);

int ksignal_sigtimedwait(siginfo_t * retval, const sigset_t * restrict set,
                         const struct timespec * restrict timeout);

/**
 * Check if a signal is blocked.
 * @param sigs is a pointer to a signals struct, that's already locked.
 * @param signum is the signal number to be checked.
 * @returns 0 if the given signal is not blocked; Value other than zero if
 *          the signal is blocked.
 */
int ksignal_isblocked(struct signals * sigs, int signum);

/**
 * Manipulate sigs signal mask.
 * This is same as user space sigprocmask but in kernel space and directly uses
 * signals struct.
 * @param sigs is a pointer to a signals struct; will be locked in this func.
 * @param how is SIG_BLOCK, SIG_SETMASK, or SIG_UNBLOCK.
 * @param set is the new set.
 * @param oldset is where the old set will be copied if oldset is set;
 *               Can be NULL.
 * @returns Returns 0 if succeed; Otherwise a negative error code is returned.
 */
int ksignal_sigsmask(struct signals * sigs, int how,
                     const sigset_t * restrict set,
                     sigset_t * restrict oldset);

/**
 * Get copy of a signal action struct.
 * @param action[out] is a pointer to a struct than will be modified.
 * @param sigs[in]    is a pointer to a signals struct, that's already locked.
 * @param signum      is the signal number.
 * @returns Returns 0 if operation succeed;
 *          Otherwise a negative error code is returned.
 */
void ksignal_get_ksigaction(struct ksigaction * action,
                            struct signals * sigs, int signum);

/**
 * Reset signal action to default.
 * @param sigs is a pointer to a signals struct, that's already locked.
 * @param signum is the signal number.
 * @returns Returns 0 if operation succeed;
 *          Otherwise a negative error code is returned.
 */
int ksignal_reset_ksigaction(struct signals * sigs, int signum);

/**
 * Set signal action.
 * @note action can be allocated from stack as its data will be copied.
 */
int ksignal_set_ksigaction(struct signals * sigs, struct ksigaction * action);

/**
 * @addtogroup kernel_sigsetops
 * sigsetops
 * Manipulate signal sets.
 * @{
 */

/**
 * Add a signal to a signal set.
 */
int sigaddset(sigset_t * set, int signo);

/**
 * Delete a signal from a signal set.
 */
int sigdelset(sigset_t * set, int signo);

/**
 * Initialize and empty a signal set.
 * No signals are set.
 */
int sigemptyset(sigset_t * set);

/**
 * Initialize and fill a signal set.
 * All defined signals will be included.
 */
int sigfillset(sigset_t * set);

/**
 * Test for a signal in a signal set.
 * Test if signo is a member of the signal set set.
 */
int sigismember(const sigset_t * set, int signo);

/**
 * Test if a signal set is an empty set.
 */
int sigisemptyset(const sigset_t * set);

/**
 * Find first signal in a signal set.
 */
int sigffs(sigset_t * set);

/**
 * Take union of sigset a and b to target.
 */
sigset_t * sigunion(sigset_t * target, const sigset_t * a,
                    const sigset_t * b);

/**
 * Take intersection of sigset a and b to target.
 */
sigset_t * sigintersect(sigset_t * target, const sigset_t * a,
                        const sigset_t * b);

/**
 * Take complement of sigset set to target.
 */
sigset_t * sigcompl(sigset_t * target, const sigset_t * set);

/**
 * @}
 */

#endif /* KSIGNAL_H */

