/**
 *******************************************************************************
 * @file    signal.h
 * @author  Olli Vanhoja
 * @brief   Signal numbers.
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
#ifndef SIGNAL_NUM_H
#define SIGNAL_NUM_H

/** @addtogroup Signals
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
#define SIGHUP  ((int)1)    /*!< (T) Hangup. */
#define SIGINT  ((int)2)    /*!< (T) Terminal interrupt signal. */
#define SIGQUIT ((int)3)    /*!< (A) Terminal quit signal. */
#define SIGILL  ((int)4)    /*!< (A) Illegal instruction. */
#define SIGTRAP ((int)5)    /*!< (A) Trace/breakpoint trap. */
#define SIGABRT ((int)6)    /*!< (A) Process abort signal. */
#define SIGCHLD ((int)7)    /*!< (I) Child process terminated, stopped. */
#define SIGFPE  ((int)8)    /*!< (A) Erroneous arithmetic operation. */
#define SIGKILL ((int)9)    /*!< (T) Kill (cannot be caught or ignored). */
#define SIGBUS  ((int)10)   /*!< (A) Access to an undefined portion of a memory
                             *   object. */
#define SIGSEGV ((int)11)   /*!< (A) Invalid memory reference. */
#define SIGCONT ((int)12)   /*!< (C) Continue executing, if stopped. */
#define SIGPIPE ((int)13)   /*!< (T) Write on a pipe with no one to read it. */
#define SIGALRM ((int)14)   /*!< (T) Alarm clock. */
#define SIGTERM ((int)15)   /*!< (T) Termination signal. */
#define SIGSTOP ((int)16)   /*!< (S) Stop executing (cannot be caught or
                             *   ignored). */
#define SIGTSTP ((int)17)   /*!< (S) Terminal stop signal. */
#define SIGTTIN ((int)18)   /*!< (S) Background process attempting read. */
#define SIGTTOU ((int)19)   /*!< (S) Background process attempting write. */
#define SIGUSR1 ((int)20)   /*!< (T) User-defined signal 1. */
#define SIGUSR2 ((int)21)   /*!< (T) User-defined signal 2. */
#define SIGSYS  ((int)22)   /*!< (A) Bad system call. */
#define SIGURG  ((int)23)   /*!< (I) High bandwidth data is available at a
                             *   socket. */
#define SIGINFO ((int)24)   /*!< (I) Info request. */
#define SIGPWR  ((int)25)   /*!< (T) System power failure. */

#endif /* SIGNAL_NUM_H */

/**
  * @}
  */

/**
  * @}
  */
