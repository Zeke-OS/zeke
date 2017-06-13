/**
 *******************************************************************************
 * @file arm11_thread_yield.S
 * @author Olli Vanhoja
 * @brief Interrupt service routine handlers.
 * @section LICENSE
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy
                            Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <autoconf.h>

    .syntax unified
    .text
    .arch armv6k
    .include "arm11.inc"

@ void hal_thread_yield(void)
@ Yield from a thread.
@
    .globl  hal_thread_yield
    .type   hal_thread_yield,%function
hal_thread_yield:
    stmdb   sp, {r0-r14}
    nop
    sub     sp, sp, #60
    mrs     r5, cpsr        /* Preserve CPSR */
    mov     r6, lr          /* Preserve lr */
    add     r6, #4          /* Add 4 since we are going to substract 4 later */

    /* Get the stack frame pointer */
    mrc     p15, 0, r0, c13, c0, 4

    /* Push preserved registers to the thread stack frame */
    str     r5, [r0]        /* Store CPSR */
    add     r0, r0, #68     /* to the begining of the frame. */
    stmdb   r0!, {r6}       /* Push lr */
    ldmia   sp!, {r1-r11}
    sub     r0, r0, #16
    stmdb   r0, {r1-r11}
    add     r0, r0, #16
    ldmia   sp!, {r1-r4}
    stmdb   r0!, {r1-r4}

    clrex
    cps     #0x12           /* Change to IRQ mode */

    bl      _thread_suspend

    /*
     * Set Context ID to 0 (as we need ASID to be 0 in kernel mode)
     */
    mov     r0, #0
    bl      arm11_set_cid

    /* Run scheduler */
    bl      sched_handler

    /* Update process system state */
    bl      proc_update
    mov     r5, r0          /* New PID is into r5 */

    /* Resume process and attach process page table */
    bl      _thread_resume
    bl      mmu_attach_pagetable

    /* Set process Context ID to the value of current PID */
    mov     r0, r5
    bl      arm11_set_cid
    bl      arm11_set_current_thread_stackframe

/* Load/Restore context */
    popcontext 0            /* Pop context from SCHED_SFRAME_SYS */
    subs    pc, lr, #4      /* Return back (return address is PC+4) */

.end
