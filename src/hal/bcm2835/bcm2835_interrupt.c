/**
 *******************************************************************************
 * @file arm1176jzf_s_interrupt.c
 * @author Olli Vanhoja
 * @brief Interrupt service routines.
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

/** @addtogroup BCM2835
* @{
*/

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <kstring.h>
#include <kerror.h>
#include <sched.h>
#include <syscall.h>
#include <kinit.h>
#include <hal/hal_core.h>
#include <hal/hal_mcu.h>
#include <hal/mmu.h>
#include "bcm2835_mmio.h"
#include "bcm2835_interrupt.h"

#define ARM_TIMER_PRESCALE_1    0x0
#define ARM_TIMER_PRESCALE_16   0x4
#define ARM_TIMER_PRESCALE_256  0x8

#define ARM_TIMER_16BIT         0x0
#define ARM_TIMER_23BIT         0x2

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

#define SYS_CLOCK       700000 /* kHz */
#define ARM_TIMER_FREQ  configSCHED_HZ

void interrupt_preinit(void);
void interrupt_postinit(void);

HW_PREINIT_ENTRY(interrupt_preinit);
HW_POSTINIT_ENTRY(interrupt_postinit);

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
    __asm__ volatile (          /* Event                    Pri LnAddr Mode */
        "b bad_exception\n\t"   /* Reset                    1   8      abt  */
        "b undef_handler\n\t"   /* Undefined instruction    6   0      und  */
        "b interrupt_svc\n\t"   /* Software interrupt       6   0      svc  */
        "b interrupt_pabt\n\t"  /* Prefetch abort           5   4      abt  */
        "b interrupt_dabt\n\t"  /* Data abort               2   8      abt  */
        "b bad_exception\n\t"   /* Unused vector                            */
        "b interrupt_sys\n\t"   /* IRQ                      4   4      irq  */
        "b bad_exception\n\t"   /* FIQ                      3   4      fiq  */
    );
}

/**
 * Unhandled exception
 */
__attribute__ ((naked)) void bad_exception(void)
{
    KERROR(KERROR_CRIT, "This is like panic but unexpected.");
    while (1) {
        __asm__ volatile ("wfe");
    }
}

extern volatile uint32_t flag_kernel_tick;
void interrupt_clear_timer(void)
{
    uint32_t val;

    mmio_start();
    val = mmio_read(ARM_TIMER_VALUE);
    if (val == 0) {
        mmio_write(ARM_TIMER_IRQ_CLEAR, 0);
        mmio_end();
        //bcm2835_uputc('C'); /* Timer debug print */
        flag_kernel_tick = 1;
    } else mmio_end();
}

void interrupt_preinit(void)
{
    SUBSYS_INIT();
    KERROR(KERROR_LOG, "Enabling interrupts");

    /* Set interrupt base register */
    __asm__ volatile ("mcr p15, 0, %[addr], c12, c0, 0"
            : : [addr]"r" (&interrupt_vectors));
    /* Turn on interrupts */
    __asm__ volatile ("cpsie i");
}

void interrupt_postinit(void)
{
    KERROR(KERROR_LOG, "Starting ARM timer");

    mmio_start();

    /* Use the ARM timer - BCM 2832 peripherals doc, p.196 */
    /* Enable ARM timer IRQ */
    mmio_write(IRQ_ENABLE_BASIC, 0x00000001);

    /* Interrupt every (value * prescaler) timer ticks */
    mmio_write(ARM_TIMER_LOAD, (SYS_CLOCK / (ARM_TIMER_FREQ * 16)));

    mmio_write(ARM_TIMER_CONTROL,
            (ARM_TIMER_PRESCALE_16 | ARM_TIMER_EN | ARM_TIMER_INT_EN | ARM_TIMER_23BIT));

    KERROR(KERROR_DEBUG, "OK");
}

/**
* @}
*/

/**
* @}
*/
