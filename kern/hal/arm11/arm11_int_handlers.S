/**
 *******************************************************************************
 * @file arm11_int_handlers.S
 * @author Olli Vanhoja
 * @brief Interrupt service routine handlers.
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

#include <autoconf.h>

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

    /* Attach kernel master page table */
    enter_kernel
    /* We could actually panic here if attach failed */

    cps     #0x1f           /* Change to system mode */

    /* Change to a kernel stack of the process to protect kernel data.
     * RFE This breaks nested syscalls.
     * Solution: Check if earlier mode was other than usr and don't change
     * stack address if so.
     */
    ldr     sp, =0x00001000 /* vaddr of tkstack. */

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
@ Systick handler
@
    .globl  interrupt_sys
    .type   interrupt_sys,%function
interrupt_sys:
    pushcontext #0          /* Push context to SCHED_SFRAME_SYS */

    /* Handle kernel_tick & Clear Timer IRQ if this is a real tick */
    bl      schedtimer_clear

    /* Set Context ID to 0 (as we need ASID to be 0 in kernel mode) */
    mov     r0, #0
    bl      cpu_set_cid

    /* Suspend process and attach kernel master page table */
    bl      proc_suspend
    attach_kernel_mptable

    /* Get current PID */
    /*ldr     r5, =current_process_id
    ldr     r5, [r5]*/

    /* Run scheduler */
    bl      sched_handler

    /* End of kernel tick */
    ldr     r5, =flag_kernel_tick
    mov     r0, #0
    str     r0, [r5]

    /* Update process system state */
    bl      proc_update
    mov     r5, r0          /* New PID is into r5 */

    /* Resume process and attach process page table */
    bl      proc_resume
    bl      mmu_attach_pagetable

    /* Set process Context ID to the value of current PID */
    mov     r0, r5
    bl      cpu_set_cid

/* Load/Restore context */
    popcontext #0           /* Pop context from SCHED_SFRAME_SYS */
    subs    pc, lr, #4      /* Return back (return address is PC+4) */


@ Prefetch Abort handler
    .globl  interrupt_pabt
    .type   interrupt_pabt,%function
interrupt_pabt:
_tag:                       /* TODO */
    b       _tag
    subs    pc, lr, #0x4


@ Data Abort handler
    .globl  interrupt_dabt
    .type   interrupt_dabt,%function
interrupt_dabt:
    push    {r0-r3}
    mrs     r1, spsr
    push    {r1}
    and     r0, r1, #0x1f   /* Get only the mode part. */
    tst     r0, #0x10       /* User mode */
#if 0 /* TODO How to resolve if we came from a syscall or sys thread? */
    tstne   r0, #0x1f       /* System mode */
#endif
    movne   r4, #1          /* 1 = Skip restoring of thread context */
    movne   r0, sp          /* arg0 = sp TODO Incorrect as this is not the stack
                             * of the trouble maker. */
    movne   r2, lr          /* arg2 = lr */
    bne     1f              /* DAB came possibly from an interrupt handler. */

    /* Store thread context */
    mov     r1, sp
    ldr     r0, =0x00001000 /* vaddr of tkstack. */
    add     sp, sp, #16     /* Reset stack pointer */
    cps     #0x1f           /* Change to system mode. */
    mov     r3, sp          /* Preserve usr sp */
    /* At this point we have:
     * r0 = thread specific kernel stack
     * r1 = irq stack
     * r2 = original link register value
     * sp = usr stack
     */
    stmdb   r0!, {sp}       /* Store usr sp to the kstack */
    stmdb   r0!, {r4-r12}   /* Store r4-r12 to the kstack */
    ldmia   r1!, {r8}       /* Pop spsr */
    ldmia   r1!, {r4-r7}    /* Pop r0-r3 TODO remove ! if not needed */
    stmdb   r0!, {r4-r7}    /* Store original values of r0-r3 to the kstack */
    stmdb   r0!, {r8}       /* Store spsr to kstack */
    mov     r1, r8          /* spsr to r1 */
    mov     r4, #0          /* #0 = Dont skip restoring of the thread context */
    eor     r0, r0, sp      /* Swap r0 <-> sp */
    eor     r0, r0, sp
    eor     r0, r0, sp
    /* by now usr sp in r0, spsr in r1 and lr in r2 */
1:  bl      mmu_data_abort_handler
    mov     lr, r0          /* Restore lr */
    tst     r4, #1          /* Skip? */
    beq     2f

    /* Restore thread user context, Lets assume that we are still in sys mode */
    ldmia   sp!, {r0}
    msr     spsr, r0
    ldmia   sp!, {r0-r3}
    ldmia   sp!, {r4-r12}
    mov     sp, sp          /* Get usr sp. State of kstack is irrelevant after
                             * this interrupt handler returns. */
    subs    pc, lr, #0x8
2:  pop     {r0}            /* Return */
    msr     spsr, r0
    pop     {r0-r3}
    subs    pc, lr, #0x8

.end
