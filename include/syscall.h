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
    osThreadDef_t * def;
    void * argument;
} ds_osThreadCreate_t;

typedef struct {
    osThreadId thread_id;
    int32_t signal;
} ds_osSignalSet_t;


/**
  * Make system call
  */
uint32_t syscall(int type, void * p);

#ifdef KERNEL_INTERNAL

uint32_t _intSyscall_handler(int type, void * p);

#endif

#endif /* SYSCALL_H */
