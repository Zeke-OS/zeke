/**
 *******************************************************************************
 * @file arm1176jzf_s_interrupt.c
 * @author Olli Vanhoja
 * @brief Interrupt service routines.
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

/** @addtogroup ARM1176JZF_S
* @{
*/

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <sched.h>
#include <syscall.h>
#include <hal/hal_core.h>
#include <hal/mmu.h>
#include "arm11_interrupt.h"

/* Peripheral Addresses */

static volatile uint32_t * irq_enable1 = (uint32_t *)0x2000b210;
static volatile uint32_t * irq_enable2 = (uint32_t *)0x2000b214;
static volatile uint32_t * irq_enable_basic = (uint32_t *)0x2000b218;

static volatile uint32_t * arm_timer_load = (uint32_t *)0x2000b400;
static volatile uint32_t * arm_timer_value = (uint32_t *)0x2000b404;
static volatile uint32_t * arm_timer_control = (uint32_t *)0x2000b408;
static volatile uint32_t * arm_timer_irq_clear = (uint32_t *)0x2000b40c;
/* End of Peripheral Addresses */

void interrupt_init_module(void) __attribute__((constructor));

/* Interrupt vectors.
 *
 * @ note This needs to be aligned to 32 bits as the bottom 5 bits of the vector
 * address as set in the control coprocessor must be zero.
 */
__attribute__ ((naked, aligned(32))) static void interrupt_vectors(void)
{
    /* Processor will never jump to bad_exception when reset is hit because
     * interrupt vector offset is set back to 0x0 on reset. */
    asm volatile(
        "b bad_exception\n\t"               /* RESET */
        "b bad_exception\n\t"               /* UNDEF */
        "b interrupt_svc\n\t"               /* SVC   */
        "b bad_exception\n\t"               /* Prefetch abort */
        "b bad_exception\n\t"               /* data abort */
        "b bad_exception\n\t"               /* Unused vector */
        "b bad_exception\n\t"               /* IRQ */
        "b bad_exception\n\t"               /* FIQ */
    );
}

/**
 * Unhandled exception
 */
__attribute__ ((naked)) void bad_exception(void)
{
    while (1) {
        __asm__ volatile ("wfe");
    }
}

void interrupt_init_module(void)
{
    /* Set interrupt base register */
    asm volatile("mcr p15, 0, %[addr], c12, c0, 0"
            : : [addr]"r" (&interrupt_vectors));
    /* Turn on interrupts */
    asm volatile("cpsie i");

    /* Use the ARM timer - BCM 2832 peripherals doc, p.196 */
    /* Enable ARM timer IRQ */
    *irq_enable_basic = 0x00000001;

    /* Interrupt every 1024 * 256 (prescaler) timer ticks */
    *arm_timer_load = 0x00000400;

    /* Timer enabled, interrupt enabled, prescale=256, 23 bit counter */
    *arm_timer_control = 0x000000aa;
}

/**
* @}
*/

/**
* @}
*/
