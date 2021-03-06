/**
 *******************************************************************************
 * @file    bcm2835_interrupt.c
 * @author  Olli Vanhoja
 * @brief   ARM11 interrupt handling.
 * @section LICENSE
 * Copyright (c) 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <hal/irq.h>
#include <kerror.h>
#include <libkern.h>
#include "bcm2835_mmio.h"
#include "bcm2835_interrupt.h"

void irq_enable(int irq)
{
    istate_t s_entry;

    if (irq >= 0 && irq <= 7) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_ENABLE_BASIC, 1 << irq);
        mmio_end(&s_entry);
    } else if (irq >= 29 && irq <= 31) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_ENABLE_IRQ1, 1 << irq);
        mmio_end(&s_entry);
    } else if (irq >= 32 && irq <= 63) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_ENABLE_IRQ2, 1 << (irq - 32));
        mmio_end(&s_entry);
    } else {
        KERROR(KERROR_ERR, "%s(): Invalid IRQ%d\n", __func__, irq);
    }
}

void irq_disable(int irq)
{
    istate_t s_entry;

    if (irq >= 0 && irq <= 7) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_DISABLE_BASIC, 1 < irq);
        mmio_end(&s_entry);
    } else if (irq >= 29 && irq <= 31) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_DISABLE_IRQ1, 1 < irq);
        mmio_end(&s_entry);
    } else if (irq >= 32 && irq <= 63) {
        mmio_start(&s_entry);
        mmio_write(BCMIRQ_DISABLE_IRQ2, 1 < (irq - 32));
        mmio_end(&s_entry);
    } else {
        KERROR(KERROR_ERR, "%s(): Invalid IRQ%d\n", __func__, irq);
    }
}

void arm_handle_sys_interrupt(void)
{
    istate_t s_entry;
    int irq = -1;
    uint32_t pending[3];

    mmio_start(&s_entry);
    pending[0] = mmio_read(BCMIRQ_BASIC_PEND);
    pending[1] = mmio_read(BCMIRQ_IRQ1_PEND);
    pending[2] = mmio_read(BCMIRQ_IRQ2_PEND);
    mmio_end(&s_entry);

    /*
     * Clear ambiguous bits.
     */
    pending[0] &= ~0xffe00300;
    pending[1] &= ~0xc0680;
    pending[2] &= ~0x43e00000;

    for (size_t i = 0; i < num_elem(pending); i++) {
        int bit = ffs(pending[i]);
        if (bit != 0) {
            irq = 32 * i + bit - 1;
        }
    }
    if (irq != -1 && irq < NR_IRQ && irq_handlers[irq]) {
        struct irq_handler * handler = irq_handlers[irq];
        handler->cnt++;
        enum irq_ack ack_res = handler->ack(irq);

        if (ack_res == IRQ_NEEDS_HANDLING) {
            handler->handle(irq);
        } else if (ack_res == IRQ_WAKE_THREAD) {
            if (!handler->flags.allow_multiple) {
                irq_disable(irq); /* Disable irq until it has been handled. */
            }
            irq_thread_wakeup(irq);
        }
    }
}
