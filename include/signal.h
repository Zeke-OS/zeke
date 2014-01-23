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
 *******************************************************************************
 */

/** @addtogroup Library_Functions
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

#include <signal_num.h>

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
