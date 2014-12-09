/**
 *******************************************************************************
 * @file    ksignal.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for thread Signal Management in kernel (ksignal.c).
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#ifndef KSIGNAL_H
#define KSIGNAL_H

#include <sys/queue.h>
#include <sys/tree.h>
#include <stdint.h>
#include <klocks.h>
#include <signal.h>

struct ksigaction {
    int ks_signum;
    struct sigaction ks_action;
    RB_ENTRY(ksigaction) _entry; /* Should be the last entry. */
};

STAILQ_HEAD(sigwait_queue, __siginfo);
RB_HEAD(sigaction_tree, ksigaction);

/**
 * Thread signals struct.
 */
struct signals {
    sigset_t s_block;       /*!< List of blocked signals. */
    sigset_t s_wait;        /*!< Signal wait mask. */
    sigset_t s_pending;     /*!< Signals pending for handling. */
    sigset_t s_running;     /*!< Signals running mask. */
    /* TODO struct sigwait_queue s_waitqueue; */
    struct sigaction_tree sa_tree;
    uintptr_t s_usigret;  /*!< Address of the sigret() function in uspace. */
    mtx_t s_lock;
};

struct thread_info;

RB_PROTOTYPE(sigaction_tree, ksigaction, _entry, signum_comp);
int signum_comp(struct ksigaction * a, struct ksigaction * b);

int ksignal_thread_sendsig(struct thread_info * thread, int signum);
int ksignal_sendsig_fatal(pthread_t thid, int signum);
int ksignal_isblocked(struct signals * sigs, int signum);
void ksignal_get_ksigaction(struct ksigaction * action,
                            struct signals * sigs, int signum);
int ksignal_set_ksigaction(struct signals * sigs, struct ksigaction * action);

int sigaddset(sigset_t * set, int signo);
int sigdelset(sigset_t * set, int signo);
int sigemptyset(sigset_t * set);
int sigfillset(sigset_t * set);
int sigismember(const sigset_t * set, int signo);
int sigisemptyset(const sigset_t * set);
int sigffs(sigset_t * set);

#endif /* KSIGNAL_H */

