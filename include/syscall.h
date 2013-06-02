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
#if configDEVSUBSYS != 0
#include "devtypes.h"
#endif

#define SYSCALL_MINORBITS   27 /*!< Number of minor bits */
#define SYSCALL_MINORMASK   ((1u << MINORBITS) - 1) /*!< Minor bits mask */

/**
 * Get syscall major number from uint32_t.
 */
#define SYSCALL_MAJOR(type) ((uint32_t)((type) >> DEV_MINORBITS))

/**
 * Get syscall minor number from uint32_t.
 */
#define SYSCALL_MINOR(type) ((uint32_t)((type) & DEV_MINORMASK))

/**
 * Convert major, minor pair into syscall type code (uint32_t).
 */
#define SYSCALL_MMTOTYPE(ma, mi) (((ma) << DEV_MINORBITS) | (mi))

/* Syscall groups */
#define SYSCALL_GROUP_SCHED         0x01
#define SYSCALL_GROUP_SCHED_THREAD  0x02
#define SYSCALL_GROUP_SCHED_SIGNAL  0x03
#define SYSCALL_GROUP_DEV           0x04
#define SYSCALL_GROUP_LOCKS         0x05

/* List of syscalls */
#define SYSCALL_SCHED_THREAD_CREATE         SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x00)
#define SYSCALL_SCHED_THREAD_GETID          SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x01)
#define SYSCALL_SCHED_THREAD_TERMINATE      SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x02)
/* osThreadYield is not a syscall */
#define SYSCALL_SCHED_THREAD_SETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x03)
#define SYSCALL_SCHED_THREAD_GETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x04)
#define SYSCALL_SCHED_DELAY                 SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x00)
#define SYSCALL_SCHED_WAIT                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x01)
#define SYSCALL_SCHED_GET_LOADAVG           SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x02)
#define SYSCALL_SCHED_EVENT_GET             SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x03)
#define SYSCALL_SCHED_SIGNAL_SET            SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_SIGNAL, 0x00)
#define SYSCALL_SCHED_SIGNAL_CLEAR          SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_SIGNAL, 0x01)
#define SYSCALL_SCHED_SIGNAL_GETCURR        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_SIGNAL, 0x02)
#define SYSCALL_SCHED_SIGNAL_GET            SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_SIGNAL, 0x03)
#define SYSCALL_SCHED_SIGNAL_WAIT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_SIGNAL, 0x04)
#define SYSCALL_DEV_OPEN                    SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x00)
#define SYSCALL_DEV_CLOSE                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x01)
#define SYSCALL_DEV_CHECK_RES               SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x02)
#define SYSCALL_DEV_CWRITE                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x03)
#define SYSCALL_DEV_CREAD                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x04)
#define SYSCALL_DEV_BWRITE                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x05)
#define SYSCALL_DEV_BREAD                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x06)
#define SYSCALL_DEV_BSEEK                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x07)
#define SYSCALL_DEV_WAIT                    SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x08)
#define SYSCALL_MUTEX_TEST_AND_SET          SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x00)

//#ifndef PU_TEST_BUILD
uint32_t syscall(uint32_t type, void * p);
//#endif /* PU_TEST_BUILD */

/* Kernel scope functions */
#ifdef KERNEL_INTERNAL
#include "syscalldef.h"

uint32_t _intSyscall_handler(uint32_t type, void * p);
#endif /* KERNEL_INTERNAL */

#endif /* SYSCALL_H */
