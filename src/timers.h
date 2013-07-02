/**
 *******************************************************************************
 * @file    timers.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for kernel timers (timers.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "kernel.h"

/* User Flag Bits */
#define TIMERS_FLAG_ENABLED     0x1 /* See for description: timer_alloc_data_t */
#define TIMERS_FLAG_PERIODIC    0x2 /* See for description: timer_alloc_data_t */
#define TIMERS_USER_FLAGS       (TIMERS_FLAG_ENABLED | TIMERS_FLAG_PERIODIC)

typedef uint32_t timers_flags_t;

extern uint32_t timers_value;

void timers_init(void);
void timers_run(void);

/**
 * Reserve a new timer
 * @return Reference to a timer or -1 if allocation failed.
 */
int timers_add(osThreadId thread_id, timers_flags_t flags, uint32_t millisec);

void timers_start(int tim);
void timers_release(int tim);
osThreadId timers_get_owner(int tim);

/**
 * Get owner of the timer
 * @param tim timer id
 * @return thread id or -1 if out of bounds or timer is currently in released
 *         state
 */
osThreadId timers_get_owner(int tim);

#endif /* TIMERS_H */
