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

#define KERNEL_SYSCALL_SCHED_THREAD_CREATE  1
#define KERNEL_SYSCALL_SCHED_DELAY          2

#include "sched.h"

/**
  * Make system call
  */
inline osStatus syscall(int type, void * p)
{
    asm volatile(
                 "MOV r1, %0\n"
                 "MOV r2, %1\n"
                 "SVC #0\n"
                 "DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 : : "r" (type), "r" (p));

    /* Get return value */
    osStatus scratch;
    asm volatile("MOV %0, r3\n"
                 : "=r" (scratch));

    return scratch;
}

#ifdef KERNEL_INTERNAL

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
      default:
        result = osErrorValue;
    }

    return result;
}

#endif

#endif /* SYSCALL_H */
