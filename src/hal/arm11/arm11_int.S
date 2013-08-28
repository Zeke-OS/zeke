/**
 *******************************************************************************
 * @file arm1176jzf_s_interrupt.c
 * @author Olli Vanhoja
 * @brief Interrupt service routines.
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

@ void SVC_Handler(void)
@
    .globl  SVC_Handler
    .type   SVC_Handler,%function
SVC_Handler:
    push    {r1-r12, lr}

    @ Call kernel's internal syscall handler
    bl  _intSyscall_handler

    pop     {r1-r12, lr}
    movs    pc, lr

@ void SysTick_Handler(void)
@
    .globl  SysTick_Handler
    .type   SysTick_Handler,%function
SysTick_Handler:
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

    @ Check if scheduler is enabled
    ldr     r0, .LCPI2_0
    ldr     r0, [r0]
    cmp     r0, #0
    beq     L_load_context
    bl      sched_handler   @ Run scheduler

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

.LCPI2_0:
    .long   sched_enabled

