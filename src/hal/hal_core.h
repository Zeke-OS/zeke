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
#include <kernel.h>

#if configMP != 0 && configPREEMPT == 0
#error configMP requires configPREEMPT
#endif

/**
 * Type for storing interrupt state.
 */
typedef size_t istate_t;

extern volatile size_t flag_kernel_tick;

/**
 * Init stack frame.
 * @param thread_def Thread definitions
 * @param a_del_thread Address of del_thread function
 * @param priv set thread as privileged/kernel mode thread.
 */
void init_stack_frame(ds_pthread_create_t * thread_def,
        void (*a_del_thread)(void *), int priv);

/**
 * Make a system call
 * @param type syscall type.
 * @param p pointer to the syscall parameter(s).
 * @return return value of the called kernel function.
 * @note Must be only used in thread scope.
 */
uint32_t syscall(uint32_t type, void * p);

/**
 * Test and set
 * @param lock pointer to the lock variable.
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


/* Core Implementation must declare following inlined functions:
 * + inline void * rd_thread_stack_ptr(void)
 * + inline void wr_thread_stack_ptr(void * ptr)
 * and these can be done as either inlined functions or macros:
 * + disable_interrupt()
 * + enable_interrupt()
 * + req_context_switch()
 * + idle_sleep()
 * and the following types:
 * + hw_stack_frame_t - is a struct that describes hardware backed stack frame.
 * + sw_stack_frame_t - is a struct that describes software backed stack frame.
 */

/* Select Core Implementation ************************************************/

#if configARM_PROFILE_M != 0 /* All M profile cores are handled in one file. */
#include "cortex_m/cortex_m.h"
#elif configARCH == __ARM6__ || __ARM6K__ /* ARM11 uses ARMv6 arch */
#include "arm11/arm11.h"
#elif PU_TEST_BUILD == 1
#include "pu_test_core.h"
#else
    #error Selected ARM profile/architecture is not supported
#endif

#endif /* HAL_CORE_H */

/**
  * @}
  */

