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

/**
 * @addtogroup ksignal
 * Signal management.
 * @{
 */

#pragma once
#ifndef KSIGNAL_H
#define KSIGNAL_H

#include <signal.h>
#include <stdint.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <klocks.h>
#include <kobj.h>

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

enum signals_owner {
    SIGNALS_OWNER_UNSET = 0,
    SIGNALS_OWNER_PROCESS,
    SIGNALS_OWNER_THREAD,
};

/*
 * Ksignal sigs flags.
 */
#define KSIGFLAG_INTERRUPTIBLE  0x01 /*!< Interruptible syscall
                                      *   @note The syscall must handle this
                                      *   properly if set by the syscall!
                                      */
#define KSIGFLAG_SIGHANDLER     0x02 /*!< Going to user space signal handler
                                      *   after a syscall exit. */
#define KSIGFLAG_SA_KILL        0x10 /*!< The thread should be terminated
                                      *   as soon as possible.
                                      */

#define KSIGFLAG_IS_SET(_sigs_, _flag_) \
    (((_sigs_)->s_flags & (_flag_)) == (_flag_))

#define KSIGFLAG_SET(_sigs_, _flag_) \
    ((_sigs_)->s_flags |= (_flag_))

#define KSIGFLAG_CLEAR(_sigs_, _flag_) \
    ((_sigs_)->s_flags &= ~(_flag_))

/**
 * Thread signals struct.
 */
struct signals {
    enum signals_owner s_owner_type;    /*!< Type of the owner container. */
    unsigned s_flags;                   /*!< Sigs flags */
    sigset_t s_block;                   /*!< List of blocked signals. */
    sigset_t s_wait;                    /*!< Signal wait mask. */
    sigset_t s_running;                 /*!< Signals running mask. */
    struct sigwait_queue s_pendqueue;   /*!< Signals pending for handling. */
    struct sigaction_tree sa_tree;      /*!< Configured signal actions. */
    ksigmtx_t s_lock;
    struct kobj s_obj;
};

struct ksignal_param {
    int si_code;
    int si_errno;
    void * si_addr;
    int si_status;
    long si_band;
    union sigval si_value;
};

/**
 * @addtogroup ksignal_pendqueue
 * Operations for ksignal pending signals queue.
 * @{
 */

/**
 * Initialize a new signal queue.
 */
#define KSIGNAL_PENDQUEUE_INIT(_sigs) \
    STAILQ_INIT(&(_sigs)->s_pendqueue)

/**
 * Test if a queue is empty.
 */
#define KSIGNAL_PENDQUEUE_EMPTY(_sigs) \
    STAILQ_EMPTY(&(_sigs)->s_pendqueue)

/**
 * Foreach.
 */
#define KSIGNAL_PENDQUEUE_FOREACH(_var, _sigs) \
    STAILQ_FOREACH((_var), &(_sigs)->s_pendqueue, _entry)

/**
 * Removal safe foreach.
 */
#define KSIGNAL_PENDQUEUE_FOREACH_SAFE(_var, _sigs, _tmp) \
    STAILQ_FOREACH_SAFE((_var), &(_sigs)->s_pendqueue, _entry, (_tmp))

/**
 * Insert to head.
 */
#define KSIGNAL_PENDQUEUE_INSERT_HEAD(_sigs, _elm) \
    STAILQ_INSERT_HEAD(&(_sigs)->s_pendqueue, (_elm), _entry)

/**
 * Insert to tail.
 */
#define KSIGNAL_PENDQUEUE_INSERT_TAIL(_sigs, _elm) \
    STAILQ_INSERT_TAIL(&(_sigs)->s_pendqueue, (_elm), _entry)

/**
 * Remove an element from a queue.
 */
#define KSIGNAL_PENDQUEUE_REMOVE(_sigs, _elm) \
    STAILQ_REMOVE(&(_sigs)->s_pendqueue, (_elm), ksiginfo, _entry)

/**
 * @}
 */

/*
 * Additional signal codes OR'ed with exit_signal.
 */
#define KSIGNAL_EXIT_SIGNAL_CORE    0x40000000 /*!< Hint to dump core. */

struct thread_info;

RB_PROTOTYPE(sigaction_tree, ksigaction, _entry, signum_comp);
int signum_comp(struct ksigaction * a, struct ksigaction * b);

/**
 * Get a string name for a signal number.
 */
const char * ksignal_signum2str(int signum);

/**
 * Sigs struct constructor.
 */
void ksignal_signals_ctor(struct signals * sigs, enum signals_owner owner_type);

/**
 * Re-init sigs on thread fork.
 */
void ksignal_signals_fork_reinit(struct signals * sigs);

void ksignal_signals_dtor(struct signals * sigs);

/**
 * Send signal to a process or thread.
 * @param sigs is a sigs struct owned by a process or thread.
 * @param signum is the signal number to be signald.
 */
int ksignal_sendsig(struct signals * sigs, int signum,
                    const struct ksignal_param * param);

/**
 * Kill process by sending a fatal signal that can't be blocked.
 * @param p is the process to be signaled.
 * @param signum is the signum used.
 */
void ksignal_sendsig_fatal(struct proc_info * p, int signum,
                           const struct ksignal_param * param);

/**
 * Wait for signal(s) specified in set.
 */
int ksignal_sigwait(siginfo_t * retval, const sigset_t * restrict set);

/**
 * Wait for signal(s) specified in set until timeout.
 * @returns Returns 0 if a signal was received;
 *          Otherwise a negative error code is returned.
 */
int ksignal_sigtimedwait(siginfo_t * retval, const sigset_t * restrict set,
                         const struct timespec * restrict timeout);

/**
 * Sleep until timeout or a non-ignored signal is received.
 * Only for syscalls.
 */
int ksignal_sigsleep(const struct timespec * restrict timeout);

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
 * Enter to a syscall.
 * This function must be called before entering a syscall (and is called in
 * syscall.c). This function also sets SCHED_INSYS_FLAG for the current thread
 * because its synchronization with ksignal system is somewhat important to
 * determine the correct behavior.
 */
void ksignal_syscall_enter(void);

/**
 * Exit from a syscall.
 * Determines if there is any postponed actions to be performed due to received
 * signals before we can return to the caller in user mode.
 * - If KSIGFLAG_SA_KILL is set the thread will be terminated immediately.
 * - This function determines if we are going to execute a signal handler
 *   after returning from this syscall or return to the caller and adjusts
 *   stack and return values accordingly.
 * -- KSIGFLAG_INTERRUPTIBLE is cleared by this function and
 *    KSIGFLAG_SIGHANDLER selects the action.
 * @param retval is the return value that shall be returned to the original
 *               caller.
 * @returns Returns a value that should be immediately returned with return,
 *          this value is will be either a return value to the caller or an
 *          argument to the signal handler executed next.
 */
int ksignal_syscall_exit(int retval);

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

/**
 * @}
 */
