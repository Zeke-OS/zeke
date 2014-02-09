/**
 *******************************************************************************
 * @file arm11.inc
 * @author Olli Vanhoja
 * @brief ARM11 macros.
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

/**
 * Save thread context.
 * @return Returns thread sp in r0 and spsr in r1.
 */
.macro pushcontext
    /* Store the original value of r0 */
    stmdb   sp!, {r0} 
    /* Store thread sp to r0 */
    stmdb   sp, {sp}^
    nop
    sub     sp, sp, #4
    ldmia   sp!, {r0}
    /* Push lr to the thread stack */
    stmdb   r0!, {lr}
    /* Use lr as thread stack pointer and restore the original value of r0 */
    mov     lr, r0
    ldmia   sp!, {r0}
    /* Push usr mode registers to the thread stack */
    stmdb   lr, {r0-r14}^
    nop
    sub     lr, lr, #60
    /* Push the SPSR to the thread stack */
    mrs     r1, spsr
    stmdb   lr!, {r1}

    /* Thread sp to r0 */
    mov     r0, lr
.endm

/**
 * Load thread context.
 * @return Returns thread lr in lr.
 */
.macro popcontext
    /* Get the thread stack pointer */
    stmdb   sp, {sp}^
    nop
    sub     sp, sp, #4
    ldmia   sp!, {lr}

    /* Get the SPSR from the thread stack */
    ldmia   lr!, {r0}
    msr     spsr, r0

    /* Restore all registers */
    ldmia   lr, {r0-r14}^
    nop

    /* Restore the return address */
    ldr     lr, [lr, #+60]
.endm

/**
 * Attach process master page table.
 * @param r0 process id.
 */
.macro attach_proc_mptable
    bl      proc_get_pptable
    bl      mmu_attach_pagetable
.endm

/**
 * Attach kernel master page table.
 */
.macro attach_kernel_mptable
    ldr     r0, =mmu_pagetable_master
    bl      mmu_attach_pagetable
.endm

