/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel's internal Syscall handler that is called from kernel scope.
 *
 *******************************************************************************
 */

#include "sched.h"
#include "ksignal.h"
#define KERNEL_INTERNAL 1
#include "dev.h"
#include "hal_core.h"
#include "syscall.h"

/**
 * Internal Syscall handler/translator
 *
 * This function is called from interrupt handler. This function calls the
 * actual kernel function and returns result pointer to the interrupt handler
 * which returns it to the original caller, which is usually a library function
 * in kernel.c
 * @param type syscall type.
 * @param p pointer to the parameter or parameter structure.
 * @return result value or pointer to the result.
 */
#pragma optimize=no_code_motion
uint32_t _intSyscall_handler(int type, void * p)
{
    uint32_t result;

    switch(type) {
    case KERNEL_SYSCALL_SCHED_THREAD_CREATE:
        result =(uint32_t)sched_ThreadCreate(
                    ((ds_osThreadCreate_t *)(p))->def,
                    ((ds_osThreadCreate_t *)(p))->argument
                );
        break;

    case KERNEL_SYSCALL_SCHED_THREAD_GETID:
         result =(uint32_t)sched_thread_getId();
        break;

    case KERNEL_SYSCALL_SCHED_THREAD_TERMINATE:
        result =(uint32_t)sched_thread_terminate(
                    *((osThreadId *)p)
                );
        break;

    case KERNEL_SYSCALL_SCHED_THREAD_SETPRIORITY:
        result =(uint32_t)sched_thread_setPriority(
                    ((ds_osSetPriority_t *)(p))->thread_id,
                    ((ds_osSetPriority_t *)(p))->priority
                );
        break;

    case KERNEL_SYSCALL_SCHED_THREAD_GETPRIORITY:
        result =(uint32_t)sched_thread_getPriority(
                    *((osPriority *)(p))
                );
        break;

    case KERNEL_SYSCALL_SCHED_DELAY:
        result =(uint32_t)sched_threadDelay(
                    *((uint32_t *)(p))
                );
        break;

    case KERNEL_SYSCALL_SCHED_WAIT:
        result =(uint32_t)sched_threadWait(
                    *((uint32_t *)(p))
                );
        break;

    case KERNEL_SYSCALL_SCHED_SIGNAL_SET:
        result =(uint32_t)ksignal_threadSignalSet(
                    ((ds_osSignal_t *)p)->thread_id,
                    ((ds_osSignal_t *)p)->signal
                );
        break;

    case KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR:
        result =(uint32_t)ksignal_threadSignalClear(
                    ((ds_osSignal_t *)p)->thread_id,
                    ((ds_osSignal_t *)p)->signal
                );
        break;

    case KERNEL_SYSCALL_SCHED_SIGNAL_GETCURR:
        result =(uint32_t)ksignal_threadSignalGetCurrent();
        break;

    case KERNEL_SYSCALL_SCHED_SIGNAL_GET:
        result =(uint32_t)ksignal_threadSignalGet(
                    *((osThreadId *)p)
                );
        break;

    case KERNEL_SYSCALL_SCHED_SIGNAL_WAIT:
        result =(uint32_t)ksignal_threadSignalWait(
                    ((ds_osSignalWait_t *)p)->signals,
                    ((ds_osSignalWait_t *)p)->millisec
                );
        break;

#if configDEVSUBSYS == 1
    case KERNEL_SYSCALL_SCHED_DEV_WAIT:
        result =(uint32_t)dev_threadDevWait(
                    ((ds_osDevWait_t *)p)->dev,
                    ((ds_osDevWait_t *)p)->millisec
                );
        break;
#endif

    case KERNEL_SYSCALL_MUTEX_TEST_AND_SET:
        result = (uint32_t)test_and_set((int *)p);
        break;

    case KERNEL_SYSCALL_SCHED_GET_LOADAVG:
        result = NULL;
        sched_get_loads((uint32_t *)p);
        break;

    default:
        result = NULL;
        break;
    }

    return result;
}
