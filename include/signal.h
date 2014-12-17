/**
 *******************************************************************************
 * @file    signal.h
 * @author  Olli Vanhoja
 * @brief   Signals.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *******************************************************************************
 */

/** @addtogroup LIBC
  * @{
  */

#pragma once
#ifndef SIGNAL_H
#define SIGNAL_H

#include <time.h> /* TODO Header shall define the timespec structure as
                   *      described in time.h */

/** @addtogroup Signals
  * @{
  */

#include <sys/types_pthread.h>
#include <sys/signal_num.h>
#include <sys/_sigset.h>

/*
 * Signal properties and actions, acording to IEEE Std 1003.1, 2004 Edition
 */
#ifdef KERNEL_INTERNAL
#define SA_KILL         0x000001 /*!< Terminates process by default. */
#define SA_CORE         0x000002 /*!< ditto and coredumps. */
#define SA_STOP         0x000004 /*!< suspend process. */
#define SA_TTYSTOP      0x000008 /*!< ditto, from tty. */
#define SA_IGNORE       0x000010 /*!< ignore by default. */
#define SA_CONT         0x000020 /*!< continue if suspended. */
#define SA_CANTMASK     0x000040 /*!< non-maskable, catchable. */
#endif /* KERNEL_INTERNAL */
#define SA_NOCLDSTOP    0x000100 /*!< Do not generate SIGCHLD when children stop
                                  * or stopped children continue. */
#define SA_ONSTACK      0x000200 /*!< Causes signal delivery to occur on
                                  * an alternate stack.
                                  */
#define SA_RESETHAND    0x000400 /*!< Causes signal dispositions to be set to
                                  * SIG_DFL on entry to signal handlers.
                                  */
#define SA_RESTART      0x000800 /*!< Causes certain functions to become
                                  * restartable.
                                  */
#define SA_SIGINFO      0x001000 /*!< Causes extra information to be passed to
                                  * signal handlers at the time of receipt of
                                  * a signal.
                                  */
#define SA_NOCLDWAIT    0x002000 /*!< Causes implementations not to create
                                  * zombie processes on child death.
                                  */
#define SA_NODEFER      0x004000 /*!< Causes signal not to be automatically
                                  * blocked on entry to signal handler.
                                  */

#define SIG_BLOCK       0x000001 /*!< The resulting set is the union of
                                  * the current set and the signal set
                                  * pointed to by the argument set.
                                  */
#define SIG_UNBLOCK     0x000002 /*!< The resulting set is the intersection of
                                  * the current set and the complement of the
                                  * signal set pointed to by the argument set.
                                  */
#define SIG_SETMASK     0x000004 /*!< The resulting set is the signal set pointed
                                  * to by the argument set.
                                  */

#define SS_ONSTACK      0x000008 /*!< Process is executing on an alternate
                                  * signal stack.
                                  */
#define SS_DISABLE      0x000010 /*!< Alternate signal stack is disabled. */

#define MINSIGSTKSZ     1024     /*!< Minimum stack size for a signal handler.
                                  */
#define SIGSTKSZ        4096     /*!< Default size in bytes for
                                  *  the alternate signal stack.
                                  */

/* Requests, sa_handler values. */
#define SIG_DFL ((__sighandler_t *)0)  /*!< Request for default signal
                                        * handling. */
#define SIG_IGN ((__sighandler_t *)1)  /*!< Return value from signal() in case
                                        * of error. */
#define SIG_ERR ((__sighandler_t *)-1) /*!< Request that signal be held. */
#define SIG_HOLD ((__sighandler_t *)3) /*!< Request that signal be ignored. */

#ifndef _SIGSET_T_DECLARED
#define _SIGSET_T_DECLARED
typedef __sigset_t sigset_t;
#endif

union sigval {
    int sival_int;
    void * sival_ptr;
};

struct sigevent {
    int sigev_notify;           /* Notification type */
    int sigev_signo;            /* Signal number */
    union sigval sigev_value;   /* Signal value */
    union {
        //__lwpid_t        _threadid;
        struct {
            void (*_function)(union sigval);
                void *_attribute; /* pthread_attr_t * */
        } _sigev_thread;
        unsigned short _kevent_flags;
        long __spare__[8];
    } _sigev_un;
};

/* Signal codes */
#define SI_UNKNOWN  0 /*!< The signal source is unknown. */
#define SI_USER     1 /*!< The signal was send from a user thread. */
#define SI_QUEUE    2 /*!< The signal was sent by the sigqueue() function. */
#define SI_TIMER    3 /*!< The signal was generated by the expiration of
                       *   a timer set by timer_settime(). */
#define SI_ASYNCIO  4 /*!< The signal was generated by the completion of
                       *   an asynchronous I/O request. */


typedef struct __siginfo {
    int si_signo;           /*!< signal number. */
    int si_code;            /*!< signal code. */
    int si_errno;           /*!< errno association. */
    pid_t si_pid;           /*!< sending process. */
    uid_t si_uid;           /*!< sender's ruid. */
    void * si_addr;         /*!< faulting instruction. */
    int si_status;          /*!< Exit value or signal. */
    union sigval si_value;  /*!< Signal value. */
} siginfo_t;

struct sigaction {
    /**
     * Additional set of signals to be blocked during execution
     * of signal-catching function.
     */
    sigset_t sa_mask;

    /**
     * Special flags to affect behavior of signal.
     */
    int sa_flags;

    /**
     * Pointer to a signal-catching function.
     */
    union {
        void (*sa_handler)(int); /*!< fn pointer or SIG_IGN or SIG_DFL. */
        void (*sa_sigaction)(int, siginfo_t *, void *);
    };
};

/*
 * Type of a signal handling function.
 *
 * Language spec sez signal handlers take exactly one arg, even though we
 * actually supply three.  Ugh!
 *
 * We don't try to hide the difference by leaving out the args because
 * that would cause warnings about conformant programs.  Nonconformant
 * programs can avoid the warnings by casting to (__sighandler_t *) or
 * sig_t before calling signal() or assigning to sa_handler or sv_handler.
 *
 * The kernel should reverse the cast before calling the function.  It
 * has no way to do this, but on most machines 1-arg and 3-arg functions
 * have the same calling protocol so there is no problem in practice.
 * A bit in sa_flags could be used to specify the number of args.
 */
typedef void __sighandler_t(int);
typedef __sighandler_t * sig_t;
typedef void __siginfohandler_t(int, struct __siginfo *, void *);

typedef struct __stack {
    void * ss_sp;   /*!< Stack base or pointer. */
    size_t ss_size; /*!< Stack size. */
    int ss_flags;   /*!< Flags. */
} stack_t;

/**
 * Machine-specific context.
 */
typedef struct __mcontext {
    size_t __not_used[17];
} mcontext_t;

/**
 * User context.
 */
typedef struct __ucontext {
    struct __ucontext * uc_link;    /*!< Pointer to the context that is resumed
                                     *   when this context returns. */
    sigset_t uc_sigmask;    /*!< The set of signals that are blocked when this
                             *   context is active. */
    stack_t uc_stack;       /*!< The stack used by this context. */
    mcontext_t uc_mcontext; /*!< A machine-specific representation of the saved
                             *   context. */
} ucontext_t;

/**
 * Arguments struct for SYSCALL_SIGNAL_PKILL
 */
struct _pkill_args {
    pid_t pid;
    int sig;
};

/**
 * Arguments struct for SYSCALL_SIGNAL_TKILL
 */
struct _tkill_args {
    pthread_t thread_id;
    int sig;
};

/**
 * Arguments for SYSCAL_SIGNAL_SIGNAL
 */
struct _signal_signal_args {
    int signum;
    void (*handler)(int);
};

/**
 * Arguments for SYSCALL_SIGNAL_ACTION
 */
struct _signal_action_args {
    int signum;
    struct sigaction action;
};

/**
 * Arguments for SYSCALL_SIGNAL_SIGMASK
 */
struct _signal_sigmask_args {
    int threadmask; /*!< 0 = process mask: 1 = thread mask */
    int how;
    const sigset_t * restrict set;
    const sigset_t * restrict oset;
};

/**
 * Arguments for SYSCALL_SIGNAL_SIGWAIT
 */
struct _signal_sigwait_args {
    const sigset_t * restrict set;
    int * restrict sig;
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

void (*signal(int sig, void (*func)(int)))(int);

int kill(pid_t pid, int sig);
/*
int killpg(pid_t pgrp, int sig);
void psiginfo(const siginfo_t * pinfo, const char * message);
*/
void psignal(int signum, const char * message);

int pthread_kill(pthread_t thread, int sig);
/*
int pthread_sigmask(int how, const sigset_t * restrict set,
        sigset_t * restrict oset);
*/
int raise(int sig);
/*
int sigaction(int sig, const struct sigaction * restrict act,
        struct sigaction * restrict oact);
*/

/**
 * @addtogroup sigsetops
 * sigsetops
 * Manipulate signal sets.
 *
 * These functions manipulate   signal sets stored in a sigset_t.  Either
 * sigemptyset() or sigfillset() must  be called for every object of type
 * sigset_t before any other use of the object.
 *
 * @return  The sigismember() function   returns 1 if the signal is a member
 *          of the set, 0 otherwise.   The other functions return 0 upon
 *          success.  A -1 return value indicates an error occurred and
 *          the global variable errno is set to indicate the reason.
 * @{
 */

/**
 * The sigaddset() function adds the specified signal   signo to
 * the signal set.
 */
int sigaddset(sigset_t * set, int signo);

/**
 * The sigdelset() function deletes the specified signal signo from
 * the signal set.
 */
int sigdelset(sigset_t * set, int signo);

/**
 * The sigemptyset() function   initializes a signal set to be empty.
 */
int sigemptyset(sigset_t * set);

/**
 * The sigfillset() function initializes a signal set   to contain all
 * signals.
 */
int sigfillset(sigset_t * set);

/**
 * The sigismember() function   returns whether a specified signal signo is
 * contained in the signal set.
 */
int sigismember(const sigset_t * set, int signo);

/**
 * @}
 */

/*
int sigpending(sigset_t * set);
*/
int sigprocmask(int how, const sigset_t * restrict set,
                sigset_t * restrict oset);
/*
int sigqueue(pid_t pid, int signo, const union sigval value);
int sigsuspend(const sigset_t * sigmask);
*/
int sigtimedwait(const sigset_t * restrict set, siginfo_t * restrict info,
        const struct timespec * restrict timeout);
int sigwait(const sigset_t * restrict set, int * restrict sig);
/*
int sigwaitinfo(const sigset_t * restrict set, siginfo_t *restrict info);
*/

/**
 * Return from a signal handler.
 */
void sigreturn(void);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* SIGNAL_H */

/**
  * @}
  */

/**
  * @}
  */
