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

#define KERNEL_SYSCALL_SCHED_DELAY  0

#include "sched.h"

/**
  * Make system call
  */
inline osStatus syscall(int type, void * p)
{
    asm volatile("PUSH {r5-r7}\n"
                 "MOV r5, %0\n"
                 "MOV r6, %1\n"
                 "SVC #0\n"
                 "DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 : : "r" (type), "r" (p));

    /* Get return value */
    osStatus scratch;
    asm volatile("MOV %0, r7\n"
                 "POP {r4-r7}\n"
                 : "=r" (scratch));

    return scratch;
}

inline osStatus _intSyscall_handler(int type, void * p)
{
    osStatus result;

    switch(type) {
      case KERNEL_SYSCALL_SCHED_DELAY:
        result = sched_threadDelay(*((uint32_t*)(p)));
        break;
      case 1:
        break;
      default:
        result = osErrorValue;
    }

    return result;
}

#endif /* SYSCALL_H */
