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

#include <sys/signal_num.h>
#include <sys/_sigset.h>

#define SIG_DFL ((__sighandler_t *)0)
#define SIG_IGN ((__sighandler_t *)1)
#define SIG_ERR ((__sighandler_t *)-1)
/* #define SIG_CATCH ((__sighandler_t *)2) See signalvar.h */
#define SIG_HOLD ((__sighandler_t *)3)

#ifndef _SIGSET_T_DECLARED
#define _SIGSET_T_DECLARED
typedef __sigset_t sigset_t;
#endif

union sigval {
    int sival_int;
    void * sival_ptr;
};

struct sigevent {
    int sigev_notify;                /* Notification type */
    int sigev_signo;                /* Signal number */
    union sigval sigev_value;        /* Signal value */
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

typedef struct __siginfo {
    int si_signo;                /* signal number */
    int si_errno;                /* errno association */
    /*
     * Cause of signal, one of the SI_ macros or signal-specific
     * values, i.e. one of the FPE_... values for SIGFPE.  This
     * value is equivalent to the second argument to an old-style
     * FreeBSD signal handler.
     */
    int si_code;                /* signal code */
    pid_t si_pid;                        /* sending process */
    uid_t si_uid;                        /* sender's ruid */
    int si_status;                /* exit value */
    void * si_addr;                /* faulting instruction */
    union sigval si_value;                /* signal value */
    union {
        struct {
            int _trapno; /* machine specific trap code */
        } _fault;
        struct {
            int _timerid;
            int _overrun;
        } _timer;
        struct {
            int _mqd;
        } _mesgq;
        struct {
            long _band;                /* band event for SIGPOLL */
        } _poll;                        /* was this ever used ? */
        struct {
            long __spare1__;
            int __spare2__[7];
        } __spare__;
    } _reason;
} siginfo_t;

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


int kill(pid_t pid, int sig);
/*
int killpg(pid_t pgrp, int sig);
void psiginfo(const siginfo_t * pinfo, const char * message);
void psignal(int signum, const char * message);
int pthread_kill(pthread_t thread, int sig);
int pthread_sigmask(int how, const sigset_t * restrict set,
        sigset_t * restrict oset);
*/
int raise(int sig);
/*
int sigaction(int sig, const struct sigaction * restrict act,
        struct sigaction * restrict oact);
int sigaddset(sigset_t * set, int signo);
int sigdelset(sigset_t * set, int signo);
int sigemptyset(sigset_t * set);
int sigfillset(sigset_t * set);
int sigismember(const sigset_t * set, int signo);
int sigpending(sigset_t * set);
int sigprocmask(int how, const sigset_t * restrict set,
        sigset_t * restrict oset);
int sigqueue(pid_t pid, int signo, const union sigval value);
int sigsuspend(const sigset_t * sigmask);
int sigtimedwait(const sigset_t * restrict set, siginfo_t * restrict info,
        const struct timespec * restrict timeout);
int sigwait(const sigset_t * restrict set, int * restrict sig);
int sigwaitinfo(const sigset_t * restrict set, siginfo_t *restrict info);
*/

#endif /* SIGNAL_H */

/**
  * @}
  */

/**
  * @}
  */
