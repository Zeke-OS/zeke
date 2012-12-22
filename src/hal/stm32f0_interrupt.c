/**
 *******************************************************************************
 * @file    stm32f0_interrupt.c
 * @author  Olli Vanhoja
 * @brief Interrupt service routines.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup STM32F0
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif


#include "sched.h"
#include "syscall.h"
#include "kernel_config.h"
#include "stm32f0_interrupt.h"

#define SYSTICK_DIVIDER         SystemCoreClock / configSCHED_FREQ

int interrupt_init_module(void)
{
    NVIC_SetPriority(PendSV_IRQn, 0x03); /* Set PendSV to lowest level */

    /* Setup SysTick Timer for tLevel ms interrupts  */
    if (SysTick_Config(SystemCoreClock / SYSTICK_DIVIDER))
    {
        /* Capture error */
        while (1);
    }

    /* Configure the SysTick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x03);

    return 0;
}

/**
  * This function handles NMI exception.
  */
void NMI_Handler(void)
{
    /* @todo fixme */
}

/**
  * This function handles Hard Fault exception.
  */
void HardFault_Handler(void)
{
    /* @todo fixme */
    while (1)
    { }
}

/**
  * This function handles SVCall exception.
  */
#pragma optimize = no_cse
void SVC_Handler(void)
{
    int type;
    void * p;

    asm volatile("MOV %0, r1\n"
                 "MOV %1, r2\n"
                     : "=r" (type), "=r" (p));

    /* Call kernel internal syscall handler */
    uint32_t result = _intSyscall_handler(type, p);

    /* This is the return value */
    asm volatile("MOV r3, %0\n"
                 : : "r" (result));
}

/**
  * This function handles PendSVC exception.
  */
void PendSV_Handler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        void * result = NULL;
        asm volatile("MRS %0, msp\n"
                     : "=r" (result));

        sched_handler(result);
        asm volatile ("POP {r0}\n"
                      "ADD sp, sp, #(4)\n"
                      "BX  r0");
    }
}

/**
  * This function handles SysTick Handler.
  */
void SysTick_Handler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        void * result = NULL;
        asm volatile("MRS %0, msp\n"
                     : "=r" (result));

        sched_handler(result);
        asm volatile ("POP {r0}\n"
                      "ADD sp, sp, #(4)\n"
                      "BX  r0");
    }
}

/**
  * This function handles USART interrupt request.
  */
void USART1_IRQHandler(void)
{
    /* @todo fixme */
}

/**
  * @}
  */

/**
  * @}
  */
