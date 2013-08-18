/**
 *******************************************************************************
 * @file    startup_bcm2835.s
 * @author  Olli Vanhoja
 * @brief   BCM2835 startup code.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/* To keep this in the first portion of the binary. */
.section ".text.boot"

.globl Start

/* Entry point for the kernel.
 * r15 should begin execution at 0x8000.
 */
Start:
    /* Setup the stack. */
    mov sp, #0x8000

    /* Clear out bss. */
    ldr r4, =_bss_start
    ldr r9, =_bss_end
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b   2f

1:
    /* Store multiple at r4. */
    stmia   r4!, {r5-r8}

    /* If we are still below bss_end, loop. */
2:
    cmp r4, r9
    blo 1b

    /* TODO Call something to preserve r0-r2 for further use */

    /* Call static constructors */
    ldr r3, =exec_init_array
    blx r3

    /* Call kernel init */
    ldr r3, =kinit
    blx r3

    /* Start scheduler */
    ldr r3, =sched_start
    blx r3

/* Loop/wait here until scheduler kicks in. */
3:
    b 3b

    /* Halt */
halt:
    wfe
    b   halt
