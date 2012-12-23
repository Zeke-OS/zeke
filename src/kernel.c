/**
 *******************************************************************************
 * @file    kernel.h
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "stm32f0_interrupt.h"
#include "syscall.h"
#include "kernel.h"

/* Kernel Control Functions */

int32_t osKernelRunning(void)
{
      return 1;
}

/* Thread Management */

/** @todo doesn't pass argument now */
osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument)
{
    osThreadId result;

    result = (osThreadId)syscall(KERNEL_SYSCALL_SCHED_THREAD_CREATE, (void *)thread_def);

    return result;
}


/* Generic Wait Functions */

osStatus osDelay(uint32_t millisec)
{
    osStatus result;

    result = (osStatus)syscall(KERNEL_SYSCALL_SCHED_DELAY, &millisec);

    /* Request context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Set PendSV pending status */
    asm volatile("DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 );

    return result;
}

osEvent osWait(uint32_t millisec)
{
    osEvent * result;

    result = (osEvent *)syscall(KERNEL_SYSCALL_SCHED_WAIT, &millisec);

    /* Request context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Set PendSV pending status */
    asm volatile("DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 );

    /* Retrun a copy of the current state of the event structure */
    return *result;
}


/*  ==== Signal Management ==== */

int32_t osSignalSet(osThreadId thread_id, int32_t signal)
{
    ds_osSignalSet_t ds = { thread_id, signal };
    int32_t result;

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SETSIGNAL, &ds);

    return result;
}

/**
  * @}
  */