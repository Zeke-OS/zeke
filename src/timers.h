/**
 *******************************************************************************
 * @file    timers.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for kernel timers (timers.c).
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
