/**
 *******************************************************************************
 * @file    syscall.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for syscalls
 *
 *******************************************************************************
 */

#pragma once
#ifndef SYSCALL_H
#define SYSCALL_H

/* List of syscalls */
#define KERNEL_SYSCALL_SCHED_THREAD_CREATE  1
#define KERNEL_SYSCALL_SCHED_DELAY          2
#define KERNEL_SYSCALL_SCHED_WAIT           3
#define KERNEL_SYSCALL_SCHED_SETSIGNAL      4

#include "sched.h"

typedef struct {
    osThreadId thread_id;
    int32_t signal;
} ds_osSignalSet_t;


/**
  * Make system call
  */
#pragma optimize=no_code_motion
uint32_t syscall(int type, void * p);

#ifdef KERNEL_INTERNAL

#pragma optimize=no_code_motion
inline uint32_t _intSyscall_handler(int type, void * p)
{
    uint32_t result;

    switch(type) {
      case KERNEL_SYSCALL_SCHED_THREAD_CREATE:
        result = (uint32_t)sched_ThreadCreate((osThreadDef_t *)(p));
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

#endif

#endif /* SYSCALL_H */
