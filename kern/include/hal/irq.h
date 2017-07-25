/**
 *******************************************************************************
 * @file    irq.h
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


#define NR_IRQ 64

enum irq_ack {
    IRQ_HANDLED = 0,
    IRQ_NEEDS_HANDLING,
    IRQ_WAKE_THREAD
};

/**
 * IRQ handler descriptor.
 */
struct irq_handler {
    /**
     * IRQ verify, ack & clear function for threaded handlers.
     */
    enum irq_ack (*ack)(int irq);
    /**
     * IRQ handler callback.
     */
    void (*handle)(int irq);

    struct {
        unsigned allow_multiple : 1; /*!< Allow multiple IRQs to be received for
                                      *   a threaded handler. */
    } flags; /*!< IRQ handler control flags */

    unsigned int cnt; /*!< Interrupts received count. */
    char name[]; /*!< Name of the handler/IRQ. Should be incremented by the
                  *   HW specific IRQ resolver. */
};

/**
 * An array of interrupt handlers.
 */
extern struct irq_handler * irq_handlers[NR_IRQ];

/**
 * Enable irq.
 * HW specific.
 */
void irq_enable(int irq);

/**
 * Disable irq.
 * HW specific.
 */
void irq_disable(int irq);

/**
 * Register an interrupt handler.
 */
int irq_register(int irq, struct irq_handler * handler);

/**
 * Deregister an interrupt handler.
 */
int irq_deregister(int irq);

/**
 * Postpone IRQ handling to the threaded IRQ handler.
 */
void irq_thread_wakeup(int irq);
