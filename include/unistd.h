/**
 *******************************************************************************
 * @file    unistd.h
 * @author  Olli Vanhoja
 * @brief   Standard symbolic constants and types.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef UNISTD_H
#define UNISTD_H

#include <sys/cdefs.h>

/* Options and option groups */
#define _POSIX_THREAD_ATTR_STACKADDR    200809L
#define _POSIX_THREAD_ATTR_STACKSIZE    200809L
#define _POSIX_THREADS                  200809L

#ifndef NULL
#define NULL ((void*)0)
#endif

/* Block device seek origin types */
#ifndef SEEK_SET
#define SEEK_SET    0 /*!< Beginning of file. */
#define SEEK_CUR    1 /*!< Current position */
#define SEEK_END    2 /*!< End of file. */
#endif

#define STDIN_FILENO    0 /*!< File number of stdin. */
#define STDOUT_FILENO   1 /*!< File number of stdout. */
#define STDERR_FILENO   2 /*!< File number of stderr. */

__BEGIN_DECLS
/**
 * Set the break value.
 * @param addr is the new break addr.
 * @return  Returns 0 on succesful completion;
 *          Returns -1 and sets errno on error.
 * @throws  ENOMEM = The requested change would be impossible,
 *          EAGAIN = Temporary failure.
 */
int brk(void * addr);

/**
 * Increment or decrement the break value.
 * @param incr is the amount of allocated space in bytes. Negative value
 * decrements the break value.
 * @return  Returns the prior break value on succesful operation;
 *          Returns (void *)-1 if failed.
 * @throws  ENOMEM = The requested change would be impossible,
 *          EAGAIN = Temporary failure.
 */
void * sbrk(intptr_t incr);

pid_t fork(void);
ssize_t pwrite(int fildes, const void *buf, size_t nbyte,
    off_t offset);
ssize_t write(int fildes, const void * buf, size_t nbyte);

unsigned sleep(unsigned seconds);
__END_DECLS

#endif /* UNISTD_H */
