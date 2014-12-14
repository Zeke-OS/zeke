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
#include <kstring.h>
#include <libkern.h>
#include <syscall.h>
#include <thread.h>
#include <tsched.h>
#include <proc.h>
#include <kmalloc.h>
#include <vm/vm.h>
#include <timers.h>
#include <errno.h>
#include <sys/priv.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include "ksignal.h"

#define KSIG_LOCK_FLAGS (MTX_TYPE_TICKET | MTX_TYPE_SLEEP | MTX_TYPE_PRICEIL)

#define KSIG_EXEC_IF(thread_, signum_) do {                     \
    int blocked = ksignal_isblocked(&thread_->sigs, signum_);   \
    int swait = sigismember(&thread_->sigs.s_wait, signum_);    \
    if (blocked && swait) thread_release(thread_);              \
    else if (!blocked) sched_thread_set_exec(thread_->id);      \
} while (0)

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

/**
 * Signum comparator for rb trees.
 */
int signum_comp(struct ksigaction * a, struct ksigaction * b)
{
    KASSERT((a && b), "a & b must be set");

     return a->ks_signum - b->ks_signum;
}

static int ksig_lock(ksigmtx_t * lock)
{
    istate_t s;

    s = get_interrupt_state();
    if (s & PSR_INT_I) {
        return mtx_trylock(&lock->l);
    } else {
        return mtx_lock(&lock->l);
    }
}

static void ksig_unlock(ksigmtx_t * lock)
{
    istate_t s;

    s = get_interrupt_state();
    if (s & PSR_INT_I)
        return;

    mtx_unlock(&lock->l);
}

static void ksignal_thread_ctor(struct thread_info * th)
{
    struct signals * sigs = &th->sigs;

    STAILQ_INIT(&sigs->s_pendqueue);
    RB_INIT(&sigs->sa_tree);
    mtx_init(&sigs->s_lock.l, KSIG_LOCK_FLAGS);
}
DATA_SET(thread_ctors, ksignal_thread_ctor);

static void ksignal_fork_handler(struct thread_info * th)
{
    struct sigaction_tree old_tree = th->sigs.sa_tree;
    struct ksigaction * sigact_old;
    struct ksiginfo * n1;
    struct ksiginfo * n2;

    /* Clear pending signals as required by POSIX. */
    n1  = STAILQ_FIRST(&th->sigs.s_pendqueue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, _entry);
        kfree(n1);
        n1 = n2;
    }
    STAILQ_INIT(&th->sigs.s_pendqueue);

    /* Clone configured signal actions. */
    RB_INIT(&th->sigs.sa_tree);
    RB_FOREACH(sigact_old, sigaction_tree, &old_tree) {
        struct ksigaction * sigact_new = kmalloc(sizeof(struct ksigaction));

        KASSERT(sigact_new != NULL, "OOM during thread fork\n");

        memcpy(sigact_new, sigact_old, sizeof(struct ksigaction));
        RB_INSERT(sigaction_tree, &th->sigs.sa_tree, sigact_new);
    }

    /* Reinit mutex lock */
    mtx_init(&th->sigs.s_lock.l, KSIG_LOCK_FLAGS);
}
DATA_SET(thread_fork_handlers, ksignal_fork_handler);

/**
 * Allocate memory from the user stack of a thread.
 * @note Works only for any thread of the current process.
 * @param thread is the thread owning the stack.
 * @param len is the length of the allocation.
 * @return Returns pointer to the uaddr of the allocation if succeed;
 * Otherwise NULL.
 */
static void * usr_stack_alloc(struct thread_info * thread,
        uintptr_t * old_frame, size_t len)
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
    *old_frame = frame->sp;
    frame->sp -= memalign(len);

    return sp;
}

/**
 * Forward signals pending in proc sigs struct.
 */
static void forward_proc_signals(void)
{
    struct signals * sigs = &curproc->sigs;
    struct ksiginfo * ksiginfo;
    struct ksiginfo * tmp;

    if (ksig_lock(&sigs->s_lock))
        return;

    /* Get next pending signal. */
    STAILQ_FOREACH_SAFE(ksiginfo, &sigs->s_pendqueue, _entry, tmp) {
        struct thread_info * thread;
        struct thread_info * thread_it = NULL;

        while ((thread = proc_iterate_threads(curproc, &thread_it))) {
            int signum;
            int blocked, swait;

            /*
             * Check if signal is not blocked for the thread.
             */
            if (ksig_lock(&thread->sigs.s_lock)) {
                ksig_unlock(&sigs->s_lock);
                return; /* Try again later */
            }
            signum = ksiginfo->siginfo.si_signo;
            blocked = ksignal_isblocked(&thread->sigs, signum);
            swait = sigismember(&thread->sigs.s_wait, signum);

            if (!(blocked && swait) && blocked) {
                ksig_unlock(&thread->sigs.s_lock);
                continue; /* check next thread */
            }

            STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);
            STAILQ_INSERT_TAIL(&thread->sigs.s_pendqueue, ksiginfo, _entry);
            if (thread != current_thread)
                KSIG_EXEC_IF(thread, ksiginfo->siginfo.si_signo);
            ksig_unlock(&thread->sigs.s_lock);
            ksig_unlock(&sigs->s_lock);
            return; /* Safer than break? */
        }
    }

    ksig_unlock(&sigs->s_lock);
}

/**
 * Post thread scheduling handler that updates thread stack frame if a signal
 * is pending. After this handler the thread will enter to signal handler
 * instead of returning to normal execution.
 */
static void ksignal_post_scheduling(void)
{
    int signum, blocked, swait;
    struct signals * sigs = &current_thread->sigs;
    struct ksigaction action;
    struct ksiginfo * ksiginfo;
    siginfo_t * usiginfo; /* Note: in user space */
    uintptr_t old_uframe;
    sw_stack_frame_t * usave_frame;
    sw_stack_frame_t * next_frame;

    forward_proc_signals();

    /*
     * Can't handle signals now as the thread is in syscall and we don't want to
     * export data from kernel registers to the user space stack.
     */
    if (current_thread->flags & SCHED_INSYS_FLAG)
        return;

    /*
     * Can't handle signals right now if we can't get lock to sigs of
     * the current thread.
     */
    if (ksig_lock(&sigs->s_lock))
        return;

    /* Get next pending signal. */
    STAILQ_FOREACH(ksiginfo, &sigs->s_pendqueue, _entry) {
        signum = ksiginfo->siginfo.si_signo;
        blocked = ksignal_isblocked(sigs, signum);
        swait = sigismember(&sigs->s_wait, signum);

        /* Signal pending let's check if we should handle it now. */
        ksignal_get_ksigaction(&action, sigs, signum);
        if (sigismember(&sigs->s_running, signum)) {
            /* Already running a handler for that signum */
            sigdelset(&sigs->s_running, signum);
            ksig_unlock(&sigs->s_lock);
            continue;
        }

        /* TODO check handling policy and kill now if requested. */

        /* Check if the thread is waiting for this signal */
        if (blocked && swait) {
            current_thread->sigwait_retval = signum;
            sigemptyset(&sigs->s_wait);
            STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);
            kfree(ksiginfo);
            ksig_unlock(&sigs->s_lock);
            return; /* There is a sigwait() for this signum. */
        }

        /* Check if signal is blocked */
        if (blocked) {
            ksig_unlock(&sigs->s_lock);
            continue; /* This signal is currently blocked. */
        }

        /* Take a sig action request? */
        switch ((int)(action.ks_action.sa_handler)) {
        case (int)(SIG_DFL):
            break;
        case (int)(SIG_IGN):
            /*
             * TODO
             * - Decide if we want to continue and/or remove this
             *   signal from pend in some of these cases?
             * - Any other differences between IGN, ERR and HOLD?
             */
        case (int)(SIG_ERR):
        case (int)(SIG_HOLD):
            ksig_unlock(&sigs->s_lock);
            goto next;
        }

        break;
next:
        continue;
    }
    if (!ksiginfo) {
        ksig_unlock(&sigs->s_lock);
        return; /* All signals blocked or no signals pending */
    }
    /* Else the pending singal should be handled now. */
    STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);

    /* TODO apply sa_mask?? */

    /*
     * TODO Take sa_flag actions requested provided in action.ks_action.sa_flags
     */

    /* Allocate memory from user thread stack for stack frame and siginfo */
    usave_frame = usr_stack_alloc(current_thread, &old_uframe, USAVEFRAME_SIZE);
    if (!usave_frame) {
        /*
         * Thread has trashed its stack; Nothing we can do but
         * give SIGILL.
         */
        ksignal_sendsig_fatal(current_thread->id, SIGILL);
        ksig_unlock(&sigs->s_lock);
        return; /* TODO Is this ok? */
    }

    /* Save stack frame to the user space stack */
    next_frame = &current_thread->sframe[SCHED_SFRAME_SYS];
    (void)copyout(next_frame, usave_frame, sizeof(sw_stack_frame_t));

    /* Copyout siginfo struct. */
    usiginfo = (void *)usave_frame + sizeof(sw_stack_frame_t);
    (void)copyout(&ksiginfo->siginfo, usiginfo, sizeof(siginfo_t));

    /*
     * Set next frame
     */
    next_frame->pc = (uintptr_t)action.ks_action.sa_sigaction;
    next_frame->r0 = signum;                /* arg1 = signum */
    next_frame->r1 = (uintptr_t)usiginfo;   /* arg2 = siginfo */
    next_frame->r2 = 0;                     /* arg3 = TODO context */
    next_frame->r9 = old_uframe;            /* frame pointer */
    next_frame->lr = sigs->s_usigret;

    /*
     * TODO
     * - Check current_thread sigs
     *  -- Change return address to our own handler to call syscall that
     *     reverts signal handling state
     *  -- Change to alt stack if requested
     * - or handle signal in kernel
     */

    ksig_unlock(&sigs->s_lock);
    kfree(ksiginfo);
}
DATA_SET(post_sched_tasks, ksignal_post_scheduling);

/* TODO use signal queue and prepare siginfo struct to be delivered. */
int ksignal_queue_sig(struct signals * sigs, int signum, int si_code)
{
    int retval = 0;
    struct ksigaction action;
    struct ksiginfo * ksiginfo;

    if (signum <= 0 || signum > _SIG_MAXSIG) {
        return -EINVAL;
    }

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
    ksiginfo = kmalloc(sizeof(struct ksiginfo));
    if (!ksiginfo)
        return -ENOMEM;
    *ksiginfo = (struct ksiginfo){
        .siginfo.si_signo = signum,
        .siginfo.si_code = si_code,
        .siginfo.si_errno = 0, /* TODO */
        .siginfo.si_pid = current_process_id,
        .siginfo.si_uid = curproc->uid,
        .siginfo.si_addr = 0, /* TODO */
        .siginfo.si_status = 0, /* TODO */
        .siginfo.si_value = { 0 }, /* TODO */
    };
    STAILQ_INSERT_TAIL(&sigs->s_pendqueue, ksiginfo, _entry);

out:
    ksig_unlock(&sigs->s_lock);

    return 0;
}

/**
 * Kill process by a fatal signal that can't be blocked.
 */
int ksignal_sendsig_fatal(pthread_t thid, int signum)
{
    /*
     * TODO Implementation
     */
    return 0;
}

int ksignal_isblocked(struct signals * sigs, int signum)
{
    KASSERT(mtx_test(&sigs->s_lock.l), "sigs should be locked\n");

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
    KASSERT(mtx_test(&sigs->s_lock.l), "sigs should be locked\n");

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, &find))) {
        memcpy(action, p_action, sizeof(struct ksigaction));
        return;
    }

    action->ks_signum = signum;
    sigemptyset(&action->ks_action.sa_mask);
    action->ks_action.sa_flags = (signum < num_elem(default_sigproptbl)) ?
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

    KASSERT(mtx_test(&sigs->s_lock.l), "sigs should be locked\n");

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
    struct signals * sigs;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    /* TODO if pid == 0 send signal to all procs */

    proc = proc_get_struct(args.pid);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }
    sigs = &proc->sigs;

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

    if (ksig_lock(&sigs->s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    ksignal_queue_sig(sigs, args.sig, SI_USER);

    ksig_unlock(&sigs->s_lock);

    forward_proc_signals();

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
    struct signals * sigs;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    /* TODO if thread_id == 0 then send to all threads */

    thread = sched_get_thread_info(args.thread_id);
    if (!thread) {
        set_errno(ESRCH);
        return -1;
    }
    sigs = &thread->sigs;

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

    err = ksignal_queue_sig(sigs, args.sig, SI_USER);
    if (err) {
        set_errno(-err);
        return -1;
    }

    KSIG_EXEC_IF(thread, args.sig);

    return 0;
}

static int sys_signal_signal(void * user_args)
{
    struct _signal_signal_args args;
    struct ksigaction action;
    void * old_handler;
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

    if (ksig_lock(&current_thread->sigs.s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    /* Get current sigaction */
    ksignal_get_ksigaction(&action, &current_thread->sigs, args.signum);

    /* Swap handler pointers */
    old_handler = action.ks_action.sa_handler;
    action.ks_action.sa_handler = args.handler;
    args.handler = old_handler;

    /* Set new handler */
    err = ksignal_set_ksigaction(&current_thread->sigs, &action);

    ksig_unlock(&current_thread->sigs.s_lock);
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = copyout(&args, user_args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
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

    if (ksig_lock(&current_thread->sigs.s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    err = ksignal_set_ksigaction(
            &current_thread->sigs,
            &(struct ksigaction){
                .ks_signum = args.signum,
                .ks_action = args.action
            });
    ksig_unlock(&current_thread->sigs.s_lock);
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_signal_altstack(void * user_args)
{
    /*
     * TODO Implement altstack syscall that can be used to set alternative
     *      user stack for signal handlers.
     */
    set_errno(ENOTSUP);
    return -1;
}

/**
 * Examine and change blocked signals of the thread or the current process.
 */
static int sys_signal_sigmask(void * user_args)
{
    struct _signal_sigmask_args args;
    sigset_t set;
    sigset_t * current_set;
    ksigmtx_t * s_lock;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }

    if (args.oset) {
        /* Copy current set to usr oset. */
        copyout(&current_thread->sigs.s_block, (void *)args.oset,
                sizeof(struct _signal_sigmask_args));
        if (err) {
            set_errno(-err);
            return -1;
        }
    }

    /* If 'set' is null we can return now. */
    if (!args.set) {
        return 0;
    }

    err = copyin(args.set, &set, sizeof(sigset_t));
    if (err) {
        set_errno(-err);
        return -1;
    }

    /* Select current set */
    if (args.threadmask) {
        /* current thread. */
        current_set = &current_thread->sigs.s_block;
        s_lock = &current_thread->sigs.s_lock;
    } else {
        /* current process. */
        current_set = &curproc->sigs.s_block;
        s_lock = &curproc->sigs.s_lock;
    }

    if (ksig_lock(s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    /* Change ops. */
    switch (args.how) {
    case SIG_BLOCK:
        /*
         * The resulting set is the union of the current set and the signal set
         * pointed by 'set'
         */
        sigunion(current_set, current_set, &set);
        break;
    case SIG_SETMASK:
        /*
         * The resulting set is the signal set pointed by 'set'.
         */
        memcpy(current_set, &set, sizeof(sigset_t));
        break;
    case SIG_UNBLOCK:
        /*
         * The resulting set is the intersection of the current set and
         * the complement of the signal set pointed by 'set'.
         */
        sigintersect(current_set, current_set, sigcompl(&set, &set));
        break;
    default:
        /*
         * Invalid 'how' value.
         */
        ksig_unlock(s_lock);
        set_errno(EINVAL);
        return -1;
    }

    ksig_unlock(s_lock);

    return 0;
}

static int sys_signal_sigwait(void * user_args)
{
    struct _signal_sigwait_args args;
    sigset_t set;
    struct signals * sigs = &current_thread->sigs;
    ksigmtx_t * s_lock = &sigs->s_lock;
    struct ksiginfo * ksiginfo;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }
    err = copyin(args.set, &set, sizeof(set));
    if (err) {
        set_errno(-err);
        return -1;
    }

    memcpy(&current_thread->sigs.s_wait, &set,
           sizeof(current_thread->sigs.s_wait));
    forward_proc_signals();

    if (ksig_lock(s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    /* Iterate through pending signals */
    STAILQ_FOREACH(ksiginfo, &sigs->s_pendqueue, _entry) {
        if (sigismember(&set, ksiginfo->siginfo.si_signo)) {
            current_thread->sigwait_retval = ksiginfo->siginfo.si_signo;
            STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);
            sigemptyset(&current_thread->sigs.s_wait);
            ksig_unlock(s_lock);
            goto out;
        }
    }

    ksig_unlock(s_lock);
    thread_wait();

out:
    err = copyout(&current_thread->sigwait_retval, args.sig, sizeof(int));
    if (err) {
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static int sys_signal_return(void * user_args)
{
    /*
     * TODO
     * Return from signal handler
     * - get address of the old frame pointer
     * - revert stack frame and alt stack
     * - reset pc and oth registers
     */

    set_errno(ENOTSUP);
    return -1;
}

static const syscall_handler_t ksignal_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_PKILL,      sys_signal_pkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_TKILL,      sys_signal_tkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGNAL,     sys_signal_signal),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ACTION,     sys_signal_action),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ALTSTACK,   sys_signal_altstack),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGMASK,    sys_signal_sigmask),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGWAIT,    sys_signal_sigwait),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_RETURN,     sys_signal_return),
};
SYSCALL_HANDLERDEF(ksignal_syscall, ksignal_sysfnmap);
