/**
 *******************************************************************************
 * @file    hal_core.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for the CPU core
 * @section LICENSE
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup HAL
 * @{
 */

#pragma once
#ifndef HAL_CORE_H
#define HAL_CORE_H

#include <pthread.h>

/* Select Core Implementation ************************************************/
#if defined(configARM_PROFILE_M) /* All M profile cores. */
#error CORTEX-M profile is not supported
#elif defined(__ARM6__) || defined(__ARM6K__) /* ARM11 uses ARMv6 arch */
#include "../hal/arm11/arm11.h"
#else
#error Selected ARM profile/architecture is not supported
#endif

#ifdef configUSE_HFP
#define IS_HFP_PLAT 1
#else
#define IS_HFP_PLAT 0
#endif

struct thread_info;

/**
 * Type for storing interrupt state.
 */
typedef size_t istate_t;

/**
 * Init stack frame.
 * @param thread_def    contains the thread definitions.
 * @param sframe        is the stack frame used when the thread is put to sleep.
 * @param priv          set thread as privileged/kernel mode thread.
 * @return Returns the TLS address.
 */
__user void * init_stack_frame(struct _sched_pthread_create_args * thread_def,
                               thread_stack_frames_t * tsf, int priv);

/**
 * Get a pointer to the stack frame that will return to user space.
 */
sw_stack_frame_t * get_usr_sframe(struct thread_info * thread);

/**
 * Functions that can be calling while in a syscall.
 * @{
 */

/**
 * Get arguments of a syscall.
 */
void svc_getargs(uint32_t * type, uintptr_t * p);

/**
 * Set the return value of a system call.
 */
void svc_setretval(intptr_t retval);

/**
 * @}
 */

/**
 * Get interrupt disable state.
 * This function can be called before calling disable_interrupt() to preserve
 * current state of interrupt flags.
 * @return Returns interrupt state.
 */
istate_t get_interrupt_state(void);

/**
 * Set interrupt state.
 * Sets interrupt state flags previously preserved with get_interrupt_state().
 * @param state is the interrupt state variable.
 */
void set_interrupt_state(istate_t state);

/**
 * Yield from a thread.
 * This function should be implemented similarly to the normal interrupt driven
 * context switch, thus the thread must be able to resume normally on a next
 * execution window.
 */
void hal_thread_yield(void);

void stack_dump(sw_stack_frame_t frame);

/*
 * Core Implementation must provide following either as inlined functions or
 * macros:
 * + disable_interrupt()
 * + enable_interrupt()
 * + req_context_switch()
 * + idle_sleep()
 * and the following types:
 * + hw_stack_frame_t - is a struct that describes hardware backed stack frame.
 * + sw_stack_frame_t - is a struct that describes software backed stack frame.
 * + thread_stack_frames_t - is a type defining a hardware specific stack frame
 *   for threads.
 */

#endif /* HAL_CORE_H */

/**
 * @}
 */
