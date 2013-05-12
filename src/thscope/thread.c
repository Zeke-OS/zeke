/**
 *******************************************************************************
 * @file    thread.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code.
 *          Functions that are called in thread context/scope.
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "hal_core.h"
#include "syscall.h"
#include "kernel.h"

/** @addtogroup Thread_Management
  * @{
  */

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

/**
  * @}
  */

/**
  * @}
  */

