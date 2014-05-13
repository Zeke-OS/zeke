/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
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
#include <syscall.h>
#include <errno.h>
#include <sched.h>
#include <timers.h>
#include <kmalloc.h>
#include <sys/sysctl.h>
#include "ksignal.h"

static int kern_logsigexit = 1;
SYSCTL_INT(_kern, KERN_LOGSIGEXIT, logsigexit, CTLFLAG_RW,
        &kern_logsigexit, 0,
        "Log processes quitting on abnormal signals to syslog(3)");

/**
 * Signal names.
 */
static char * signames[] = {
    "SIGHUP",
    "SIGINT",
    "SIGQUIT",
    "SIGILL",
    "SIGTRAP",
    "SIGABRT",
    "SIGCHLD",
    "SIGFPE",
    "SIGKILL",
    "SIGBUS",
    "SIGSEGV",
    "SIGCONT",
    "SIGPIPE",
    "SIGALRM",
    "SIGTERM",
    "SIGSTOP",
    "SIGTSTP",
    "SIGTTIN",
    "SIGTTOU",
    "SIGUSR1",
    "SIGUSR2",
    "SIGSYS",
    "SIGURG",
    "SIGINFO",
    "SIGPWR"
};

/*
 * Signal properties and actions.
 * The array below categorizes the signals and their default actions
 * according to the following properties:
 */
#define SA_KILL     0x01    /*!< Terminates process by default. */
#define SA_CORE     0x02    /*!< ditto and coredumps. */
#define SA_STOP     0x04    /*!< suspend process. */
#define SA_TTYSTOP  0x08    /*!< ditto, from tty. */
#define SA_IGNORE   0x10    /*!< ignore by default. */
#define SA_CONT     0x20    /*!< continue if suspended. */
#define SA_CANTMASK 0x40    /*!< non-maskable, catchable. */

static int sigproptbl[] = {
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

ksiginfo_t * ksiginfo_alloc(int wait)
{
    return (ksiginfo_t *)kmalloc(sizeof(ksiginfo_t));
}

void ksiginfo_free(ksiginfo_t * ksi)
{
    kfree(ksi);
}

static inline int ksiginfo_tryfree(ksiginfo_t * ksi)
{
    int retval = 0;

    if (!(ksi->ksi_flags & KSI_EXT)) {
        kfree(ksi);
        retval = 1;
        goto out;
    }

out:
    return retval;
}

void sigqueue_init(sigqueue_t * list, struct proc * p)
{
    SIGEMPTYSET(list->sq_signals);
    SIGEMPTYSET(list->sq_kill);
    TAILQ_INIT(&list->sq_list);
    list->sq_proc = p;
    list->sq_flags = SQ_INIT;
}

/* Syscall handlers ***********************************************************/

/**
 * Scheduler signal syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uintptr_t ksignal_syscall(uint32_t type, void * p)
{
    switch(type) {
        case SYSCALL_SIGNAL_KILL:
            set_errno(ENOSYS);
            return -3;

        case SYSCALL_SIGNAL_RAISE:
            set_errno(ENOSYS);
            return -4;

        case SYSCALL_SIGNAL_ACTION:
            set_errno(ENOSYS);
            return -5;

        case SYSCALL_SIGNAL_ALTSTACK:
            set_errno(ENOSYS);
            return -6;

        default:
            set_errno(ENOSYS);
            return -1;
    }
}

