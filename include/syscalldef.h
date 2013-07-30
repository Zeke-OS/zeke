/**
 *******************************************************************************
 * @file    syscalldef.h
 * @author  Olli Vanhoja
 *
 * @brief   Types and definitions for syscalls
 *
 *******************************************************************************
 */

#pragma once
#ifndef SYSCALLDEF_H
#define SYSCALLDEF_H

#include <kernel.h>

/** Argument struct for SYSCALL_SCHED_THREAD_CREATE */
typedef struct {
    osThreadDef_t * def;    /*!< Thread definitions for a new thread */
    void * argument;        /*!< Thread parameter(s) pointer */
} ds_osThreadCreate_t;

/** Argument struct for SYSCALL_SCHED_THREAD_SETPRIORITY */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
    osPriority priority;    /*!< Thread priority */
} ds_osSetPriority_t;

/** Argument struct for SYSCALL_SCHED_SIGNAL_SET
 *  and KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
    int32_t signal;         /*!< Thread signals to set */
} ds_osSignal_t;

/** Argument struct for SYSCALL_SCHED_SIGNAL_WAIT */
typedef struct {
    int32_t signals;        /*!< Thread signal(s) to wait */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osSignalWait_t;

/** Argument struct for SYSCALL_SEMAPHORE_WAIT */
typedef struct {
    uint32_t * s;           /*!< Pointer to the semaphore */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osSemaphoreWait_t;

#if configDEVSUBSYS != 0
/** Argument struct for some dev syscalls */
typedef struct {
    osDev_t dev;            /*!< Device */
    osThreadId thread_id;   /*!< Thread id */
} ds_osDevHndl_t;

/** Argument struct for dev write syscalls */
typedef struct {
    osDev_t dev;            /*!< Device to be read from or written to. */
    void * data;            /*!< Data pointer. */
} ds_osDevCData_t;

/** Generic argument struct for block functions in dev subsystem */
typedef struct {
    void * buff;            /*!< pointer to a block of memory with a size of at
                             *   least (size*count) bytes, converted to a
                             *   void *. */
    size_t size;            /*!< in bytes, of each element. */
    size_t count;           /*!< number of elements, each one with a size of
                             *   size bytes. */
    osDev_t dev;            /*!< device to be read from or written to. */
} ds_osDevBData_t;

/** Argument struct for block seek function in dev subsystem */
typedef struct {
    int offset;             /*!< Number of size units to offset from origin. */
    int origin;             /*!< Position used as reference for the offset. */
    size_t size;            /*!< in bytes, of each element. */
    osDev_t dev;            /*!< device to be seeked from. */
} ds_osDevBSeekData_t;

/** Argument struct for SYSCALL_DEV_WAIT */
typedef struct {
    osDev_t dev;            /*!< Device */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osDevWait_t;
#endif

#endif /* SYSCALLDEF_H */
