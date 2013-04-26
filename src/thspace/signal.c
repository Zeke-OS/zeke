/**
 *******************************************************************************
 * @file    signal.c
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

/** @addtogroup Signal
  * @{
  */

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

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_GETCURR, NULL);

    return result;
}

int32_t osSignalGet(osThreadId thread_id)
{
    int32_t result;

    result = (int32_t)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_GET, &thread_id);

    return result;
}

osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
    ds_osSignalWait_t ds = { signals, millisec };
    osEvent * result;

    result = (osEvent *)syscall(KERNEL_SYSCALL_SCHED_SIGNAL_WAIT, &ds);

    if (result->status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    return *result;
}

/**
  * @}
  */

/**
  * @}
  */
