/**
 *******************************************************************************
 * @file arm11_int_handlers.S
 * @author Olli Vanhoja
 * @brief Interrupt service routine handlers.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define change_to_tkstack   ldr     sp, =(configTKSTACK_END + 1)

    .syntax unified
    .text
    .arch armv6k
    .include "arm11.inc"

@ void interrupt_svc(void)
@
    .globl  interrupt_svc
    .type   interrupt_svc,%function
interrupt_svc:
    pushcontext #1          /* Push context to SCHED_SFRAME_SVC */
    clrex

    /* Attach kernel master page table */
    enter_kernel
    /* We could actually panic here if attach failed */

    cps     #0x1f           /* Change to system mode */

    /* Change to a kernel stack of the process to protect kernel data. */
    change_to_tkstack

    cpsie   aif             /* Enable interrupts */

    /* Call syscall handler.
     * This will also set the correct return value. */
    bl      syscall_handler

    exit_kernel             /* Attach process master page table */
    cpsid   aif             /* Disable interrupts to prevent stack */
                            /* corruption */
    cps     #0x12           /* Back to IRQ mode */
    popcontext #1           /* Pop context from SCHED_SFRAME_SVC */
    movs    pc, lr          /* Regular exit from svc */


@ void interrupt_sys(void)
@ IRQ handler.
@
    .globl  interrupt_sys
    .type   interrupt_sys,%function
interrupt_sys:
    pushcontext #0          /* Push context to SCHED_SFRAME_SYS */
    clrex

    /* Suspend process and attach kernel master page table */
    attach_kernel_mptable
    bl      _thread_suspend

    /* Handler IRQ */
    bl      schedtimer_clear /* if kernel_tick */
    cmp     r0, #0
    bne     1f
    /* TODO Other interrupts */
    b       2f

    /* Set Context ID to 0 (as we need ASID to be 0 in kernel mode) */
1:  mov     r0, #0
    bl      cpu_set_cid

    /* Run scheduler */
    bl      sched_handler

    /* End of a kernel tick */
    ldr     r5, =flag_kernel_tick
    mov     r0, #0
    str     r0, [r5]

    /* Update process system state */
    bl      proc_update
    mov     r5, r0          /* New PID is into r5 */

    /* Resume process and attach process page table */
    bl      _thread_resume
    bl      mmu_attach_pagetable

    /* Set process Context ID to the value of current PID */
    mov     r0, r5
    bl      cpu_set_cid

/* Load/Restore context */
2:  popcontext #0           /* Pop context from SCHED_SFRAME_SYS */
    subs    pc, lr, #4      /* Return back (return address is PC+4) */


@ Prefetch Abort handler
    .globl  interrupt_pabt
    .type   interrupt_pabt,%function
interrupt_pabt:
    pushcontext #2          /* Push context to SCHED_SFRAME_ABO */
    clrex

    enter_kernel

    cps     #0x1f           /* Change to system mode */
    change_to_tkstack

    bl      mmu_prefetch_abort_handler

    exit_kernel

    cps     #0x17           /* Back to ABT mode */
    popcontext #2           /* Pop context from SCHED_SFRAME_ABO */
    subs    pc, lr, #0x4

@ Data Abort handler
    .globl  interrupt_dabt
    .type   interrupt_dabt,%function
interrupt_dabt:
    pushcontext #2          /* Push context to SCHED_SFRAME_ABO */
    clrex

    enter_kernel

    cps     #0x1f           /* Change to system mode */
    change_to_tkstack
    
    bl      mmu_data_abort_handler

    exit_kernel

    cps     #0x17           /* Back to ABT mode */
    popcontext #2           /* Pop context from SCHED_SFRAME_ABO */
    subs    pc, lr, #0x8

.end
