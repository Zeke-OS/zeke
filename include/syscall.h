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

#if configDEVSUBSYS == 1
#include "dev.h"
#endif

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
#define KERNEL_SYSCALL_SCHED_DEV_WAIT           30
#define KERNEL_SYSCALL_SCHED_GET_LOADAVG        31

/** Argument struct for KERNEL_SYSCALL_SCHED_THREAD_CREATE */
typedef struct {
    osThreadDef_t * def;    /*!< Thread definitions for a new thread */
    void * argument;        /*!< Thread parameter(s) pointer */
} ds_osThreadCreate_t;

/** Argument struct for KERNEL_SYSCALL_SCHED_THREAD_SETPRIORITY */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
    osPriority priority;    /*!< Thread priority */
} ds_osSetPriority_t;

/** Argument struct for KERNEL_SYSCALL_SCHED_SIGNAL_SET
 *  and KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
    int32_t signal;         /*!< Thread signals to set */
} ds_osSignal_t;

/** Argument struct for KERNEL_SYSCALL_SCHED_SIGNAL_WAIT */
typedef struct {
    int32_t signals;        /*!< Thread signal(s) to wait */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osSignalWait_t;

#if configDEVSUBSYS == 1
typedef struct {
    osDev_t dev;            /*!< Device */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osDevWait_t;
#endif

#ifndef PU_TEST_BUILD
/**
  * Make system call
  */
uint32_t syscall(int type, void * p);
#endif /* PU_TEST_BUILD */

/* Kernel scope functions */
#ifdef KERNEL_INTERNAL
uint32_t _intSyscall_handler(int type, void * p);
#endif /* KERNEL_INTERNAL */

#endif /* SYSCALL_H */
