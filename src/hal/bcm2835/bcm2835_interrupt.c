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

/** @addtogroup BCM2835
* @{
*/

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <sched.h>
#include <syscall.h>
#include <kinit.h>
#include <hal/hal_core.h>
#include <hal/mmu.h>
#include "bcm2835_mmio.h"
#include "bcm2835_interrupt.h"

#define ARM_TIMER_PRESCALE_1    0x0
#define ARM_TIMER_PRESCALE_16   0x4
#define ARM_TIMER_PRESCALE_256  0x8

#define ARM_TIMER_EN            0x80
#define ARM_TIMER_INT_EN        0x20

/* Peripheral Addresses */
#define IRQ_ENABLE1         0x2000b210
#define IRQ_ENABLE2         0x2000b214
#define IRQ_ENABLE_BASIC    0x2000b218

#define ARM_TIMER_LOAD      0x2000b400
#define ARM_TIMER_VALUE     0x2000b404
#define ARM_TIMER_CONTROL   0x2000b408
#define ARM_TIMER_IRQ_CLEAR 0x2000b40c
/* End of Peripheral Addresses */


void interrupt_init_module(void);
SECT_HW_POSTINIT(interrupt_init_module);

/**
 * Interrupt vectors.
 *
 * @ note This needs to be aligned to 32 bits as the bottom 5 bits of the vector
 * address as set in the control coprocessor must be zero.
 */
__attribute__ ((naked, aligned(32))) static void interrupt_vectors(void)
{
    /* Processor will never jump to bad_exception when reset is hit because
     * interrupt vector offset is set back to 0x0 on reset. */
    __asm__ volatile (
        "b bad_exception\n\t"               /* RESET */
        "b bad_exception\n\t"               /* UNDEF */
        "b interrupt_svc\n\t"               /* SVC   */
        "b bad_exception\n\t"               /* Prefetch abort */
        "b bad_exception\n\t"               /* data abort */
        "b bad_exception\n\t"               /* Unused vector */
        "b interrupt_systick\n\t"           /* Timer IRQ */
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
    __asm__ volatile ("mcr p15, 0, %[addr], c12, c0, 0"
            : : [addr]"r" (&interrupt_vectors));
    /* Turn on interrupts */
    __asm__ volatile ("cpsie i");

    mmio_start();

    /* Use the ARM timer - BCM 2832 peripherals doc, p.196 */
    /* Enable ARM timer IRQ */
    mmio_write(IRQ_ENABLE_BASIC, 0x00000001);

    /* Interrupt every (value * prescaler) timer ticks */
    mmio_write(ARM_TIMER_LOAD, 0x00000400);

    mmio_write(ARM_TIMER_CONTROL,
            (ARM_TIMER_PRESCALE_256 | ARM_TIMER_EN | ARM_TIMER_INT_EN));
}

/**
* @}
*/

/**
* @}
*/
