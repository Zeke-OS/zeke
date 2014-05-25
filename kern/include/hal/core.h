/**
 *******************************************************************************
 * @file    hal_core.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for the CPU core
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

/** @addtogroup HAL
 * @{
 */

#pragma once
#ifndef HAL_CORE_H
#define HAL_CORE_H

#include <autoconf.h>
#include <syscalldef.h>

/* Select Core Implementation ************************************************/
#if configARM_PROFILE_M != 0 /* All M profile cores are handled in one file. */
#include "../hal/cortex_m/cortex_m.h"
#elif configARCH == __ARM6__ || __ARM6K__ /* ARM11 uses ARMv6 arch */
#include "../hal/arm11/arm11.h"
#else
#error Selected ARM profile/architecture is not supported
#endif


/**
 * Type for storing interrupt state.
 */
typedef size_t istate_t;

extern volatile size_t flag_kernel_tick;

/**
 * Init stack frame.
 * @param thread_def    contains the thread definitions.
 * @param sframe        is the stack frame used when the thread is put to sleep.
 * @param priv          set thread as privileged/kernel mode thread.
 */
void init_stack_frame(struct _ds_pthread_create * thread_def,
        sw_stack_frame_t * sframe, int priv);

/**
 * Test if locked.
 * @param lock is pointer to a lock variable.
 * @return Lock value.
 */
int test_lock(int * lock);

/**
 * Test and set.
 * @param lock is pointer to a lock variable.
 * @return 0 if set succeeded.
 */
int test_and_set(int * lock);

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
 * Get us timestamp.
 * @return Returns us timestamp.
 */
uint64_t get_utime(void);

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
 */

#endif /* HAL_CORE_H */

/**
 * @}
 */
