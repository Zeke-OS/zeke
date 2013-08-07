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

uint32_t syscall(uint32_t type, void * p)
{
    uint32_t scratch;

    __asm__ volatile (
            "MOV    r2, %[typ]\n" /* Put parameters to r2 & r3 */
            "MOV    r3, %[arg]\n"
            "SVC    #0\n"
            "MOV    %[res], r4\n" /* Return value is now in r4 */
            : [res]"=r" (scratch)
            : [typ]"r" (type), [arg]"r" (p)
            : "r2", "r3", "r4");

    return scratch;
}

int test_and_set(int * lock) {
    int err = 2; /* Initial value of error meaning already locked */

    __asm__ volatile (
            "MOV      r1, #1\n"         /* locked value to r1 */
            "LDREX    r2, [%[addr]]\n"  /* load value of lock */
            "CMP      r2, #1\n"         /* if already set */
            "STREXNE  %[res], r1, [%[addr]]\n" /* Sets err = 0 if store op ok */
            : [res]"+r" (err)   /* + makes LLVM think that err is also read in
                                 * the inline asm. Otherwise it would expand
                                 * previous line to:  strexne r2,r1,r2 */
            : [addr]"r" (lock)
            : "r1", "r2"
    );

    return err;
}

/* HardFault Handling *********************************************************/

void HardFault_Handler(void)
{
    /* TODO
    __asm__ volatile (
            "TST LR, #4\n"
            "ITE EQ\n"
            "MRSEQ R0, MSP\n"
            "MRSNE R0, PSP\n"
            "B hard_fault_handler_armv6\n" : : : "r0");
*/
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
/* TODO
    __asm__ volatile ("MRS %0, PSP\n"
            : "=r" (thread_stack)); */
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
