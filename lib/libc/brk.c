/**
 *******************************************************************************
 * @file    brk.c
 * @author  Olli Vanhoja
 * @brief   Change space allocation.
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

#include <syscall.h>
#include <errno.h>
#include <unistd.h>

/** @addtogroup unistd
 *  @{
 */

static struct _ds_getbreak ds_brk;
static void * curr_break;

static int getbrk(void)
{
    /* Following syscall just gets the start and stop limits for the brk
     * functionality. Actual brk operation is solely implemented in
     * userland.
     */

    if (ds_brk.start == 0) {
        if (syscall(SYSCALL_PROC_GETBREAK, &ds_brk)) {
            /* This should never happen unles user is trying to do something
             * fancy. */
            errno = EAGAIN;
            return -1;
        }
    }

    return 0;
}

int brk(void * addr)
{
    intptr_t alloc_size;

    if (getbrk())
        return -1;

    if ((ds_brk.start > addr) || (addr > ds_brk.stop)) {
        errno = ENOMEM;
        return -1;
    }

    alloc_size = ((intptr_t)addr - (intptr_t)curr_break);
    if (alloc_size > 0)
        memset(curr_break, 0, (size_t)alloc_size);

    curr_break = addr;
    return 0;
}

void * sbrk(intptr_t incr)
{
    void * old_break = curr_break;
    void * new_break = (void *)((char *)curr_break + incr);

    if (getbrk())
        return (void *)-1;

    if ((ds_brk.start > new_break) || (new_break > ds_brk.stop)) {
        errno = ENOMEM;
        return (void *)-1;
    }

    if (incr > 0)
        memset(old_break, 0, (size_t)incr);

    curr_break = new_break;
    return old_break;
}

/**
  * @}
  */

/**
  * @}
  */
