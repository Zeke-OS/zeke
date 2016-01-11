/**
 *******************************************************************************
 * @file    idle.h
 * @author  Olli Vanhoja
 * @brief   Kernel idle thread and idle coroutine management.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#ifndef IDLE_H
#define IDLE_H

#include <stdint.h>
#include <sys/linker_set.h>

/**
 * Function type for an idle task.
 */
typedef void idle_task_t(uintptr_t arg);

struct _idle_task_desc {
    idle_task_t * fn;
    const uintptr_t arg;
};

/**
 * Declare an idle task.
 */
#define IDLE_TASK(_fun_, _arg_)                 \
struct _idle_task_desc _idle_task_##_fun_ = {   \
    .fn = _fun_,                                \
    .arg = _arg_,                               \
};                                              \
DATA_SET(_idle_tasks, _idle_task_##_fun_)

#endif /* IDLE_H */
