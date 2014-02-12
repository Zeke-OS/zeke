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
#if configPREEMPT != 0
    /* In Pre-empt mode we use thread stack also in syscalls to allow syscalls
     * to be pre-empted at almost any time. So next up we'll swap back to thread
     * stack pointer and restore svc sp later after the call.
     * TODO We may wan't to update frame pointer here to help debugging.
     */
    /* Store thread sp to r4 as we know for sure that it's safe to use
     * r4 for anything in syscall due to our special ABI.
     * TODO This could be actually optimized to use r3 and remove clobber from
     *      syscall. */
    mov     r4, lr              /* Preserve link register. */
    cps     #0x1f               /* Change to system mode to use thread stack. */
    mov     lr, r4              /* Restore link register. */

    /* Store spsr to allow nested swi.
     * We don't encourage to use nested swi calls but oh well... */
    mrs     r4, spsr
    push    {r4}
 
    /*cpsie   aif*/                 /* Enable interrupts */
#endif

    push    {r1-r12, lr}

    /* Attach kernel master page table */
    push    {r0-r3}             /* Preserve syscall argument(s) */
    attach_kernel_mptable
    /* We could actually panic here if attach failed */
    pop     {r0-r3}

    /* Call kernel's internal syscall handler */
    bl      _intSyscall_handler

    cmp     r1, #1              /* On ARM11 r1 is set to 1 if syscall was
                                 * blocking and otherwise 0. */
    bne     Lret_svc            /* Syscall was not blocking so we can return. */
    pop     {r1-r12, lr}
#if configPREEMPT != 0
    cpsid   aif                 /* Disable interrupts to prevent stack
                                 * corruption */
    pop     {r4}                /* Restore spsr */
    msr     spsr, r4

    mov     r4, lr
    cps     #0x12               /* Change to IRQ mode. */
    mov     lr, r4
#endif
    add     lr, lr, #4          /* interrupt_sys will expect this... */
    b       interrupt_sys       /* Syscall was blocking. */

Lret_svc:
    mov     r4, r0              /* Preserve syscall retval */

    /* Attach process master page table */
    ldr     r0, =current_process_id
    ldr     r0, [r0]
    attach_proc_mptable

    mov     r0, r4              /* Restore syscall retval */
    pop     {r1-r12, lr}

#if configPREEMPT != 0
    cpsid   aif                 /* Disable interrupts to prevent stack */
                                /* corruption */
    pop     {r4}                /* Restore spsr */
    msr     spsr, r4
#endif
    movs    pc, lr


@ void interrupt_sys(void)
@ Systick handler
@
    .globl  interrupt_sys
    .type   interrupt_sys,%function
interrupt_sys:
    pushcontext

#if 0
    /* Check if we are here just after kinit */
    and     r2, r1, #0x1f   /* 0x1f == system mode */
    cmp     r2, #0x1f
    moveq   r0, #0
    bleq    sched_handler   /* Get initial value for thread sp */
#endif
    /* Update thread stack pointer */
    bl      wr_thread_stack_ptr

    /* Handle kernel_tick & Clear Timer IRQ if this is a real tick */
    bl      interrupt_clear_timer

    /* Set Context ID to 0 (as we need ASID to be 0 in kernel mode) */
    mov     r0, #0
    bl      cpu_set_cid

    /* Attach kernel master page table */
    attach_kernel_mptable

    /* Get current PID */
    ldr     r5, =current_process_id
    ldr     r5, [r5]

    /* Run scheduler */
    bl      rd_thread_stack_ptr
    bl      sched_handler
    bl      wr_thread_stack_ptr

    /* End of kernel tick */
    ldr     r5, =flag_kernel_tick
    mov     r0, #0
    str     r0, [r5]

    /* Update process system state */
    bl      proc_update

    /* Get newly selected PID */
    ldr     r0, =current_process_id
    ldr     r5, [r0]

    /* Attach process page table */
    mov     r0, r5
    attach_proc_mptable

    /* Set process Context ID to the value of current PID */
    mov     r0, r5
    bl      cpu_set_cid

/* Load/Restore context */
    popcontext

/*  For debuging
    stmdb   sp!, {r0}
    ldr     r0, =main
    cmp     lr, r0
tag: bne    tag
    ldmia   sp!, {r0}
*/
    subs    pc, lr, #4      /* Return back (SWI return address is PC+4) */


@ Prefetch Abort handler
    .globl  interrupt_pabt
    .type   interrupt_pabt,%function
interrupt_pabt:
    pushcontext
_tag:                       /* TODO */
    b       _tag
    popcontext
    subs    pc, lr, #0x4


@ Data Abort handler
    .globl  interrupt_dabt
    .type   interrupt_dabt,%function
interrupt_dabt:
    pushcontext    
    bl      mmu_data_abort_handler
    popcontext
    subs    pc, lr, #0x8    /* Return back */

.end
