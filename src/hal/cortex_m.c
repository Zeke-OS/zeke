/**
 *******************************************************************************
 * @file    cortex_m.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for Cortex-M
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Cortex-M
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include "kernel.h"
#include "cortex_m.h"

uint32_t flag_kernel_tick = 0;

void init_hw_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread)
{
    hw_stack_frame_t * thread_frame;

    thread_frame = (hw_stack_frame_t *)((uint32_t)(thread_def->stackAddr)
                    + thread_def->stackSize - sizeof(hw_stack_frame_t));
    thread_frame->r0 = (uint32_t)(argument);
    thread_frame->r1 = 0;
    thread_frame->r2 = 0;
    thread_frame->r3 = 0;
    thread_frame->r12 = 0;
    thread_frame->pc = ((uint32_t)(thread_def->pthread));
    thread_frame->lr = a_del_thread;
    thread_frame->psr = DEFAULT_PSR;
}

/**
  * Make a system call
  */
#pragma optimize=no_code_motion
uint32_t syscall(int type, void * p)
{
    asm volatile(
                 "MOV r2, %0\n" /* Put parameters to r2 & r3 */
                 "MOV r3, %1\n"
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
                 "MOV r4, r1\n" /* Read r4 back from r1 */
                 : "=r" (scratch));

    return scratch;
}

int test_and_set(int * lock) {
    int old_value;

    /* Ensure that all explicit memory accesses that appear in program order
     * before the DMB instruction are observed before any explicit memory
     * accesses. */
    asm volatile("DMB");
    old_value = *lock;
    *lock = 1;

    return old_value == 1;
}

/**
  * @}
  */

/**
  * @}
  */
