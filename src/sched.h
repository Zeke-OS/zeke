/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header file for sched.c.
 *******************************************************************************
 */

/** @addtogroup Scheduler
  * @{
  */

#pragma once
#ifndef SCHED_HPP
#define SCHED_HPP

#include "kernel.h"

#define SCHED_IN_USE_FLAG   0x00000001u /*!< IN USE FLAG */
#define SCHED_EXEC_FLAG     0x00000002u /*!< EXEC = 1 / SLEEP = 0 */
#define SCHED_NO_SIG_FLAG   0x00000004u /*!< Thread cannot be woken up by a signal. */

extern volatile uint32_t sched_enabled;
extern volatile uint32_t sched_cpu_load;

typedef struct {
    void * sp;                  /*!< Stack pointer */
    uint32_t flags;             /*!< Status flags */
    osEvent event;              /*!< Event struct */
    osPriority def_priority;    /*!< Thread priority */
    osPriority priority;        /*!< Thread dynamic runtime priority */
    uint32_t uCounter;          /*!< Time slice counter */
    struct threadInheritance_t {
        void * parent;              /*!< Parent thread */
        void * first_child;         /*!< Link to the first child thread */
        /* Circularly linked list of childs of the parent thread */
        void * next_child;          /*!< Next child of the common parent */
    } inh;                      /*!< Thread inheritance; Parent and child thread pointers. */
} threadInfo_t;

/* Parent and child threads
 * ========================
 * + first_child is a parent thread attribute containing address to a first
 *   child of the parent thread
 * + parent is a child thread attribute containing address to a parent
 *   thread of the child thread
 * + prev_child is a child thread attribute containing address of
 *   a previous child node of a common parent thread ie. sharing same
 *   parent thread
 * + next_child is a child thread attribute containing address of a next
 *   child node of the common parent thread
 */

/* Public function prototypes ------------------------------------------------*/
void sched_init(void);
void sched_start(void);
void sched_handler(void * st);

/* Functions used by syscalls */
int sched_ThreadCreate(osThreadDef_t * thread_def, void * argument);
osStatus sched_threadDelay(uint32_t millisec);
uint32_t sched_threadWait(uint32_t millisec);
uint32_t sched_threadSetSignal(osThreadId thread_id, int32_t signal);

#endif /* SCHED_HPP */

/**
  * @}
  */
