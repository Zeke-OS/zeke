/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *        The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *        @(#)kern_sig.c        8.7 (Berkeley) 4/18/94
 *******************************************************************************
 */

#define KERNEL_INTERNAL 1
#include <sys/tree.h>
#include <syscall.h>
#include <errno.h>
#include <libkern.h>
#include <tsched.h>
#include <proc.h>
#include <sys/priv.h>
#include <timers.h>
#include <kmalloc.h>
#include <kstring.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include "ksignal.h"

#define KSIG_LOCK_FLAGS (MTX_TYPE_TICKET | MTX_TYPE_SLEEP | MTX_TYPE_PRICEIL)

static int kern_logsigexit = 1;
SYSCTL_INT(_kern, KERN_LOGSIGEXIT, logsigexit, CTLFLAG_RW,
           &kern_logsigexit, 0,
           "Log processes quitting on abnormal signals to syslog(3)");

/*
 * Signal default actions.
 */
static const uint8_t default_sigproptbl[] = {
    SA_IGNORE,          /*!< Not a signal */
    SA_KILL,            /*!< SIGHUP */
    SA_KILL,            /*!< SIGINT */
    SA_KILL|SA_CORE,    /*!< SIGQUIT */
    SA_KILL|SA_CORE,    /*!< SIGILL */
    SA_KILL|SA_CORE,    /*!< SIGTRAP */
    SA_KILL|SA_CORE,    /*!< SIGABRT */
    SA_IGNORE,          /*!< SIGCHLD */
    SA_KILL|SA_CORE,    /*!< SIGFPE */
    SA_KILL,            /*!< SIGKILL */
    SA_KILL|SA_CORE,    /*!< SIGBUS */
    SA_KILL|SA_CORE,    /*!< SIGSEGV */
    SA_IGNORE|SA_CONT,  /*!< SIGCONT */
    SA_KILL,            /*!< SIGPIPE */
    SA_KILL,            /*!< SIGALRM */
    SA_KILL,            /*!< SIGTERM */
    SA_STOP,            /*!< SIGSTOP */
    SA_STOP|SA_TTYSTOP, /*!< SIGTSTP */
    SA_STOP|SA_TTYSTOP, /*!< SIGTTIN */
    SA_STOP|SA_TTYSTOP, /*!< SIGTTOU */
    SA_KILL,            /*!< SIGUSR1 */
    SA_KILL,            /*!< SIGUSR2 */
    SA_KILL|SA_CORE,    /*!< SIGSYS */
    SA_IGNORE,          /*!< SIGURG */
    SA_IGNORE,          /*!< SIGINFO */
    SA_KILL             /*!< SIGPWR */
};

RB_GENERATE(sigaction_tree, ksigaction, _entry, signum_comp);

int signum_comp(struct ksigaction * a, struct ksigaction * b)
{
    KASSERT((a && b), "a & b must be set");

     return a->ks_signum - b->ks_signum;
}

static int ksig_lock(mtx_t * lock)
{
    istate_t s;

    s = get_interrupt_state();
    if (s & PSR_INT_I) {
        return mtx_trylock(lock);
    } else {
        return mtx_lock(lock);
    }
}

static void ksig_unlock(mtx_t * lock)
{
    istate_t s;

    s = get_interrupt_state();
    if (s & PSR_INT_I)
        return;

    mtx_unlock(lock);
}

static void ksignal_thread_ctor(struct thread_info * th)
{
    RB_INIT(&th->sigs.sa_tree);
    mtx_init(&th->sigs.s_lock, KSIG_LOCK_FLAGS);
}
DATA_SET(thread_ctors, ksignal_thread_ctor);

static void ksignal_fork_handler(struct thread_info * th)
{
    struct sigaction_tree old_tree = th->sigs.sa_tree;
    struct ksigaction * sigact_old;

    /* Clear pending signals as required by POSIX. */
    memset(&th->sigs.s_pending, 0, sizeof(th->sigs.s_pending));

    /* Clone configured signal actions. */
    RB_INIT(&th->sigs.sa_tree);
    RB_FOREACH(sigact_old, sigaction_tree, &old_tree) {
        struct ksigaction * sigact_new = kmalloc(sizeof(struct ksigaction));

        KASSERT(sigact_new != NULL, "OOM during thread fork\n");

        memcpy(sigact_new, sigact_old, sizeof(struct ksigaction));
        RB_INSERT(sigaction_tree, &th->sigs.sa_tree, sigact_new);
    }

    /* Reinit mutex lock */
    mtx_init(&th->sigs.s_lock, KSIG_LOCK_FLAGS);
}
DATA_SET(thread_fork_handlers, ksignal_fork_handler);

/**
 * Allocate memory from thread user stack.
 * @note Works only for any thread of the current process.
 * @param thread is the thread owning the stack.
 * @param len is the length of the allocation.
 * @return Returns pointer to the uaddr of the allocation if succeed;
 * Otherwise NULL.
 */
static void * usr_stack_alloc(struct thread_info * thread, size_t len)
{
    int framenum;
    sw_stack_frame_t * frame;
    void * sp;

    KASSERT(thread, "thread should be always set.\n");
    KASSERT(len > 0, "len should be greater than zero.\n");

    if (thread->flags & SCHED_INSYS_FLAG)
        framenum = SCHED_SFRAME_SVC;
    else
        framenum = SCHED_SFRAME_SYS;

    frame = &current_thread->sframe[framenum];
    sp = (void *)frame->sp;

    /* Check user access */
    if (!sp || !useracc(sp, len, VM_PROT_READ | VM_PROT_WRITE))
        return NULL;

    /* Make allocation. */
    frame->sp += memalign(len);

    return sp;
}

/**
 * Post thread scheduling handler that updates thread stack frame if a signal
 * is pending. After this handler the thread will enter to signal handler
 * instead of returning to normal execution.
 */
static void ksignal_post_scheduling(void)
{
    int signum;
    struct signals * sigs = &current_thread->sigs;
#if 0
    struct ksigaction action;
#endif
    sw_stack_frame_t * save_frame;

    if (ksig_lock(&sigs->s_lock))
        return; /* Can't handle signals now. */

    signum = sigffs(&sigs->s_pending);
    if (signum < 0)
        return; /* No signals pending. */

#if 0
    ksignal_get_ksigaction(&action, sigs, signum);
#endif

    if (sigismember(&sigs->s_running, signum)) {
        /* Already running a handler for that signum */
        sigdelset(&sigs->s_running, signum);

        return;
    }

    if (ksignal_isblocked(sigs, signum))
        return; /* Signal is currently blocked. */

    /* Allocate memory from user thread stack */
    save_frame = usr_stack_alloc(current_thread, sizeof(sw_stack_frame_t));
    if (!save_frame) {
        /*
         * Thread has trashed its stack; Nothing we can do but
         * give SIGILL.
         */
        ksignal_sendsig_fatal(current_thread->id, SIGILL);
        return; /* TODO Is this ok? */
    }

    panic("Can't handle signals yet\n");

    /*
     * TODO
     * - Check current_thread sigs
     *  -- if we have to enter sig handler save stack frame on top of user stack
     *     or alt stack
     *  -- Change return address to our own handler to call syscall that
     *     reverts signal handling state
     *  -- Change to alt stack if requested
     * - or handle signal in kernel
     */

    ksig_unlock(&sigs->s_lock);
}
DATA_SET(post_sched_tasks, ksignal_post_scheduling);

int ksignal_thread_sendsig(struct thread_info * thread, int signum)
{
    int retval = 0;
    struct signals * sigs;
    struct ksigaction action;

    if (!thread)
        return -EINVAL;
    sigs = &thread->sigs;

    if (ksig_lock(&sigs->s_lock))
        return -EWOULDBLOCK;

    retval = sigismember(&sigs->s_running, signum);
    if (retval) /* Already running a handler. */
        goto out;

    /* Get action struct for this signal. */
    ksignal_get_ksigaction(&action, sigs, signum);

    /* Ignored? */
    if (action.ks_action.sa_handler == SIG_IGN)
        goto out;

    /* Not ignored so we can set the signal to pending state. */
    retval = sigaddset(&sigs->s_pending, signum);
    if (retval)
        goto out;

    /* Signal is not blocked or the thread is waiting for this signal. */
    if (!ksignal_isblocked(sigs, signum) || sigismember(&sigs->s_wait, signum)) {
        /* so set exec */
        sched_thread_set_exec(thread->id);
    }

out:
    ksig_unlock(&sigs->s_lock);

    return 0;
}

/**
 * Kill process by a fatal signal that can't be blocked.
 */
int ksignal_sendsig_fatal(pthread_t thid, int signum)
{
    /* TODO */
    return 0;
}

int ksignal_isblocked(struct signals * sigs, int signum)
{
    KASSERT(mtx_test(&sigs->s_lock), "sigs should be locked\n");

    /*
     * TODO IEEE Std 1003.1, 2004 Edition
     * When a signal is caught by a signal-catching function installed by
     * sigaction(), a new signal mask is calculated and installed for
     * the duration of the signal-catching function (or until a call to either
     * sigprocmask() or sigsuspend() is made). This mask is formed by taking
     * the union of the current signal mask and the value of the sa_mask for
     * the signal being delivered [XSI]   unless SA_NODEFER or SA_RESETHAND
     * is set, and then including the signal being delivered. If and when
     * the user's signal handler returns normally, the original signal
     * mask is restored.
     */
    if (sigismember(&sigs->s_block, signum))
        return 1;
    return 0;
}

/**
 * Get a copy of signal action struct.
 */
void ksignal_get_ksigaction(struct ksigaction * action,
                            struct signals * sigs, int signum)
{
    struct ksigaction find = { .ks_signum = signum };
    struct ksigaction * p_action;

    KASSERT(action, "Action should be set\n");
    KASSERT(signum >= 0, "Signum should be positive\n");
    KASSERT(mtx_test(&sigs->s_lock), "sigs should be locked\n");

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, &find))) {
        memcpy(action, p_action, sizeof(struct ksigaction));
        return;
    }

    action->ks_signum = signum;
    sigemptyset(&action->ks_action.sa_mask);
    action->ks_action.sa_flags = (signum < sizeof(default_sigproptbl)) ?
        default_sigproptbl[signum] : SA_IGNORE;
    action->ks_action.sa_handler = SIG_DFL;
}

/**
 * Set signal action struct.
 * @note Always copied, so action struct can be allocated from stack.
 */
int ksignal_set_ksigaction(struct signals * sigs, struct ksigaction * action)
{
    struct ksigaction * p_action;
    const int signum = action->ks_signum;
    struct sigaction * sigact = NULL;

    KASSERT(mtx_test(&sigs->s_lock), "sigs should be locked\n");

    if (!action)
        return -EINVAL;

    if (action->ks_signum < 0)
        return -EINVAL;

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, action))) {

        memcpy(p_action, action, sizeof(struct ksigaction) -
                sizeof(RB_ENTRY(ksigaction)));
    } else {
        p_action = kmalloc(sizeof(struct ksigaction));
        memcpy(p_action, action, sizeof(struct ksigaction) -
                sizeof(RB_ENTRY(ksigaction)));

        if (RB_INSERT(sigaction_tree, &sigs->sa_tree, p_action)) {
            panic("ksignal_set_ksigaction() failed to insert.\n");
        }
    }

    /* Check if this action can be actually removed */
    sigact = &p_action->ks_action;
    if (sigisemptyset(&sigact->sa_mask) &&
        (sigact->sa_flags == (signum < sizeof(default_sigproptbl)) ?
            default_sigproptbl[signum] : SA_IGNORE) &&
        (sigact->sa_handler == SIG_DFL)) {
        if (RB_REMOVE(sigaction_tree, &sigs->sa_tree, p_action)) {
            kfree(p_action);
        } else {
            panic("Can't remove an entry from sigaction_tree\n");
        }
    }

    return 0;
}

/**
 * Send a signal to a process or a group of processes.
 */
static int sys_signal_pkill(void * user_args)
{
    struct _pkill_args args;
    proc_info_t * proc;
    struct thread_info * thread;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    proc = proc_get_struct(args.pid);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }

    /*
     * Check if process is privileged to signal other users.
     */
    if ((curproc->euid != proc->uid && curproc->euid != proc->suid) &&
        (curproc->uid  != proc->uid && curproc->uid  != proc->suid)) {
        if (priv_check(curproc, PRIV_SIGNAL_OTHER)) {
            set_errno(EPERM);
            return -1;
        }
    }

    /*
     * The null signal can be used to check the validity of pid.
     * IEEE Std 1003.1, 2013 Edition
     *
     * If sig == 0 we can return immediately.
     */
    if (args.sig == 0)
        return 0;

    /*
     * TODO
     * Proper signaling logic:
     * - send to a sigwaiting thread or sighandling thread
     * - send to all
     */
    thread = curproc->main_thread;
    if (!thread) {
        set_errno(ESRCH);
        return -1;
    }

    err = ksignal_thread_sendsig(thread, args.sig);
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

/**
 * Send a signal to a thread or threads.
 */
static int sys_signal_tkill(void * user_args)
{
    struct _tkill_args args;
    struct thread_info * thread;
    proc_info_t * proc;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    thread = sched_get_thread_info(args.thread_id);
    if (!thread) {
        set_errno(ESRCH);
        return -1;
    }

    proc = proc_get_struct(thread->pid_owner);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }

    /*
     * Check if process is privileged to signal other users.
     */
    if ((curproc->euid != proc->uid && curproc->euid != proc->suid) &&
        (curproc->uid  != proc->uid && curproc->uid  != proc->suid)) {
        if (priv_check(curproc, PRIV_SIGNAL_OTHER)) {
            set_errno(EPERM);
            return -1;
        }
    }

    /*
     * The null signal can be used to check the validity of pid. (thread id)
     * IEEE Std 1003.1, 2013 Edition
     *
     * If sig == 0 we can return immediately.
     */
    if (args.sig == 0)
        return 0;

    err = ksignal_thread_sendsig(thread, args.sig);
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_signal_action(void * user_args)
{
    struct _signal_action_args args;
    int err;

    if (priv_check(curproc, PRIV_SIGNAL_ACTION)) {
        set_errno(ENOTSUP);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    err = ksignal_set_ksigaction(&current_thread->sigs,
            &(struct ksigaction){
                .ks_signum = args.signum,
                .ks_action = args.action
            });
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_signal_altstack(void * user_args)
{
    /* TODO */
    set_errno(ENOTSUP);
    return -1;
}

static int sys_signal_return(void * user_args)
{
    /*
     * TODO
     * - Return from signal, revert stack frame and alt stack
     */

    return 0;
}

static const syscall_handler_t ksignal_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_PKILL,      sys_signal_pkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_TKILL,      sys_signal_tkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ACTION,     sys_signal_action),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ALTSTACK,   sys_signal_altstack),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_RETURN,     sys_signal_return),
};
SYSCALL_HANDLERDEF(ksignal_syscall, ksignal_sysfnmap);
