/**
 *******************************************************************************
 * @file    kernel_init.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 *******************************************************************************
 */

#include "kernel.h"
#include "sched.h"
#include "main.h"

#ifndef configMCU_MODEL
    #error MCU model not selected.
#endif

#if configMCU_MODEL == MCU_MODEL_STM32F0
#include "stm32f0_interrupt.h"
#else
    #error No hardware support for the selected MCU model.
#endif

static char main_Stack[200];

int main(void)
{
    interrupt_init_module();
    sched_init();
    {
        osThreadDef_t main_thread = { (os_pthread)(&app_main), osPriorityNormal, main_Stack, sizeof(main_Stack)/sizeof(char), NULL };
        osThreadCreate(&main_thread, NULL);
    }
    sched_start();
    while(1);
}