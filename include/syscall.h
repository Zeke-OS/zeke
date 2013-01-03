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
#define KERNEL_SYSCALL_SCHED_THREAD_CREATE      1
#define KERNEL_SYSCALL_SCHED_THREAD_GETID       2
#define KERNEL_SYSCALL_SCHED_THREAD_TERMINATE   3
#define KERNEL_SYSCALL_SCHED_DELAY              4
#define KERNEL_SYSCALL_SCHED_WAIT               5
#define KERNEL_SYSCALL_SCHED_SIGNAL_SET         6
#define KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR       7
#define KERNEL_SYSCALL_SCHED_SIGNAL_GETCURR     8
#define KERNEL_SYSCALL_SCHED_SIGNAL_GET         9
#define KERNEL_SYSCALL_SCHED_SIGNAL_WAIT        10

#include "sched.h"

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


/**
  * Make system call
  */
uint32_t syscall(int type, void * p);

#ifdef KERNEL_INTERNAL

uint32_t _intSyscall_handler(int type, void * p);

#endif

#endif /* SYSCALL_H */
