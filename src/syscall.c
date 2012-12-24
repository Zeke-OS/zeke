/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Syscall handler
 *
 *******************************************************************************
 */

#include "sched.h"
#define KERNEL_INTERNAL 1
#include "syscall.h"

#ifdef __ARM_PROFILE_M__
#include "cortex_m.h"
#else
    #error Selected ARM profile is not supported
#endif

#pragma optimize=no_code_motion
uint32_t _intSyscall_handler(int type, void * p)
{
    uint32_t result;

    switch(type) {
      case KERNEL_SYSCALL_SCHED_THREAD_CREATE:
        result = (uint32_t)sched_ThreadCreate(((ds_osThreadCreate_t *)(p))->def, ((ds_osThreadCreate_t *)(p))->argument);
        break;
      case KERNEL_SYSCALL_SCHED_DELAY:
        result = (uint32_t)sched_threadDelay(*((uint32_t *)(p)));
        break;
      case KERNEL_SYSCALL_SCHED_WAIT:
        result = (uint32_t)sched_threadWait(*((uint32_t *)(p)));
        break;
      case KERNEL_SYSCALL_SCHED_SETSIGNAL:
        result = (uint32_t)sched_threadSetSignal(((ds_osSignalSet_t *)p)->thread_id, ((ds_osSignalSet_t *)p)->signal);
        break;
      default:
        result = osErrorValue;
    }

    return result;
}
