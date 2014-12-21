/**
 *******************************************************************************
 * @file    arm11.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
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

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

#pragma once
#ifndef ARM11_H
#define ARM11_H

#include <autoconf.h>
#include <stdint.h>
#include <hal/core.h>

#if configARM_PROFILE_M != 0
#error ARM Cortex-M profile is not supported by this layer.
#endif

#if configMMU == 0
#error MMU must be enabled when compiling for ARM11.
#endif

/** PSR interrupt bits mask */
#define PSR_INT_MASK    0x1C0u
#define PSR_INT_F       (1 << 6)
#define PSR_INT_I       (1 << 7)
#define PSR_INT_A       (1 << 8)

/* PSR Mode bits */
#define PSR_MODE_MASK   0x1fu
#define PSR_MODE_USER   0x10u
#define PSR_MODE_SYS    0x1fu
#define PSR_MODE_UNDEF  0x1bu
#define PSR_MODE_SUP    0x13u

/* Possible PSR start values for threads.
 * 2.10 The program status registers in ARM1176JZF-S Technical Reference Manual */
#define USER_PSR        0x40000010u /*!< User mode. */
#define SYSTEM_PSR      0x4000001fu /*!< Kernel mode. (System) */
#define UNDEFINED_PSR   0x4000001bu /*!< Kernel startup mode. (Undefined) */
#define SUPERVISOR_PSR  0x40000013u /*!< Kernel mode. (Supervisor) */

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
    __asm__ volatile ("CPSID aif"); \
} while (0)

/**
 * Enable interrupts (clear PRIMASK)
 */
#define enable_interrupt() do {     \
    __asm__ volatile ("CPSIE aif"); \
} while (0)

/**
 * Platform sepcific idle sleep mode.
 * Clock is stopped until one of the following events take place:
 * + An IRQ interrupt
 * + An FIQ interrupt
 * + A Debug Entry request made to the processor.
 */
#define idle_sleep() do {                   \
    /* Sleep until next interrupt */        \
    __asm__ volatile ("WFI");               \
} while (0)


#if configMP != 0

/**
 * Wait for event.
 * Clock is stopped until one of the following events take place:
 * + An IRQ interrupt, unless masked by the CPSR I Bit
 * + An FIQ interrupt, unless masked by the CPSR F Bit
 * + A Debug Entry request made to the processor and Debug is enabled
 * + An event is signaled by another processor using Send Event.
 * + Another MP11 CPU return from exception.
 */
#define cpu_wfe() do {                      \
    __asm__ volatile ("WFE");               \
} while (0)

/**
 * Send event.
 * Causes an event to be signaled to all CPUs within a multi-processor system.
 */
#define cpu_sev() do {                      \
    __asm__ volatile ("SEV");               \
} while (0)

#endif /* configMP */

/**
 * Data memory barrier.
 * The purpose of the DMB op is to ensure that all outstanding explicit memory
 * transactions are complete before following explicit memory transactions
 * begin.
 */
#define cpu_dmb() do {                      \
    int tmp = 0;                            \
    __asm__ volatile (                      \
        "MCR p15, 0, %[tmp], c7, c10, 5"    \
        : [tmp]"+r" (tmp));                \
} while (0)

/**
 * Halt due to kernel panic.
 */
#define panic_halt() do {                   \
    __asm__ volatile ("BKPT #01");          \
} while (0)

#define DEBUG_PRINT_CALLER() do {                           \
    intptr_t tmp;                                           \
    char buf[80];                                           \
    __asm__ volatile ("mov %[tmp], lr" : [tmp]"=r" (tmp));  \
    ksprintf(buf, sizeof(buf), "caller = %x", tmp - 4);     \
    KERROR(KERROR_DEBUG, buf);                              \
} while (0)

#endif /* ARM11_H */

/**
  * @}
  */

/**
  * @}
  */
