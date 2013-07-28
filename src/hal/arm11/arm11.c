/**
 *******************************************************************************
 * @file    arm11.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6/ARM11
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM6
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <stddef.h>
#include <sched.h>
#include "../hal_mcu.h"
#include "arm11.h"

uint32_t flag_kernel_tick = 0;

/* Core specific hard fault handlers */
void hard_fault_handler_armv6(uint32_t stack[]);

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
 * @param type syscall type.
 * @param p pointer to the syscall parameter(s).
 * @return return value of the called kernel function.
 * @note Must be only used in thread scope.
 */
uint32_t syscall(uint32_t type, void * p)
{
    __asm__ volatile ("MOV r2, %0\n" /* Put parameters to r2 & r3 */
                      "MOV r3, %1\n"
                      "MOV r1, r4\n" /* Preserve r4 by using hardware push for it */
                      "SVC #0\n"
                      "DSB\n" /* Ensure write is completed
                               * (architecturally required, but not strictly
                               * required for existing Cortex-M processors) */
                      "ISB\n" /* Ensure PendSV is executed */
                      : : "r" (type), "r" (p));

    /* Get return value */
    osStatus scratch;
    __asm__ volatile ("MOV %0, r4\n" /* Return value is now in r4 */
                      "MOV r4, r1\n" /* Read r4 back from r1 */
                      : "=r" (scratch));

    return scratch;
}

/**
 * Test & set function.
 * @param lock pointer to the integer variable that will be set to 1.
 * @return 0 if old value was previously other than 0.
 */
int test_and_set(int * lock) {
    int old_value;

    /* Ensure that all explicit memory accesses that appear in program order
     * before the DMB instruction are observed before any explicit memory
     * accesses. */
    __asm__ volatile ("DMB");
    old_value = *lock;
    *lock = 1;

    return old_value == 1;
}

/* HardFault Handling *********************************************************/

void HardFault_Handler(void)
{
    __asm__ volatile ("TST LR, #4\n"
                      "ITE EQ\n"
                      "MRSEQ R0, MSP\n"
                      "MRSNE R0, PSP\n"
                      "B hard_fault_handler_armv6\n");

    /* If core specific HardFault handler returns it means that we can safely
     * kill the current thread and call the scheduler for reschedule. */

    /* Kill the current thread */
    sched_thread_terminate(current_thread->id);

    /* Return to the scheduler ASAP */
    req_context_switch();
}

/**
 * This function handles the Hard Fault exception on ARMv6M.
 * @param stack top of the stack.
 */
void hard_fault_handler_armv6m(uint32_t stack[])
{
    uint32_t thread_stack;

    __asm__ volatile ("MRS %0, PSP\n"
                      : "=r" (thread_stack));
    if ((uint32_t)stack != thread_stack) {
        /* Kernel fault */
        __asm__ volatile ("BKPT #01");
        while(1);
    }

    /* TODO It's possible to implement a stack dump code here if desired so. */
}

/* End of Fault Handling ******************************************************/

/**
  * @}
  */

/**
  * @}
  */
