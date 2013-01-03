/**
 *******************************************************************************
 * @file    kernel.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "hal_core.h"
#include "hal_mcu.h"
#include "syscall.h"
#include "kernel.h"

/* Kernel Control Functions **************************************************/

int32_t osKernelRunning(void)
{
      return 1;
}

/* Thread Management *********************************************************/

/** @todo doesn't pass argument now */
osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument)
{
    ds_osThreadCreate_t args = {thread_def, argument};
    osThreadId result;

    result = (osThreadId)syscall(KERNEL_SYSCALL_SCHED_THREAD_CREATE, &args);

    /* Request immediate context switch */
    req_context_switch();

    return result;
}


/* Generic Wait Functions ****************************************************/

osStatus osDelay(uint32_t millisec)
{
    osStatus result;

    result = (osStatus)syscall(KERNEL_SYSCALL_SCHED_DELAY, &millisec);

    if (result != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    return result;
}

osEvent osWait(uint32_t millisec)
{
    osEvent * result;

    result = (osEvent *)syscall(KERNEL_SYSCALL_SCHED_WAIT, &millisec);

    if (result->status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Retrun a copy of the current state of the event structure */
    return *result;
}


/* Signal Management *********************************************************/

int32_t osSignalSet(osThreadId thread_id, int32_t signal)
{
    ds_osSignal_t ds = { thread_id, signal };
    int32_t result;

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_SET, &ds);

    return result;
}

int32_t osSignalClear(osThreadId thread_id, int32_t signal)
{
    ds_osSignal_t ds = { thread_id, signal };
    int32_t result;

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR, &ds);

    return result;
}

int32_t osSignalGetCurrent(void)
{
    int32_t result;

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR, NULL);

    return result;
}

int32_t osSignalGet(osThreadId thread_id)
{
}

osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
}

/**
  * @}
  */
