/**
 *******************************************************************************
 * @file    signal.h
 * @author  Olli Vanhoja
 * @brief   Signal numbers.
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
 * @addtogroup Library_Functions
 * @{
 */

#ifndef SIGNAL_NUM_H
#define SIGNAL_NUM_H

/**
 * @addtogroup Signals
 * @{
 */

/* List of signals
 * ---------------
 *  Default Action:
 *  T   Abnormal termination of the process.
 *  A   Abnormal termination of the process with additional actions.
 *  I   Ignore signal.
 *  S   Stop the process.
 *  C   Continue the process, if it is stopped; otherwise, ignore the signal.
 */
#define SIGHUP  1       /*!< (T) Hangup. */
#define SIGINT  2       /*!< (T) Terminal interrupt signal. */
#define SIGQUIT 3       /*!< (A) Terminal quit signal. */
#define SIGILL  4       /*!< (A) Illegal instruction. */
#define SIGTRAP 5       /*!< (A) Trace/breakpoint trap. */
#define SIGABRT 6       /*!< (A) Process abort signal. */
#define SIGCHLD 7       /*!< (I) Child process terminated, stopped. */
#define SIGFPE  8       /*!< (A) Erroneous arithmetic operation. */
#define SIGKILL 9       /*!< (T) Kill (cannot be caught or ignored). */
#define SIGBUS  10      /*!< (A) Access to an undefined portion of a memory
                         *   object. */
#define SIGSEGV 11      /*!< (A) Invalid memory reference. */
#define SIGCONT 12      /*!< (C) Continue executing, if stopped. */
#define SIGPIPE 13      /*!< (T) Write on a pipe with no one to read it. */
#define SIGALRM 14      /*!< (T) Alarm clock. */
#define SIGTERM 15      /*!< (T) Termination signal. */
#define SIGSTOP 16      /*!< (S) Stop executing (cannot be caught or
                         *   ignored). */
#define SIGTSTP 17      /*!< (S) Terminal stop signal. */
#define SIGTTIN 18      /*!< (S) Background process attempting read. */
#define SIGTTOU 19      /*!< (S) Background process attempting write. */
#define SIGUSR1 20      /*!< (T) User-defined signal 1. */
#define SIGUSR2 21      /*!< (T) User-defined signal 2. */
#define SIGSYS  22      /*!< (A) Bad system call. */
#define SIGURG  23      /*!< (I) High bandwidth data is available at a
                         *   socket. */
#define SIGINFO 24      /*!< (I) Info request. */
#define SIGPWR  25      /*!< (T) System power failure. */
#define _SIGMTX 31      /*!< (I) Wakeup mutex. */

#define _SIG_MAX_ 32

/**
 * @}
 */

#endif /* SIGNAL_NUM_H */

/**
 * @}
 */
