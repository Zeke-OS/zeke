/**
 *******************************************************************************
 * @file    arm6.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for ARMv6 architecture
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Cortex-M
  * @{
  */

#pragma once
#ifndef CORTEX_M_H
#define CORTEX_M_H

#include "kernel.h"
#include "hal_mcu.h" /* Needed for CMSIS */
#include "hal_core.h"

#ifdef configARM_PROFILE_M
    #error ARM Cortex-M profile is not supported by this layer.
#endif
#ifndef configARCH
    #error Core is not selected by the compiler.
#endif

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
 * Disable all interrupts except NMI (set PRIMASK)
 */
#define disable_interrupt() do {                                               \
    __asm__ volatile ("CPSID i\n"); } while (0)

/**
 * Enable interrupts (clear PRIMASK)
 */
#define enable_interrupt() do {                                                \
    __asm__ volatile ("CPSIE i\n"); } while (0)

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
#if configARCH == __ARM6M__
    __asm__ volatile ("MRS   %0,  psp\n"
                      "SUBS  %0,  %0, #32\n"
                      "MSR   psp, %0\n"         /* This is the address that will
                                                 * be used by rd_thread_stack_ptr(void)
                                                 */
                      "ISB\n"
                      "STMIA %0!, {r4-r7}\n"
                      "PUSH  {r4-r7}\n"         /* Push original register values
                                                 * so we don't lost them */
                      "MOV   r4,  r8\n"
                      "MOV   r5,  r9\n"
                      "MOV   r6,  r10\n"
                      "MOV   r7,  r11\n"
                      "STMIA %0!, {r4-r7}\n"
                      "POP   {r4-r7}\n"         /* Pop them back */
                      : "=r" (scratch));
#elif configARCH == __ARM7M__
    __asm__ volatile ("MRS   %0,  psp\n"
                      "STMDB %0!, {r4-r11}\n"
                      "MSR   psp, %0\n"
                      "ISB\n"
                      : "=r" (scratch));
#else
    #error Selected CORE not supported
#endif
}

/**
 * Load the context from the PSP
 */
inline void load_context(void)
{
    volatile uint32_t scratch;
#if configARCH == __ARM6M__
    __asm__ volatile ("MRS   %0,  psp\n"
                      "ADDS  %0,  %0, #16\n"    /* Move to the high registers */
                      "LDMIA %0!, {r4-r7}\n"
                      "MOV   r8,  r4\n"
                      "MOV   r9,  r5\n"
                      "MOV   r10, r6\n"
                      "MOV   r11, r7\n"
                      "MSR   psp, %0\n"         /* Store the new top of the stack */
                      "ISB\n"
                      "SUBS  r0,  r0, #32\n"    /* Go back to the low registers */
                      "LDMIA %0!, {r4-r7}\n"
                      : "=r" (scratch));
#elif configARCH == __ARM7M__
    __asm__ volatile ("MRS   %0,  psp\n"
                      "LDMFD %0!, {r4-r11}\n"
                      "MSR   psp, %0\n"
                      "ISB\n"
                      : "=r" (scratch));
#else
    #error Selected CORE not supported
#endif
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
    __asm__ volatile ("MRS %0, psp\n"
                      : "=r" (result));
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

/**
 * Platform sepcific idle sleep mode
 */
#define idle_sleep() do {                                 \
    __asm__ volatile ("WFI\n" /* Sleep until next interrupt */ \
                     ); } while (0)


void HardFault_Handler(void);

#endif /* CORTEX_M_H */

/**
  * @}
  */

/**
  * @}
  */
