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
#include "../hal_mcu.h" /* Needed for CMSIS */
#include "../hal_core.h"

#if configARM_PROFILE_M != 0
    #error ARM Cortex-M profile is not supported by this layer.
#endif
#ifndef configARCH
    #error Core is not selected by the compiler.
#endif

/* Exception return values */
#define HAND_RETURN         0xFFFFFFF1u /*!< Return to handler mode using the MSP. */
#define MAIN_RETURN         0xFFFFFFF9u /*!< Return to thread mode using the MSP. */
#define THREAD_RETURN       0xFFFFFFFDu /*!< Return to thread mode using the PSP. */

#define DEFAULT_PSR         0x21000000u

/* Stack frame saved by the hardware */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} hw_stack_frame_t;

/* Stack frame save by the software */
typedef struct {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
} sw_stack_frame_t;

/* Inlined core functions */
inline void save_context(void);
inline void load_context(void);
inline void * rd_stack_ptr(void);
inline void * rd_thread_stack_ptr(void);
inline void wr_thread_stack_ptr(void * ptr);

/**
 * Disable all interrupts except NMI (set PRIMASK)
 */
#define disable_interrupt() do {                                               \
    __asm__ volatile ("CPSID i\n"); } while (0)

/**
 * Enable interrupts (clear PRIMASK)
 */
#define enable_interrupt() do {                                                \
    __asm__ volatile ("CPSIE i\n"); } while (0)

/**
 * Request immediate context switch
 */
#define req_context_switch() do {                                              \
    syscall(0, 0); /* TODO this is not ok but we have to implement this with
                    * syscalls later */                                        \
    } while (0)

/**
 * Save the context on the PSP
 */
inline void save_context(void)
{
    volatile uint32_t scratch;

#if configARCH == __ARM6__ || configARCH == __ARM6K__
    /* TODO push registers to thread stack */
#else
    #error Selected CORE not supported
#endif
}

/**
 * Load the context from the PSP
 */
inline void load_context(void)
{
    volatile uint32_t scratch;
#if configARCH == __ARM6__ || configARCH == __ARM6K__
        /* TODO Pop registers from thread stack */
#else
    #error Selected CORE not supported
#endif
}

/**
 * Read the main stack pointer
 */
inline void * rd_stack_ptr(void)
{
    void * result = NULL;
    /* TODO There is no such special register in real ARM achs. */
    /*__asm__ volatile ("MRS %0, msp\n"
                      : "=r" (result));*/
    return result;
}

/**
 * Read the PSP so that it can be stored in the task table
 */
inline void * rd_thread_stack_ptr(void)
{
    void * result = NULL;
    /* TODO There is no such special register in real ARM achs. */
    /*__asm__ volatile ("MRS %0, psp\n"
                      : "=r" (result));*/
    return(result);
}

/**
 * Write stack pointer of the current thread to the PSP
 */
inline void wr_thread_stack_ptr(void * ptr)
{
    /* TODO */
    /*__asm__ volatile ("MSR psp, %0\n"
                      "ISB\n"
                      : : "r" (ptr));*/
}

/**
 * Platform sepcific idle sleep mode.
 */
#define idle_sleep() do {                                 \
    __asm__ volatile ("WFI\n" /* Sleep until next interrupt */ \
                     ); } while (0)


void HardFault_Handler(void);

#endif /* ARM11_H */

/**
  * @}
  */

/**
  * @}
  */
