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

#include "kernel.h"

/* List of syscalls */
#define KERNEL_SYSCALL_SCHED_THREAD_CREATE      1
#define KERNEL_SYSCALL_SCHED_THREAD_GETID       2
#define KERNEL_SYSCALL_SCHED_THREAD_TERMINATE   3
/* osThreadYield is not a syscall */
#define KERNEL_SYSCALL_SCHED_THREAD_SETPRIORITY 4
#define KERNEL_SYSCALL_SCHED_THREAD_GETPRIORITY 5
#define KERNEL_SYSCALL_SCHED_DELAY              6
#define KERNEL_SYSCALL_SCHED_WAIT               7
#define KERNEL_SYSCALL_SCHED_SIGNAL_SET         8
#define KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR       9
#define KERNEL_SYSCALL_SCHED_SIGNAL_GETCURR     10
#define KERNEL_SYSCALL_SCHED_SIGNAL_GET         11
#define KERNEL_SYSCALL_SCHED_SIGNAL_WAIT        12

typedef struct {
    osThreadDef_t * def;
    void * argument;
} ds_osThreadCreate_t;

typedef struct {
    osThreadId thread_id;
    int32_t signal;
} ds_osSignal_t;

typedef struct {
    int32_t signals;
    uint32_t millisec;
} ds_osSignalWait_t;

typedef struct {
    osThreadId thread_id;
    osPriority priority;
} ds_osSetPriority_t;

#ifndef PU_TEST_BUILD
/**
  * Make system call
  */
uint32_t syscall(int type, void * p);
#endif /* PU_TEST_BUILD*/

#ifdef KERNEL_INTERNAL
uint32_t _intSyscall_handler(int type, void * p);
#endif /* KERNEL_INTERNAL */

#endif /* SYSCALL_H */
