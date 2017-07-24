/**
 *******************************************************************************
 * @file    irq.c
 * @author  Olli Vanhoja
 * @brief   Generic interrupt handling.
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

#include <errno.h>
#include <sched.h>
#include <bitmap.h>
#include <hal/irq.h>
#include <kinit.h>
#include <thread.h>

static bitmap_t irq_pending[E2BITMAP_SIZE(NR_IRQ)];
static pthread_t irq_handler_tid;
struct irq_handler * irq_handlers[NR_IRQ];

int irq_register(int irq, struct irq_handler * handler)
{
    if (irq < 0 || irq >= NR_IRQ)
        return -EINVAL;

    /* A threaded IRQ handler must specify an ACK function. */
    if (!handler->flags.fast_irq && !handler->ack) {
        return -EINVAL;
    }

    if (irq_handlers[irq])
        return -EBUSY;

    irq_handlers[irq] = handler;

    return 0;
}

int irq_deregister(int irq)
{
    if (irq < 0 || irq >= NR_IRQ)
        return -EINVAL;

    irq_handlers[irq] = NULL;

    return 0;
}

void irq_threaded_wakeup(int irq)
{
    bitmap_set(irq_pending, irq, sizeof(irq_pending));
    thread_release(irq_handler_tid);
}

static void * irq_handler_thread(void * arg)
{
    while (1) {
        thread_wait(); /* Wait until the HW specific handler calls
                        * irq_threaded_wakeup().
                        */

        for (int irq = 0; irq < NR_IRQ; irq++) {
            /* NOTE: Ignoring errors */
            if (bitmap_status(irq_pending, irq, sizeof(irq_pending))) {
                struct irq_handler * handler = irq_handlers[irq];
                handler->handle(irq);
                bitmap_clear(irq_pending, irq, sizeof(irq_pending));
            }
        }
    }
}

static int __kinit__ irq_init(void)
{
    SUBSYS_INIT("irq");

    struct sched_param param = {
        .sched_policy = SCHED_FIFO,
        .sched_priority = NICE_MIN,
    };
    irq_handler_tid = kthread_create(&param, 0, irq_handler_thread, NULL);
    if (irq_handler_tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a thread for IRQ handling");
        return irq_handler_tid;
    }

    return 0;
}
