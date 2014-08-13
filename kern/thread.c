/**
 *******************************************************************************
 * @file    thread.c
 * @author  Olli Vanhoja
 * @brief   Generic thread management and scheduling functions.
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
 *******************************************************************************
 */

#define KERNEL_INTERNAL
#include <libkern.h>
#include <sys/linker_set.h>
#include <hal/core.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <vralloc.h>
#include <syscall.h>
#include <kerror.h>
#include <errno.h>
#include <timers.h>
#include <sched.h>
#include <thread.h>

#define KSTACK_SIZE ((MMU_VADDR_TKSTACK_END - MMU_VADDR_TKSTACK_START) + 1)

/* Linker sets for pre- and post-scheduling tasks */
SET_DECLARE(pre_sched_tasks, void);
SET_DECLARE(post_sched_tasks, void);
SET_DECLARE(sched_idle_tasks, void);

void sched_handler(void)
{
    threadInfo_t * const prev_thread = current_thread;
    void ** task_p;

    if (!current_thread) {
        current_thread = sched_get_pThreadInfo(0);
    }

#if 0
    bcm2835_uart_uputc('.');
#endif

    /* Pre-scheduling tasks */
    SET_FOREACH(task_p, pre_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }

    /*
     * Call the actual context switcher function that schedules the next thread.
     */
    sched_context_switcher();
    if (current_thread != prev_thread)
        mmu_map_region(&(current_thread->kstack_region->mmu));

    /* Post-scheduling tasks */
    SET_FOREACH(task_p, post_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }
}

/**
 * Kernel idle thread
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 */
void * idle_thread(/*@unused@*/ void * arg)
{
    void ** task_p;

    while(1) {
        /* Execute idle tasks */
        SET_FOREACH(task_p, sched_idle_tasks) {
            sched_task_t task = *(sched_task_t *)task_p;
            task();
        }

        idle_sleep();
    }
}


static void thread_event_timer(void * event_arg)
{
    threadInfo_t * thread = (threadInfo_t *)event_arg;

    timers_release(thread->wait_tim);
    thread->wait_tim = -1;
    current_thread->flags &= ~SCHED_WAIT_FLAG;
    sched_thread_set_exec(thread->id);
}

void sched_thread_sleep(long millisec)
{
    //istate_t s;
    int timer_id;

    do {
        timer_id = timers_add(thread_event_timer, current_thread,
            TIMERS_FLAG_ONESHOT, millisec * 1000);
    } while (timer_id < 0);
    current_thread->wait_tim = timer_id;

    //s = get_interrupt_state();
    //disable_interrupt(); /* TODO Not MP safe! */
    /* This should prevent anyone from waking up this thread for a while. */
    timers_start(timer_id);
    sched_thread_sleep_current(0); /* TODO Should work with 1 */
    //set_interrupt_state(s);

    do {
        idle_sleep();
    } while (current_thread->wait_tim >= 0);
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

void thread_free_kstack(threadInfo_t * th)
{
    vrfree(th->kstack_region);
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

static int sys_thread_create(void * user_args)
{
    struct _ds_pthread_create args;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        return -1;
    }

    copyin(user_args, &args, sizeof(args));
    sched_threadCreate(&args, 0);
    copyout(&args, user_args, sizeof(args));

    return 0;
}

static int sys_thread_terminate(void * user_args)
{
    pthread_t thread_id;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return sched_thread_terminate(thread_id);
}

static int sys_get_current_tid(void * user_args)
{
    return (int)get_current_tid();
}

/**
 * Get address of thread errno.
 */
static intptr_t sys_geterrno(void * user_args)
{
    return (intptr_t)current_thread->errno_uaddr;
}

static const syscall_handler_t thread_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_CREATE, sys_thread_create),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_TERMINATE, sys_thread_terminate),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_GETTID, sys_get_current_tid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_GETERRNO, sys_geterrno)
};
SYSCALL_HANDLERDEF(thread_syscall, thread_sysfnmap)
