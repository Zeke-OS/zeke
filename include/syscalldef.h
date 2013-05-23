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

#if configDEVSUBSYS == 1
/** Argument struct for some dev syscalls */
typedef struct {
    osDev_t dev;            /*!< Device */
    osThreadId thread_id;   /*!< Thread id */
} ds_osDevHndl_t;

/** Argument struct for dev write syscalls */
typedef struct {
    osDev_t dev;            /*!< Device */
    void * data;               /*!< Data pointer */
} ds_osDevData_t;

/** Argument struct for SYSCALL_DEV_WAIT */
typedef struct {
    osDev_t dev;            /*!< Device */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osDevWait_t;
#endif

#endif /* SYSCALLDEF_H */
