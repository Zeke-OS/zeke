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
#include <tsched.h>
#include <timers.h>
#include <kmalloc.h>
#include <kstring.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include "ksignal.h"

static int kern_logsigexit = 1;
SYSCTL_INT(_kern, KERN_LOGSIGEXIT, logsigexit, CTLFLAG_RW,
           &kern_logsigexit, 0,
           "Log processes quitting on abnormal signals to syslog(3)");

/*
 * Signal default actions.
 */
static const uint8_t default_sigproptbl[] = {
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

     return a->signum - b->signum;
}

static void ksignal_thread_ctor(struct thread_info * th)
{
    RB_INIT(&th->sigs.sa_tree);
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
}
DATA_SET(thread_fork_handlers, ksignal_fork_handler);

/* Syscall handlers ***********************************************************/

static const syscall_handler_t ksignal_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_KILL, NULL),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_RAISE, NULL),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ACTION, NULL),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ALTSTACK, NULL),
};
SYSCALL_HANDLERDEF(ksignal_syscall, ksignal_sysfnmap);
