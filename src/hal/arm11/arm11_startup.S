/**
 *******************************************************************************
 * @file    arm11_startup.S
 * @author  Olli Vanhoja
 * @brief   Startup code for ARM11.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 */
Start:
    /* Init stack pointers */
    mov     r4, #0x0 /* Can be changed to something else if memmap is changed. */
    cps     #0x13   /* Change to supervisor (SVC) mode */
    ldr     r5, =0x3000
    add     sp, r4, r5
    cps     #0x17   /* Change to Abort mode */
    ldr     r5, =0x5000
    add     sp, r4, r5
    cps     #0x12   /* Change to IRQ mode */
    ldr     r5, =0x6000
    add     sp, r4, r5
    cps     #0x1b   /* Change to Undef mode */
    ldr     r5, =0x7000
    add     sp, r4, r5
    cps     #0x1f   /* Change to system mode */
    ldr     r5, =0x8000
    add     sp, r4, r5
 
    /* System mode stack is also technically used for user mode but we'll soon
     * have new per thread stacks. Additionally some of the stacks will overlap
     * with ATAGs so we must read ATAGs out before using those stacks. */
    /* Stay in system mode for now. */


    /* Clear the bss. */
    ldr     r4, =_bss_start
    ldr     r9, =_bss_end
    mov     r5, #0
    mov     r6, #0
    mov     r7, #0
    mov     r8, #0
    b       2f
1:  /* Store multiple at r4. */
    stmia   r4!, {r5-r8}
    /* If we are still below bss_end, loop. */
2:  cmp     r4, r9
    blo     1b
    /* End of Clear the bss. */


    /* Read information given by the bootloader
     *
     * In the future we may want to have this information as it tells some
     * things about the system we are booting on.
     * r0 : rpi model/type/fw version?
     * r1 : ARM Linux Machine Type (rpi is 3138 (0xc42) identifying the bcm2708 cpu)
     *      http://www.arm.linux.org.uk/developer/machines/
     * r2 : address of the ATAGs
     *      ATAGs are especially iteresting because this data may change
     *      depending on bootloader configuration etc.
     *      http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html
     */
    @ldr     r3, =3138
    @cmp     r1, r3
    /* TODO Store this number */
    @bne halt

    ldr     r3, =atag_scan
    blx     r3

    /* Call static constructors & initializers */
    ldr     r3, =exec_init_array
    blx     r3

    /* Wait here until scheduler kicks in. */
__wloop:  b __wloop

    /* Halt */
halt:
    wfe
    b       halt

