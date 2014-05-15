/**
 *******************************************************************************
 * @file    kernel.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
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

/** @addtogroup Library_Functions
  * @{
  */

#include <hal/hal_core.h>
#include "syscalldef.h"
#include "syscall.h"
#include <unistd.h>
#include <kernel.h>

unsigned sleep(unsigned seconds)
{
    unsigned int millisec = seconds * 1000;

    return (unsigned)syscall(SYSCALL_SCHED_SLEEP_MS, &millisec);
}

unsigned msleep(unsigned millisec)
{
    return (unsigned)syscall(SYSCALL_SCHED_SLEEP_MS, &millisec);
}

/** @addtogroup unistd
 *  @{
 */

pid_t fork(void)
{
    return (pid_t)syscall(SYSCALL_PROC_FORK, NULL);
}

ssize_t pwrite(int fildes, const void * buf, size_t nbyte,
        off_t offset)
{
    struct _fs_write_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbyte = nbyte,
        .offset = offset
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

ssize_t write(int fildes, const void * buf, size_t nbyte)
{
    struct _fs_write_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbyte = nbyte,
        .offset = SEEK_CUR
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

/**
  * @}
  */

/** @addtogroup Thread_Management
  * @{
  */

int osThreadTerminate(pthread_t thread_id)
{
    return (int)syscall(SYSCALL_THREAD_TERMINATE, &thread_id);
}

int * __error(void)
{
    return (int *)syscall(SYSCALL_THREAD_GETERRNO, NULL);
}


/**
  * @}
  */

/** @addtogroup Semaphore
  * @{
  */

//osSemaphore osSemaphoreCreate(osSemaphoreDef_t * semaphore_def, int32_t count)
//{
    /* TODO Implementation */
//}

int32_t osSemaphoreWait(osSemaphore * semaphore, uint32_t millisec)
{
    ds_osSemaphoreWait_t ds = {
        .s = &(semaphore->s),
        .millisec = millisec
    };
    int retVal;

    /* Loop between kernel mode and thread mode :) */
    while ((retVal = syscall(SYSCALL_SEMAPHORE_WAIT, &ds)) < 0) {
        if (retVal == OS_SEMAPHORE_THREAD_SPINWAIT_RES_ERROR) {
            return -1;
        }

        /* TODO priority should be lowered or some resceduling should be done
         * in the kernel so this loop would not waste time before automatic
         * rescheduling. */
        req_context_switch();
    }

    return retVal;
}

int osSemaphoreRelease(osSemaphore * semaphore)
{
    syscall(SYSCALL_SEMAPHORE_RELEASE, semaphore);
    return 0;
}

/**
  * @}
  */

/**
  * @}
  */
