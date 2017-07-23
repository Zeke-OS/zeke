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
#include "bcm2835_mmio.h"
#include "bcm2835_interrupt.h"

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

    for (size_t i = 0; i < num_elem(pending); i++) {
        for (int j = 0; j < 32; j++) {
            if (((pending[i] >> j) & 1) != 0) {
                irq = 32 * i + j;
                break;
            }
        }
    }
    if (irq != -1 && irq < NR_IRQ && irq_handlers[irq]) {
        struct irq_handler * handler = irq_handlers[irq];
        if (handler->flags.fast_irq) {
            handler->fn(irq);
        } else {
            /* TODO Implement lazy IRQ handling */
        }
    }
}
