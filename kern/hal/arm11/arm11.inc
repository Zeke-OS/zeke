/**
 *******************************************************************************
 * @file arm11.inc
 * @author Olli Vanhoja
 * @brief ARM11 macros.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
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

/**
 * Save thread context.
 * @param ind is the stack frame index.
 */
.macro pushcontext ind
    stmdb   sp, {r0-r14}^
    nop
    sub     sp, sp, #60
    mrs     r5, spsr        /* Preserve SPSR */
    mov     r6, lr          /* Preserve lr */
    mov     r0, \ind
    bl      thread_get_curr_stackframe
    cmp     r0, #0
    addeq   sp, sp, #60     /* Clear stack as we are skipping... */
    beq     1f              /* push if there is no stack frame. */
    str     r5, [r0]        /* Store SPSR */
    add     r0, r0, #68     /* to the begining of the frame. */
    stmdb   r0!, {r6}       /* Push lr */
    ldmia   sp!, {r1-r11}
    sub     r0, r0, #16
    stmdb   r0, {r1-r11}
    add     r0, r0, #16
    ldmia   sp!, {r1-r4}
    stmdb   r0!, {r1-r4}
1:  nop
.endm

/**
 * Load thread context.
 * @param ind is the stack frame index.
 * @return Returns original lr in lr.
 */
.macro popcontext ind
    mov     r0, \ind
    bl      thread_get_curr_stackframe
    mov     lr, r0
    add     lr, #4

    /* Restore SPSR */
    ldr     r0, [r0]
    msr     spsr, r0

    /* Restore all registers */
    ldmia   lr, {r0-r14}^
    nop
    ldr     lr, [lr, #60]  /* Restore lr */
.endm

/**
* Attach kernel master page table.
*/
.macro attach_kernel_mptable
    ldr     r0, =mmu_pagetable_master
    bl      mmu_attach_pagetable
.endm

/**
 * Enter to kernel mode (syscall).
 */
.macro enter_kernel
    attach_kernel_mptable
    bl      _thread_enter_kernel
.endm

/**
 * Exit from kernel mode (syscall).
 */
.macro exit_kernel
    bl      _thread_exit_kernel
    bl      mmu_attach_pagetable
    clrex
.endm
