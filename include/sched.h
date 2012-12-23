/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header.
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
#define SCHED_NO_SIG_FLAG   0x00000004u /*!< Thread cannot be woken up by any signal. */

extern volatile uint32_t sched_enabled;
extern volatile uint32_t sched_cpu_load;

typedef struct {
    void * sp;              /*!< Stack pointer */
    uint32_t flags;         /*!< Status flags */
    osEvent event;          /*!< Event struct */
    osPriority priority;    /*!< Task priority */
    uint32_t uCounter;      /*!< Counter to calculate how much time this thread gets */
} threadInfo_t;

/* Public function prototypes ------------------------------------------------*/
void sched_init(void);
void sched_start(void);
void sched_handler(void * st);

/* Functions used by syscalls */
int sched_ThreadCreate(osThreadDef_t * thread_def);
osStatus sched_threadDelay(uint32_t millisec);
uint32_t sched_threadWait(uint32_t millisec);
uint32_t sched_threadSetSignal(osThreadId thread_id, int32_t signal);

#endif /* SCHED_HPP */

/**
  * @}
  */