/**
 *******************************************************************************
 * @file    arm4t.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv4T/ARM9
 *          TODO THIS PORT IS NON-FUNCTIONAL SCRATC ATM TODO
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Cortex-M
  * @{
  */

#pragma once
#ifndef ARM4T_H
#define ARM4T_H

#include <kernel.h>
#include "../hal_core.h"

#error wtf

/* Exception return values */
#define HAND_RETURN         0xFFFFFFF1u /*!< Return to handler mode using the MSP. */
#define MAIN_RETURN         0xFFFFFFF9u /*!< Return to thread mode using the MSP. */
#define THREAD_RETURN       0xFFFFFFFDu /*!< Return to thread mode using the PSP. */

#define DEFAULT_PSR         0x21000000u

/* Stack frame saved by the hardware */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} hw_stack_frame_t;

/* Stack frame save by the software */
typedef struct {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
} sw_stack_frame_t;

/* Inlined core functions */
inline void save_context(void);
inline void load_context(void);
inline void * rd_stack_ptr(void);
inline void * rd_thread_stack_ptr(void);
inline void wr_thread_stack_ptr(void * ptr);


/**
 * Request immediate context switch
 */
#define req_context_switch() do {                                              \
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Switch the context */              \
    __asm__ volatile ("DSB\n" /* Ensure write is completed
                               * (architecturally required, but not strictly
                               * required for existing Cortex-M processors) */ \
                      "ISB\n" /* Ensure PendSV is executed */                  \
    ); } while (0)

/**
 * Save the context on the PSP
 */
inline void save_context(void)
{
    volatile uint32_t scratch;

    __asm__ volatile ("MRS   %0,  psp\n"
                     "STMDB %0!, {r4-r11}\n"
                     "MSR   psp, %0\n"
                     "ISB\n"
                     : "=r" (scratch));
}

/**
 * Load the context from the PSP
 */
inline void load_context(void)
{
    volatile uint32_t scratch;

    __asm__ volatile ("MRS   %0,  psp\n"
                      "LDMFD %0!, {r4-r11}\n"
                      "MSR   psp, %0\n"
                      "ISB\n"
                      : "=r" (scratch));
}

/**
 * Read the main stack pointer
 */
inline void * rd_stack_ptr(void)
{
    void * result = NULL;
    __asm__ volatile ("MRS %0, msp\n"
                      : "=r" (result));
    return result;
}

/**
 * Read the PSP so that it can be stored in the task table
 */
inline void * rd_thread_stack_ptr(void)
{
    void * result = NULL;
    __asm__ volatile ("MRS %0, psp\n" : "=r" (result));
    return(result);
}

/**
 * Write stack pointer of the currentthread to the PSP
 */
inline void wr_thread_stack_ptr(void * ptr)
{
    __asm__ volatile ("MSR psp, %0\n"
                      "ISB\n"
                      : : "r" (ptr));
}

#endif /* ARM4T_H */

/**
  * @}
  */

/**
  * @}
  */
