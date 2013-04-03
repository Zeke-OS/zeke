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

osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument)
{
    ds_osThreadCreate_t args;
    osThreadId result;

    args.def = thread_def;
    args.argument = argument;
    result = (osThreadId)syscall(KERNEL_SYSCALL_SCHED_THREAD_CREATE, &args);

    /* Request immediate context switch */
    req_context_switch();

    return result;
}

osThreadId osThreadGetId(void)
{
    osThreadId result;

    result = (osThreadId)syscall(KERNEL_SYSCALL_SCHED_THREAD_GETID, NULL);

    return result;
}

osStatus osThreadTerminate(osThreadId thread_id)
{
    osStatus result;

    result = (osStatus)syscall(KERNEL_SYSCALL_SCHED_THREAD_TERMINATE, &thread_id);

    return result;
}

osStatus osThreadYield(void)
{
    /* Request immediate context switch */
    req_context_switch();

    return (osStatus)osOK;
}

osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority)
{
    ds_osSetPriority_t ds;
    osStatus result;

    ds.thread_id = thread_id;
    ds.priority = priority;
    result = (osStatus)syscall(KERNEL_SYSCALL_SCHED_THREAD_SETPRIORITY, &ds);

    return result;
}

osPriority osThreadGetPriority(osThreadId thread_id)
{
    osPriority result;

    result = (osPriority)syscall(KERNEL_SYSCALL_SCHED_THREAD_GETPRIORITY, &thread_id);

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

    /* Return a copy of the current state of the event structure */
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

    /* Retrun a copy of the current state of the event structure */
    return *result;
}

void osGetLoadAvg(uint32_t loads[3])
{
    syscall(KERNEL_SYSCALL_SCHED_GET_LOADAVG, loads);
}

/**
  * @}
  */
