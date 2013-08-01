/**
 *******************************************************************************
 * @file    startup_bcm2835.s
 * @author  Olli Vanhoja
 * @brief   BCM2835 startup code.
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

    /* TODO init data sections? */

    /* Call static constructors */
    ldr r3, =__libc_init_array
    blx r3

    /* Call kernel init */
    ldr r3, =kinit
    blx r3
 
    /* Halt */
halt:
    wfe
    b   halt
