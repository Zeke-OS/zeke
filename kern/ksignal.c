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

#ifdef configLOCK_DEBUG
#define ksig_lock(lock) ksig_lock_(lock, _KERROR_WHERESTR)
static int ksig_lock_(ksigmtx_t * lock, char * whr)
#else
static int ksig_lock(ksigmtx_t * lock)
#endif
{
    istate_t s;
    int retval;

#if configLOCK_DEBUG
#define _ksig_lock_(mtx)    _mtx_lock(&mtx->l, whr)
#define _ksig_trylock_(mtx) _mtx_trylock(&mtx->l, whr)
#else
#define _ksig_lock_(mtx)    mtx_lock(&mtx->l)
#define _ksig_trylock_(mtx) mtx_trylock(&mtx->l)
#endif

    s = get_interrupt_state();
    if (s & PSR_INT_I) {
        retval = _ksig_trylock_(lock);
    } else {
        retval = _ksig_lock_(lock);
    }

#if configLOCK_DEBUG
    if (retval == 0) {
        lock->l.mtx_ldebug = whr;
    }
#endif


    return retval;
}

static void ksig_unlock(ksigmtx_t * lock)
{
    istate_t s;

    mtx_unlock(&lock->l);
}

void ksignal_signals_ctor(struct signals * sigs)
{
    /* TODO Use as a ctor for sigs structs in threads and procs.
     * Also create similar function for forking and make sure that all data
     * is cloned instead of reuse.
     */
    STAILQ_INIT(&sigs->s_pendqueue);
    RB_INIT(&sigs->sa_tree);
    mtx_init(&sigs->s_lock.l, KSIG_LOCK_FLAGS);
}

static void ksignal_thread_ctor(struct thread_info * th)
{
    ksignal_signals_ctor(&th->sigs);
}
DATA_SET(thread_ctors, ksignal_thread_ctor);

void ksignal_signals_fork_reinit(struct signals * sigs)
{
    struct sigaction_tree old_tree = sigs->sa_tree;
    struct ksigaction * sigact_old;

    /*
     * Clear pending signals as required by POSIX.
     */
#if 0
    /* Not ok but can be reused for kill */
    n1  = STAILQ_FIRST(&sigs->s_pendqueue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, _entry);
        kfree(n1);
        n1 = n2;
    }
#endif
    STAILQ_INIT(&sigs->s_pendqueue);

    /*
     * Clone configured signal actions.
     */
    RB_INIT(&sigs->sa_tree);
    RB_FOREACH(sigact_old, sigaction_tree, &old_tree) {
        struct ksigaction * sigact_new = kmalloc(sizeof(struct ksigaction));

        KASSERT(sigact_new != NULL, "OOM during thread fork\n");

        memcpy(sigact_new, sigact_old, sizeof(struct ksigaction));
        RB_INSERT(sigaction_tree, &sigs->sa_tree, sigact_new);
    }

    /*
     * Reinit mutex lock.
     */
    mtx_init(&sigs->s_lock.l, KSIG_LOCK_FLAGS);
}

static void ksignal_fork_handler(struct thread_info * th)
{
    ksignal_signals_fork_reinit(&th->sigs);
}
DATA_SET(thread_fork_handlers, ksignal_fork_handler);

/**
 * Push 'src' to thread stack.
 * @param thread    thread.
 * @param src       data to be pushed.
 * @param size      size of data.
 * @param old_thread_sp[in] returns the old thread stack pointer, can be NULL.
 * @return Error code or zero.
 */
static int push_to_thread_stack(struct thread_info * thread, const void * src,
        size_t size, void ** old_thread_sp)
{
    const int insys = (thread->flags & SCHED_INSYS_FLAG) ? 1 : 0;
    const int framenum = (insys) ? SCHED_SFRAME_SVC : SCHED_SFRAME_SYS;
    sw_stack_frame_t * sframe = &thread->sframe[framenum];
    void * old_sp = (void *)sframe->sp;
    void * new_sp = old_sp -  memalign(size);
    int err;

    KASSERT(size > 0, "size should be greater than zero.\n");

    if (!old_sp || !new_sp)
        return -EFAULT;

    if (insys) {
        err = copyout(src, new_sp, size);
        if (err)
            return -EFAULT;
    } else {
        if (!kernacc(old_sp, size, VM_PROT_READ | VM_PROT_WRITE))
            return -EFAULT;

        memmove(new_sp, src, size);
    }

    sframe->sp = (uintptr_t)new_sp;
    if (old_thread_sp)
        *old_thread_sp = old_sp;

    return 0;
}

/**
 * Pop from thread stack to dest.
 */
static int pop_from_thread_stack(struct thread_info * thread, void * dest,
        size_t size)
{
    const int insys = (thread->flags & SCHED_INSYS_FLAG) ? 1 : 0;
    const int framenum = (insys) ? SCHED_SFRAME_SVC : SCHED_SFRAME_SYS;
    const void * sp = (void *)thread->sframe[framenum].sp;
    int err;

    if (!sp)
        return -EFAULT;

    if (insys) {
        err = copyin(sp, dest, size);
        if (err)
            return err;
    } else {
        if (!kernacc(sp, size, VM_PROT_READ | VM_PROT_WRITE))
            return -EFAULT;

        memmove(dest, sp, size);
    }

    thread->sframe[framenum].sp += memalign(size);

    return 0;
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
 * @return  0 = signal handled;
 *         -1 = signal can't be handled right now;
 *          1 = signal handling shall continue
 */
static int eval_inkernel_action(struct ksigaction * action)
{
    int retval = 0;

    /* Take a sig action request? */
    switch ((int)(action->ks_action.sa_handler)) {
    case (int)(SIG_DFL):
        retval = 1;
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
        return -1;
    }

    /*
     * SA_KILL should be handled before queuing.
     */
    if (action->ks_action.sa_flags & SA_KILL) {
        panic("post_scheduling can't handle SA_KILL");
    }

    return retval;
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
    void * old_thread_sp; /* Note: in user space */
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
            continue;
        }

        /* Check if the thread is waiting for this signal */
        if (blocked && swait) {
            current_thread->sigwait_retval = signum;
            sigemptyset(&sigs->s_wait);
            STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);
            ksig_unlock(&sigs->s_lock);
            kfree(ksiginfo);
            return; /* There is a sigwait() for this signum. */
        }

        /* Check if signal is blocked */
        if (blocked) {
            continue; /* This signal is currently blocked. */
        }

        int nxt_state;
        nxt_state = eval_inkernel_action(&action);
        if (nxt_state == 0) {
            /* Handling done */
            STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);
            ksig_unlock(&sigs->s_lock);
            kfree(ksiginfo);
            return;
        } else if (nxt_state < 0) {
            /* This signal can't be handled right now */
            continue;
        }
        break;
    }
    if (!ksiginfo) {
        ksig_unlock(&sigs->s_lock);
        return; /* All signals blocked or no signals pending */
    }
    /* Else the pending singal should be handled now. */
    STAILQ_REMOVE(&sigs->s_pendqueue, ksiginfo, ksiginfo, _entry);

    /*
     * Continue to handle the signal in user space handler.
     */

    if (/* Push current stack frame to the user space thread stack. */
        push_to_thread_stack(current_thread,
                             &current_thread->sframe[SCHED_SFRAME_SYS],
                             sizeof(sw_stack_frame_t),
                             NULL) ||
        /* Push siginfo struct. */
        push_to_thread_stack(current_thread,
                             &ksiginfo->siginfo,
                             sizeof(ksiginfo->siginfo),
                             &old_thread_sp /* Address of the prev sframe. */)
       ) {
        /*
         * Thread has trashed its stack; Nothing we can do but give SIGILL.
         * TODO Should we punish only the thread or whole process?
         */
        ksignal_sendsig_fatal(curproc, SIGILL);
        ksig_unlock(&sigs->s_lock);
        return; /* TODO Is this ok? */
    }
    /* Don't push anything after this point. */

    /*
     * Update next stack frame.
     */
    next_frame = &current_thread->sframe[SCHED_SFRAME_SYS];
    next_frame->pc = (uintptr_t)action.ks_action.sa_sigaction;
    next_frame->r0 = signum;                    /* arg1 = signum */
    next_frame->r1 = next_frame->sp;            /* arg2 = siginfo */
    next_frame->r2 = 0;                         /* arg3 = TODO context */
    next_frame->r9 = (uintptr_t)old_thread_sp;  /* frame pointer */
    next_frame->lr = sigs->s_usigret;

    /*
     * TODO
     * - Check current_thread sigs
     *  -- Change to alt stack if requested
     */

    ksig_unlock(&sigs->s_lock);
    kfree(ksiginfo);
}
DATA_SET(post_sched_tasks, ksignal_post_scheduling);

int ksignal_queue_sig(struct signals * sigs, int signum, int si_code)
{
    int retval = 0;
    struct ksigaction action;
    struct ksiginfo * ksiginfo;

    KASSERT(mtx_test(&sigs->s_lock.l), "sigs should be locked\n");

    if (signum <= 0 || signum > _SIG_MAXSIG) {
        return -EINVAL;
    }

    retval = sigismember(&sigs->s_running, signum);
    if (retval) /* Already running a handler. */
        return 0;

    /* Get action struct for this signal. */
    ksignal_get_ksigaction(&action, sigs, signum);

    /* Ignored? */
    if (action.ks_action.sa_handler == SIG_IGN)
        return 0;

    /*
     * SA_KILL is handled here because post_scheduling hander can't change
     * next thread.
     */
    if (action.ks_action.sa_flags & SA_KILL) {
        /* TODO not always curproc? */
        thread_terminate(curproc->main_thread->id);

        return 0;
    }

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

    return 0;
}

int ksignal_sendsig_fatal(struct proc_info * p, int signum)
{
    struct signals * sigs = &p->sigs;
    struct ksigaction act;
    int err;

    if (ksig_lock(&sigs->s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    /* Change signal action to default to make this signal fatal. */
    err = ksignal_reset_ksigaction(sigs, signum);
    if (err)
        return err;
    ksignal_get_ksigaction(&act, sigs, signum);
    if (!(act.ks_action.sa_flags & SA_KILL)) {
        char msg[80];
        ksprintf(msg, sizeof(msg), "%d requested a fatal signal for %d"
                 "but dfl action for signum %d is not SA_KILL\n",
                 curproc->pid, p->pid, signum);
        KERROR(KERROR_WARN, msg);
    }

    err = ksignal_queue_sig(sigs, signum, SI_UNKNOWN);

    ksig_unlock(&sigs->s_lock);

    return err;
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

int ksignal_reset_ksigaction(struct signals * sigs, int signum)
{
    struct ksigaction filt = { .ks_signum = signum };
    struct ksigaction * p_action;

    if (signum < 0 || signum >= num_elem(default_sigproptbl)) {
        return -EINVAL;
    }

    KASSERT(mtx_test(&sigs->s_lock.l), "sigs should be locked\n");

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, &filt))) {
        if (RB_REMOVE(sigaction_tree, &sigs->sa_tree, p_action)) {
            kfree(p_action);
        } else {
            panic("Can't remove an entry from sigaction_tree\n");
        }
    }

    return 0;
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

    proc = proc_get_struct_l(args.pid);
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

    proc = proc_get_struct_l(thread->pid_owner);
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

    if (ksig_lock(&sigs->s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    err = ksignal_queue_sig(sigs, args.sig, SI_USER);
    if (err) {
        ksig_unlock(&sigs->s_lock);
        set_errno(-err);
        return -1;
    }
    KSIG_EXEC_IF(thread, args.sig);
    ksig_unlock(&sigs->s_lock);

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
    const sw_stack_frame_t * const sframe =
        &current_thread->sframe[SCHED_SFRAME_SYS];
    int err;

    /*
     * TODO
     * Return from signal handler
     * - get address of the old frame pointer
     * - revert stack frame and alt stack
     * - reset pc and oth registers
     */

    current_thread->sframe[SCHED_SFRAME_SYS].sp = sframe->r9;
    err = pop_from_thread_stack(current_thread,
                                &current_thread->sframe[SCHED_SFRAME_SYS],
                                sizeof(const sw_stack_frame_t));
    if (err) {
        /*
         * TODO Should we punish only the thread or whole process?
         */
        ksignal_sendsig_fatal(curproc, SIGILL);
        while (1) {
            sched_sleep_current_thread(0); /* TODO ?? */
            /* Should not return to here */
        }
    }

    /*
     * We return for now but the actual return from this system call will
     * happen to the place that was originally interrupted by a signal.
     */
    return 0;
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
