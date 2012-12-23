/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Syscall handler
 *
 *******************************************************************************
 */

#include "sched.h"
#include "syscall.h"

/**
  * Make system call
  */
#pragma optimize=no_code_motion
uint32_t syscall(int type, void * p)
{
    asm volatile(
                 "MOV r2, %0\n" /* First parameter (type) */
                 "MOV r3, %1\n" /* Second parameter (p) */
                 "MOV r1, r4\n" /* Preserve r4 by using hardware push */
                 "SVC #0\n"
                 "DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 : : "r" (type), "r" (p));

    /* Get return value */
    osStatus scratch;
    asm volatile("MOV %0, r4\n"
                 "MOV r4, r1\n" /* Load r4 back from r1 */
                 : "=r" (scratch));

    return scratch;
}