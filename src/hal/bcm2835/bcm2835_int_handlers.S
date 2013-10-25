/**
 *******************************************************************************
 * @file bcm2835_int_handlers.S
 * @author Olli Vanhoja
 * @brief Interrupt service routine handlers.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
    bl  _intSyscall_handler

    pop     {r1-r12, lr}
    movs    pc, lr

@ void interrupt_systick(void)
@
    .globl  interrupt_systick
    .type   interrupt_systick,%function
interrupt_systick:
@ Save context
    @ Store the original value of r0
    STMDB   sp!, {r0}
    @ Store thread sp to r0
    STMDB   sp, {sp}^
    NOP
    SUB     sp, sp, #4
    LDMIA   sp!, {r0}
    @ Push lr to the thread stack
    STMDB   r0!, {lr}
    @ Use lr as thread stack pointer and restore the original value of r0
    MOV     lr, r0
    LDMIA   sp!, {r0}
    @ Push usr mode registers to the thread stack
    STMDB   lr, {r0-r14}^
    NOP
    SUB     lr, lr, #60
    @ Push the SPSR to the thread stack
    MRS     r0, spsr
    STMDB   lr!, {r0}

    @ Clear Timer IRQ
    ldr     r0, =0x2000b000     @ Timer register base
    mov     r1, #0
    str     r1, [r0, #0x40c]    @ IRQ Clear/Ack <- 0

    @ Set Context ID to 0 (as we need ASID to be 0)
    mov     r0, #0
    bl      cpu_set_cid

/* TODO Is page table switch always needed? */

    @ Attach kernel master page table
    ldr     r0, =mmu_pagetable_master
    bl      mmu_attach_pagetable

    @ Run scheduler
    bl      sched_handler

    @ Get PID
    ldr     r0, =current_process_id
    ldr     r4, [r0]

    @ Attach process page table
    mov     r0, r4
    bl      process_get_pptable
    bl      mmu_attach_pagetable

    @ Set process Context ID
    mov     r0, r4
    bl      cpu_set_cid

@ Load/Restore context
L_load_context:
    @ Get the thread stack pointer
    STMDB   sp, {sp}^
    NOP
    SUB     sp, sp, #4
    LDMIA   sp!, {lr}
    @ Get the SPSR from the thread stack
    LDMFD   lr!, {r0}
    MSR     spsr, r0
    @ Restore all registers
    LDMFD   lr, {r0-r14}^
    NOP
    @ Restore the return address
    LDR     lr, [lr, #+60]

    @ Return back
    SUBS    pc, lr, #4

.end
