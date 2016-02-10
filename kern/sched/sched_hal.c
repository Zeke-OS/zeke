/**
 *******************************************************************************
 * @file    sched_hal.c
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler, the generic part of thread scheduling.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kmem.h>
#include <ksched.h>
#include <proc.h>
#include <thread.h>

/*
 * These functions are wrappers that should be called from HAL, probably from
 * a hw specific interrupt handler.
 * @{
 */

/**
 * Enter kernel mode.
 * Called by interrupt handler.
 */
void _thread_enter_kernel(void)
{
    current_thread->curr_mpt = &mmu_pagetable_master;
}

/**
 * Exit from kernel mode.
 * Called by interrupt handler.
 */
mmu_pagetable_t * _thread_exit_kernel(void)
{
    mmu_pagetable_t * next;

    next = &curproc->mm.mpt;
    current_thread->curr_mpt = next;

    return next;
}

/**
 * Set insys flag for the current thread.
 * This function should be called right after _thread_enter_kernel().
 * Setting this flag correctly is very important for ksignal to
 * work correctly.
 */
void _thread_set_insys_flag(void)
{
    thread_flags_set(current_thread, SCHED_INSYS_FLAG);
}

void _thread_clear_insys_flag(void)
{
    thread_flags_clear(current_thread, SCHED_INSYS_FLAG);
}

void _thread_set_inabo_flag(void)
{
    thread_flags_set(current_thread, SCHED_INABO_FLAG);
}

void _thread_clear_inabo_flag(void)
{
    thread_flags_clear(current_thread, SCHED_INABO_FLAG);
}

/**
 * Suspend thread, enter scheduler.
 * Called by interrupt handler.
 */
void _thread_suspend(void)
{
    /* NOP */
}

/**
 * Resume thread from scheduler.
 * Called by interrupt handler
 */
mmu_pagetable_t * _thread_resume(void)
{
    return current_thread->curr_mpt;
}

/**
 * @}
 */
