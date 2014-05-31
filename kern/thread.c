/**
 *******************************************************************************
 * @file    thread.c
 * @author  Olli Vanhoja
 * @brief   Generic thread management and scheduling functions.
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

#define KERNEL_INTERNAL
#include <hal/core.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <vralloc.h>
#include <syscall.h>
#include <kerror.h>
#include <errno.h>
#include <timers.h>
#include <sched.h>
#include "thread.h"

#define KSTACK_SIZE ((MMU_VADDR_TKSTACK_END - MMU_VADDR_TKSTACK_START) + 1)

void sched_handler(void)
{
    threadInfo_t * const prev_thread = current_thread;

    if (!current_thread) {
        current_thread = sched_get_pThreadInfo(0);
    }

#if 0
    bcm2835_uart_uputc('.');
#endif

    /* Pre-scheduling tasks */
    if (flag_kernel_tick) { /* Run only if tick was set */
        timers_run();
    }

    /* Call the actual context switcher function that schedules the next thread.
     */
    sched_context_switcher();
    if (current_thread != prev_thread)
        mmu_map_region(&(current_thread->kstack_region->mmu));

    /* Post-scheduling tasks */
    if (flag_kernel_tick) {
        sched_calc_loads();
        mmu_calc_pfcps();
    }
}

void thread_event_timer(void * event_arg)
{
    thread->wait_tim = -1;
                        sched_thread_set_exec(thread->id);

}

void thread_init_kstack(threadInfo_t * th)
{
    /* Create kstack */
    th->kstack_region = vralloc(KSTACK_SIZE);
    if (!th->kstack_region) {
        panic("OOM during thread creation");
    }

    th->kstack_region->usr_rw = 0;
    th->kstack_region->mmu.vaddr = 0x0; /* TODO Don't hard-code this */
    th->kstack_region->mmu.pt = &mmu_pagetable_system;
}

pthread_t get_current_tid(void)
{
    if (current_thread)
        return (pthread_t)(current_thread->id);
    return 0;
}

void * thread_get_curr_stackframe(size_t ind)
{
    if (current_thread && (ind < SCHED_SFRAME_ARR_SIZE))
        return &(current_thread->sframe[ind]);
    return NULL;
}

uintptr_t thread_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_THREAD_CREATE:
        {
        /* TODO pthread_create is allowed to throw errors and we definitely should
         *      use those. */

        struct _ds_pthread_create ds;
        if (!useracc(p, sizeof(struct _ds_pthread_create), VM_PROT_WRITE)) {
            /* No permission to read/write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -1;
        }
        copyin(p, &ds, sizeof(struct _ds_pthread_create));
        sched_threadCreate(&ds, 0);
        copyout(&ds, p, sizeof(struct _ds_pthread_create));

        return 0;
        }

    case SYSCALL_THREAD_TERMINATE:
        {
        pthread_t thread_id;
        if (!useracc(p, sizeof(pthread_t), VM_PROT_READ)) {
            /* No permission to read */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -1;
        }
        copyin(p, &thread_id, sizeof(pthread_t));

        return (uintptr_t)sched_thread_terminate(thread_id);
        }

    case SYSCALL_THREAD_GETTID:
        return (uintptr_t)get_current_tid();

    case SYSCALL_THREAD_GETERRNO:
        return (uintptr_t)(current_thread->errno_uaddr);

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
