/**
 *******************************************************************************
 * @file    wait.h
 * @author  Olli Vanhoja
 * @brief   Declarations for waiting.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup libc
 * @{
 */

#ifndef WAIT_H
#define WAIT_H

#include <sys/types/_pid_t.h>

#define WCONTINUED  0x1 /*!< Report a continued process. */
#define WNOHANG     0x2 /*!< Don't hang in wait. */
#define WUNTRACED   0x4 /*!< Tell about stopped, untraced children. */
#define WNOWAIT     0x8 /*!< Poll only. */


#define _WSTATUS(x) ((x) & 0177)
#define _WSTOPPED   0177

/**
 * Returns true if the child exited normally.
 * Child returned by calling exit(), _exit() or some other normal way of
 * exiting.
 */
#define WIFEXITED(x)    (_WSTATUS(x) == 0)

/**
 * Returns the exit status of the child.
 */
#define WEXITSTATUS(x)  ((x) >> 8)

/**
 * Returns true if the child was signaled.
 */
#define WIFSIGNALED(x)  (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0 \
                         && (x) != 0x13)

/**
 * Returns the exit signal code.
 */
#define WTERMSIG(x)     (_WSTATUS(x))

/**
 * Returns true if the child produced a core dump.
 * @remarks Not specified in POSIX.
 */
#define WCOREDUMP(x)    (((x) & 0200) != 0)

/**
 * Returns true if the child was stopped.
 */
#define WIFSTOPPED(x)   (_WSTATUS(x) == _WSTOPPED)

/**
 * Returns the signal number that stopped the child.
 */
#define WSTOPSIG(x)     ((x) >> 8)

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/** Arguments for SYSCALL_PROC_WAIT */
struct _proc_wait_args {
    pid_t pid;
    int status;
    int options;
};
#endif

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
pid_t wait(int * status);
#if 0
int    waitid(idtype_t, id_t, siginfo_t *, int);
#endif
pid_t waitpid(pid_t pid, int * status, int options);
__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* WAIT_H */

/**
 * @}
 */
