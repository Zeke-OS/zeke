/**
 *******************************************************************************
 * @file    cortex_m.h
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

#pragma once
#ifndef CORTEX_M_H
#define CORTEX_M_H

#ifndef __ARM_PROFILE_M__
    #error Only ARM Cortex-M profile is currently supported.
#endif
#ifndef __CORE__
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


/**
  * Init hw stack frame
  */
void init_hw_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread);

/**
  * Request immediate context switch
  */
inline void req_context_switch(void)
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Switch the context */
    asm volatile("DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
    );
}

/**
  * Save the context on the PSP
  */
inline void save_context(void)
{
    volatile uint32_t scratch;
#if __CORE__ == __ARM6M__
    asm volatile ("MRS   %0,  psp\n"
                  "SUBS  %0,  %0, #32\n"
                  "MSR   psp, %0\n"         /* This is the address that will use by rd_thread_stack_ptr(void) */
                  "ISB\n"
                  "STMIA %0!, {r4-r7}\n"
                  "PUSH  {r4-r7}\n"         /* Push original register values so we don't lost them */
                  "MOV   r4,  r8\n"
                  "MOV   r5,  r9\n"
                  "MOV   r6,  r10\n"
                  "MOV   r7,  r11\n"
                  "STMIA %0!, {r4-r7}\n"
                  "POP   {r4-r7}\n"         /* Pop them back */
                  : "=r" (scratch));
#elif __CORE__ == __ARM7M__
    asm volatile ("MRS   %0,  psp\n"
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
#if __CORE__ == __ARM6M__
    asm volatile ("MRS   %0,  psp\n"
                  "ADDS  %0,  %0, #16\n"      /* Move to the high registers */
                  "LDMIA %0!, {r4-r7}\n"
                  "MOV   r8,  r4\n"
                  "MOV   r9,  r5\n"
                  "MOV   r10, r6\n"
                  "MOV   r11, r7\n"
                  "MSR   psp, %0\n"           /* Store the new top of the stack */
                  "ISB\n"
                  "SUBS  r0,  r0, #32\n"      /* Go back to the low registers */
                  "LDMIA %0!, {r4-r7}\n"
                  : "=r" (scratch));
#elif __CORE__ == __ARM7M__
    asm volatile ("MRS   %0,  psp\n"
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
    asm volatile("MRS %0, msp\n"
                 : "=r" (result));
    return result;
}

/**
  * Read the PSP so that it can be stored in the task table
  */
inline void * rd_thread_stack_ptr(void)
{
    void * result = NULL;
    asm volatile ("MRS %0, psp\n" : "=r" (result));
    return(result);
}

/**
  * Write stack pointer of the currentthread to the PSP
  */
inline void wr_thread_stack_ptr(void * ptr)
{
    asm volatile ("MSR psp, %0\n"
                  "ISB\n"
                  : : "r" (ptr));
}

#pragma optimize=no_code_motion
uint32_t syscall(int type, void * p);

#endif /* CORTEX_M_H */

/**
  * @}
  */

/**
  * @}
  */
