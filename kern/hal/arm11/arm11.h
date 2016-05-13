/**
 *******************************************************************************
 * @file    arm11.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup ARM11
 * @{
 */

#pragma once
#ifndef ARM11_H
#define ARM11_H

#include <stdint.h>
#include <hal/core.h>

#if defined(configARM_PROFILE_M)
#error ARM Cortex-M profile is not supported by this layer.
#endif

#if !defined(configMMU)
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

/*
 * Possible PSR start values for threads.
 * 2.10 The program status registers in ARM1176JZF-S Technical Reference Manual
 */
#define USER_PSR        0x40000010u /*!< User mode. */
#define SYSTEM_PSR      0x4000001fu /*!< Kernel mode. (System) */
#define UNDEFINED_PSR   0x4000001bu /*!< Kernel startup mode. (Undefined) */
#define SUPERVISOR_PSR  0x40000013u /*!< Kernel mode. (Supervisor) */

/**
 * (I)FSR Status Mask
 */
#define FSR_STATUS_MASK 0x0f

/*
 * Aborts and registers.
 * Type                     ABT PRECISE IFSR    IFAR    DFSR    FAR WFAR
 * ---------------------------------------------------------------------
 * Int MMU fault            PAB X       X       X
 * Int debug abort          PAB X       X
 * Int ext abort on tr      PAB X       X       X
 * Int ext abort            PAB X       X       X
 * Int cache maint. op      DAB X                       X       X
 * Data MMU fault           DAB X                       X       X
 * Data debug abort         DAB                         X       X
 * Data ext abort on tr     DAB X                       X       X
 * Data ext abort           DAB                         X       X
 * Data cache maint. op     DAB X                       X       X
 */

/**
 * Test if abort came from user mode.
 */
#define ABO_WAS_USERMODE(psr)   (((psr) & PSR_MODE_MASK) == PSR_MODE_USER)

/** Stack frame saved by the hardware. (Left here for compatibility reasons) */
typedef struct {
} hw_stack_frame_t;

/** Stack frame save by the software. */
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

/**
 * Stack frames.
 * @{
 */
#define SCHED_SFRAME_SYS        0   /*!< Sys int/Scheduling stack frame. */
#define SCHED_SFRAME_SVC        1   /*!< Syscall stack frame. */
#define SCHED_SFRAME_ABO        2   /*!< Stack frame for aborts. */
#define SCHED_SFRAME_ARR_SIZE   3

typedef struct thread_stack_frames {
    sw_stack_frame_t s[SCHED_SFRAME_ARR_SIZE];
} thread_stack_frames_t;
/**
 * @}
 */


/** Other registers requiring sw backups */
struct tls_regs {
    uint32_t utls; /*!< User rw: cp15 c13 2 */
#ifdef configUSE_HFP
    /* VFP system registers */
    uint32_t fpscr;     /*!< Floating-Point Status and Control Register. */
    uint32_t fpexc;     /*!< Floating-Point Exception Register. */
    uint32_t fpinst;    /*!< Floating-Point Instruction Register. */
    uint32_t fpinst2;   /*!< Floating-Point Instruction Register 2. */
    /* VFP register file */
    uint32_t dreg[64];
#endif
};

void cpu_invalidate_caches(void);

uint32_t core_get_user_tls(void);
void core_set_user_tls(uint32_t value);
__user struct _sched_tls_desc * core_get_tls_addr(void);
void core_set_tls_addr(__user struct _sched_tls_desc * tls);

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


#if defined(configMP)

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
 * Write memory barrier.
 * Execute Drain write buffer and DMB operations.
 * The purpose of the DMB is to ensure that all outstanding explicit memory
 * transactions are complete before following explicit memory transactions
 * begin.
 */
#define cpu_wmb() do {                      \
    const uint32_t tmp = 0;                 \
    __asm__ volatile (                      \
        "MCR p15, 0, %[rd], c7, c10, 4\n\t" \
        "MCR p15, 0, %[rd], c7, c10, 5"     \
        : [rd]"+r" (tmp));                  \
} while (0)

/**
 * Halt due to kernel panic.
 */
static inline void panic_halt(void) __attribute__((noreturn));
static inline void panic_halt(void)
{
    disable_interrupt();
#if defined(configMP)
    /* TODO Handle MP */
    cpu_wfe();
#else
    idle_sleep();
#endif
    while (1);
    __builtin_unreachable();
}

#endif /* ARM11_H */

/**
 * @}
 */

/**
 * @}
 */
