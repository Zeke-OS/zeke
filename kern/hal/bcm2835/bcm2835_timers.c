/**
 *******************************************************************************
 * @file bcm2835_timers.c
 * @author Olli Vanhoja
 * @brief Timer service routines.
 * @section LICENSE
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kinit.h>
#include <kerror.h>
#include <hal/hw_timers.h>
#include "bcm2835_mmio.h"
#include "bcm2835_interrupt.h"
#include "bcm2835_timers.h"

/* Peripheral Addresses */
#define ARM_TIMER_BASE          0x2000b400
#define ARM_TIMER_LOAD          (ARM_TIMER_BASE + 0x00)
#define ARM_TIMER_VALUE         (ARM_TIMER_BASE + 0x04)
#define ARM_TIMER_CONTROL       (ARM_TIMER_BASE + 0x08)
#define ARM_TIMER_IRQ_CLEAR     (ARM_TIMER_BASE + 0x0c)
#define ARM_TIMER_RAW_IRQ       (ARM_TIMER_BASE + 0x10)
#define ARM_TIMER_MASK_IRQ      (ARM_TIMER_BASE + 0x14)
#define ARM_TIMER_RELOAD        (ARM_TIMER_BASE + 0x18)
#define ARM_TIMER_PREDIV        (ARM_TIMER_BASE + 0x1c)
#define ARM_TIMER_FREERUNCNT    (ARM_TIMER_BASE + 0x20)

#define SYS_TIMER_BASE          0x20003000
#define SYS_TIMER_STATUS        (SYS_TIMER_BASE + 0x00)
#define SYS_TIMER_CLO           (SYS_TIMER_BASE + 0x04)
#define SYS_TIMER_CHI           (SYS_TIMER_BASE + 0x08)
#define SYS_TIMER_C0            (SYS_TIMER_BASE + 0x0c)
#define SYS_TIMER_C1            (SYS_TIMER_BASE + 0x10)
#define SYS_TIMER_C2            (SYS_TIMER_BASE + 0x14)
#define SYS_TIMER_C3            (SYS_TIMER_BASE + 0x18)
/* End of Peripheral Addresses */

#define ARM_TIMER_PRESCALE_1    0x0
#define ARM_TIMER_PRESCALE_16   0x4
#define ARM_TIMER_PRESCALE_256  0x8

#define ARM_TIMER_16BIT         0x0
#define ARM_TIMER_23BIT         0x2

#define ARM_TIMER_EN            0x80
#define ARM_TIMER_INT_EN        0x20

#define SYS_CLOCK               700000 /* kHz */

static int enable_arm_timer(unsigned freq_hz)
{
    istate_t s_entry;
    uint32_t model;

    /*
     * Use the ARM timer - BCM 2832 peripherals doc, p.196
     * Enable ARM timer IRQ
     */
    mmio_start(&s_entry);
    model = mmio_read(ARM_TIMER_IRQ_CLEAR);
    mmio_end(&s_entry);

    if (model != 0x544D5241) {
        KERROR(KERROR_ERR, "BCM2835: No ARM timer found");
        return -ENOTSUP;
    }

    mmio_start(&s_entry);

    /* Interrupt every (value * prescaler) timer ticks */
    mmio_write(ARM_TIMER_LOAD, (SYS_CLOCK / (freq_hz * 16)));
    mmio_write(ARM_TIMER_RELOAD, (SYS_CLOCK / (freq_hz * 16)));
    mmio_write(ARM_TIMER_IRQ_CLEAR, 0);
    mmio_write(ARM_TIMER_CONTROL,
               ARM_TIMER_PRESCALE_16 | ARM_TIMER_EN |
               ARM_TIMER_INT_EN | ARM_TIMER_23BIT);

    /* Enable ARM timer IRQ */
    mmio_write(BCMIRQ_ENABLE_BASIC, BCMIRQ_EN_BASIC_ARM_TIMER);

    mmio_end(&s_entry);

    return 0;
}

static int arm_timer_clear_if_pend(void)
{
    istate_t s_entry;
    int retval = 0;

    /* Handle scheduling timer */
    mmio_start(&s_entry);
    if (mmio_read(ARM_TIMER_MASK_IRQ)) {
        mmio_write(ARM_TIMER_IRQ_CLEAR, 0);
        retval = 1;
    }
    mmio_end(&s_entry);

    return retval;
}

/*
 * Use ARM timer as a scheduling timer for the kernel.
 */
struct hal_schedtimer hal_schedtimer = {
    .enable = enable_arm_timer,
    .disable = NULL, /* TODO ARM timer disable() */
    .reset_if_pending = arm_timer_clear_if_pend,
};

__weak_reference(bcm_udelay, udelay);
void bcm_udelay(uint32_t delay)
{
    volatile uint64_t * ts = (uint64_t *)SYS_TIMER_CLO;
    uint64_t stop = *ts + delay;

    while (*ts < stop)
        __asm__ volatile ("nop");
}

uint64_t get_utime(void)
{
    istate_t s_entry;
    uint64_t now;

    mmio_start(&s_entry);
    now = *((uint64_t *)SYS_TIMER_CLO);
    mmio_end(&s_entry);

    return now;
}
