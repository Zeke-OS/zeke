/**
 *******************************************************************************
 * @file bcm2835_int_handlers.S
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


    .syntax unified
    .text
    .arch armv6k

@ void interrupt_svc(void)
@
    .globl  interrupt_svc
    .type   interrupt_svc,%function
interrupt_svc:
    push    {r1-r12, lr}

    @ Call kernel's internal syscall handler
    bl      _intSyscall_handler

    cmp     r1, #1              @ On ARM11 r1 is set to 1 if syscall was
                                @ blocking and otherwise 0.
    pop     {r1-r12, lr}
    beq     interrupt_systick   @ Jump if thread was blocked
    movs    pc, lr

@ void interrupt_systick(void)
@
    .globl  interrupt_systick
    .type   interrupt_systick,%function
interrupt_systick:
@ Save context
    @ Store the original value of r0
    stmdb   sp!, {r0}
    @ Store thread sp to r0
    stmdb   sp, {sp}^
    nop
    sub     sp, sp, #4
    ldmia   sp!, {r0}
    @ Push lr to the thread stack
    stmdb   r0!, {lr}
    @ Use lr as thread stack pointer and restore the original value of r0
    mov     lr, r0
    ldmia   sp!, {r0}
    @ Push usr mode registers to the thread stack
    stmdb   lr, {r0-r14}^
    nop
    sub     lr, lr, #60
    @ Push the SPSR to the thread stack
    mrs     r1, spsr
    stmdb   lr!, {r1}
 
    @ Thread sp to r0
    mov     r0, lr

    @ Check if we are here just after kinit
    and     r2, r1, #0x1f
    cmp     r2, #0x1f
    moveq   r0, #0
    bleq    sched_handler   @ Get initial value for thread sp
    @ Update thread stack pointer
    bl      wr_thread_stack_ptr

    @ Clear Timer IRQ
    bl      interrupt_clear_timer

    @ This is a kernel tick
    ldr     r5, =flag_kernel_tick
    mov     r0, #1
    str     r0, [r5]

    @ Set Context ID to 0 (as we need ASID to be 0 in kernel mode)
    mov     r0, #0
    bl      cpu_set_cid

    @ Attach kernel master page table
    ldr     r0, =mmu_pagetable_master
    bl      mmu_attach_pagetable

    @ Get current PID
    ldr     r4, =current_process_id
    ldr     r4, [r4]

    @ Run scheduler
    bl      rd_thread_stack_ptr
    bl      sched_handler
    bl      wr_thread_stack_ptr

    @ End of kernel tick
    mov     r0, #0
    str     r0, [r5]

    @ Update process system state
    bl      process_update

    @ Get newly selected PID
    ldr     r0, =current_process_id
    ldr     r5, [r0]

    @ Attach process page table
    mov     r0, r5
    bl      process_get_pptable
    bl      mmu_attach_pagetable

    @ Set process Context ID to the value of current PID
    mov     r0, r5
    bl      cpu_set_cid

@ Load/Restore context
    .globl  load_context
    .type   load_context,%function
load_context:
    @ Get the thread stack pointer
    stmdb   sp, {sp}^
    nop
    sub     sp, sp, #4
    ldmia   sp!, {lr}

    @ Get the SPSR from the thread stack
    ldmia   lr!, {r0}
    msr     spsr, r0

    @ Restore all registers
    ldmia   lr, {r0-r14}^
    nop

    ldr     lr, [lr, #+60]  @ Restore the return address
/*  For debuging
    stmdb   sp!, {r0}
    ldr     r0, =main
    cmp     lr, r0
tag: bne    tag
    ldmia   sp!, {r0}
*/
    subs    pc, lr, #4      @ Return back

.end
