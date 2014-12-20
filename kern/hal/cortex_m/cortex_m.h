/**
 *******************************************************************************
 * @file    cortex_m.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for Cortex-M
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

/** @addtogroup Cortex-M
  * @{
  */

#pragma once
#ifndef CORTEX_M_H
#define CORTEX_M_H

#include <kernel.h>
#include "../hal_mcu.h" /* Needed for CMSIS */
#include "../hal_core.h"

#ifndef configARM_PROFILE_M
    #error Only ARM Cortex-M profile is currently supported.
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
 * Save the context on the PSP
 */
inline void save_context(void)
{
    volatile uint32_t scratch;
#if __ARM6M__
    __asm__ volatile ("MRS   %0,  psp\n\t"
                      "SUBS  %0,  %0, #32\n\t"
                      "MSR   psp, %0\n\t"       /* This is the address that will
                                                 * be used by rd_thread_stack_ptr(void)
                                                 */
                      "ISB\n\t"
                      "STMIA %0!, {r4-r7}\n\t"
                      "PUSH  {r4-r7}\n\t"       /* Push original register values
                                                 * so we don't lost them */
                      "MOV   r4,  r8\n\t"
                      "MOV   r5,  r9\n\t"
                      "MOV   r6,  r10\n\t"
                      "MOV   r7,  r11\n\t"
                      "STMIA %0!, {r4-r7}\n\t"
                      "POP   {r4-r7}\n"         /* Pop them back */
                      : "=r" (scratch));
#elif __ARM7M__
    __asm__ volatile ("MRS   %0,  psp\n\t"
                      "STMDB %0!, {r4-r11}\n\t"
                      "MSR   psp, %0\n\t"
                      "ISB\n"
                      : "=r" (scratch));
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
#if __ARM6M__
    __asm__ volatile ("MRS   %0,  psp\n\t"
                      "ADDS  %0,  %0, #16\n\t"  /* Move to the high registers */
                      "LDMIA %0!, {r4-r7}\n\t"
                      "MOV   r8,  r4\n\t"
                      "MOV   r9,  r5\n\t"
                      "MOV   r10, r6\n\t"
                      "MOV   r11, r7\n\t"
                      "MSR   psp, %0\n\t"       /* Store the new top of the stack */
                      "ISB\n"
                      "SUBS  r0,  r0, #32\n\t"  /* Go back to the low registers */
                      "LDMIA %0!, {r4-r7}\n"
                      : "=r" (scratch));
#elif __ARM7M__
    __asm__ volatile ("MRS   %0,  psp\n\t"
                      "LDMFD %0!, {r4-r11}\n\t"
                      "MSR   psp, %0\n\t"
                      "ISB\n"
                      : "=r" (scratch));
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
    __asm__ volatile ("MRS %0, msp"
                      : "=r" (result));
    return result;
}

/**
 * Read the PSP so that it can be stored in the task table
 */
inline void * rd_thread_stack_ptr(void)
{
    void * result = NULL;
    __asm__ volatile ("MRS %0, psp"
                      : "=r" (result));
    return(result);
}

/**
 * Write stack pointer of the current thread to the PSP
 */
inline void wr_thread_stack_ptr(void * ptr)
{
    __asm__ volatile ("MSR psp, %0\n\t"
                      "ISB\n"
                      : : "r" (ptr));
}

/**
 * Platform sepcific idle sleep mode
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


void HardFault_Handler(void);

#endif /* CORTEX_M_H */

/**
  * @}
  */

/**
  * @}
  */
