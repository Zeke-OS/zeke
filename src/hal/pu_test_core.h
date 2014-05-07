/**
 *******************************************************************************
 * @file    pu_test_core.h
 * @author  Olli Vanhoja
 * @brief   HAL test core
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup Test
  * @{
  */

#pragma once
#ifndef PU_TEST_CORE_H
#define PU_TEST_CORE_H

#ifndef PU_TEST_BUILD
    #error pu_test_core.h should not be #included in production!
#endif

#include "syscall.h"

/* Exception return values */
#define HAND_RETURN         0xFFFFFFF1u /*!< Return to handler mode using the MSP. */
#define MAIN_RETURN         0xFFFFFFF9u /*!< Return to thread mode using the MSP. */
#define THREAD_RETURN       0xFFFFFFFDu /*!< Return to thread mode using the PSP. */

#define DEFAULT_PSR         0x21000000u

/* IAR & core specific functions */
typedef uint32_t __istate_t;
#define __get_interrupt_state() 0
#define __enable_interrupt() do { } while(0)
#define __disable_interrupt() do { } while(0)
#define __set_interrupt_state(state) do { state = state; } while(0)

/* Stack frame saved by the hardware */
typedef struct {
    uint32_t r;
} hw_stack_frame_t;

/* Stack frame save by the software */
typedef struct {
    uint32_t r;
} sw_stack_frame_t;


/**
  * Init hw stack frame
  */
void init_stack_frame(/*@unused@*/ ds_pthread_create_t * thread_def, /*@unused@*/ int priv)
{
}

/**
  * Request immediate context switch
  */
inline void req_context_switch(void)
{
}

/**
  * Save the context on the PSP
  */
inline void save_context(void)
{
}

/**
  * Load the context from the PSP
  */
inline void load_context(void)
{
}

/**
  * Read the main stack pointer
  */
inline void * rd_stack_ptr(void)
{
    return NULL;
}

/**
  * Read the PSP so that it can be stored in the task table
  */
inline void * rd_thread_stack_ptr(void)
{
    return NULL;
}

/**
  * Write stack pointer of the currentthread to the PSP
  */
inline void wr_thread_stack_ptr(/*@unused@*/ void * ptr)
{
}

int test_and_set(int * lock)
{
    int old;

    old = *lock;
    *lock = 1;

    return old;
}

#ifdef PU_TEST_SYSCALLS
/*
 * Call syscalls directly as we can't use any SVC on universal test environment.
 */
inline uint32_t syscall(uint32_t type, void * p)
{
    return _intSyscall_handler(type, p);
}
#endif

#endif /* PU_TEST_CORE_H */

/**
  * @}
  */

/**
  * @}
  */
