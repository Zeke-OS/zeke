/**
 *******************************************************************************
 * @file    arm11.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
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

/** @addtogroup ARM11
  * @{
  */

#pragma once
#ifndef ARM11_H
#define ARM11_H

#include <kernel.h>
#include <sched.h>
#include <hal/hal_mcu.h>
#include <hal/hal_core.h>

#if configARM_PROFILE_M != 0
    #error ARM Cortex-M profile is not supported by this layer.
#endif
#ifndef configARCH
    #error Core is not selected.
#endif

#if configMMU == 0
    #error MMU must be enabled when compiling for ARM11.
#endif

/* 2.10 The program status registers in ARM1176JZF-S Technical Reference Manual */
#define DEFAULT_PSR         0x40000010u

/** Stack frame saved by the hardware (Left here for compatibility reasons) */
typedef struct {
} hw_stack_frame_t;

/** Stack frame save by the software */
typedef struct {
    uint32_t psr;   /*!< PSR */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;    /*!< r13 */
    uint32_t lr;    /*!< r14 */
    uint32_t pc;    /*!< r15/lr return point */
} sw_stack_frame_t;

void cpu_invalidate_caches(void);
void cpu_set_cid(uint32_t cid);

void * rd_thread_stack_ptr(void);
void wr_thread_stack_ptr(void * ptr);

/**
 * Disable all interrupts except NMI (set PRIMASK)
 */
#define disable_interrupt() do {    \
    __asm__ volatile ("CPSID i");   \
} while (0)

/**
 * Enable interrupts (clear PRIMASK)
 */
#define enable_interrupt() do {     \
    __asm__ volatile ("CPSIE i");   \
} while (0)

/**
 * Request immediate context switch
 *
 * Called from thread context.
 * TODO Make this a common implementation for all ARM processors?
 */
#define req_context_switch() do {           \
    /* Simple and doesn't cause jitter: */  \
    __asm__ volatile ("WFI");               \
} while (0)

/**
 * Platform sepcific idle sleep mode.
 */
#define idle_sleep() do {               \
    /* Sleep until next interrupt */    \
    __asm__ volatile ("WFI");           \
} while (0)

/**
 * Halt due to kernel panic.
 */
#define panic_halt() do {               \
    __asm__ volatile ("BKPT #01");      \
} while (0)

#define eval_syscall_block() do {                   \
    if (SCHED_TEST_CSW_OK(current_thread->flags))   \
        __asm__ volatile ("mov r1, #0");            \
    else                                            \
        __asm__ volatile ("mov r1, #1");            \
} while (0)

__attribute__ ((naked)) void undef_handler(void);

#endif /* ARM11_H */

/**
  * @}
  */

/**
  * @}
  */
