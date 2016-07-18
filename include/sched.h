/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Scheduling header file.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/**
 * @addtogroup sched
 * @{
 */

#ifndef SCHED_H
#define SCHED_H

#include <sys/types/_pid_t.h>

/**
 * Scheduling parameters.
 */
struct sched_param {
    int sched_policy;   /*!< Thread  scheduling policy. */
    int sched_priority; /*!< Thread priority inside its policy class. */
};

/* Scheduling policies */
#define SCHED_FIFO  0
#define SCHED_RR    1
#define SCHED_OTHER 1

#define NICE_MAX    20
/* Default: NZERO */
#define NICE_MIN    (-20)
#define NICE_ERR    (-100) /*!< Thread doesn't exist or error */

#ifdef KERNEL_INTERNAL
/**
 * Limit the range of a nice value.
 */
#define NICE_RANGE(_prio_) (min(max(_prio_, NICE_MAX), NICE_MIN))
#endif

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
#include <sys/types/_id_t.h>
#include <sys/types/_pthread_t.h>

/**
 * Argument struct for SYSCALL_SCHED_SETPOLICY
 */
struct _setpolicy_args {
    id_t id;
    int policy;
};
#endif

#ifndef KERNEL_INTERNAL

/**
 * Set scheduling policy and parameters.
 * @note sched_policy of param is ignored since policy is already given as
 *       an argument.
 */
int sched_setscheduler(pid_t pid, int policy, const struct sched_param * param);
int sched_setparam(pid_t pid, const struct sched_param * param);

#endif /* !KERNEL_INTERNAL */

#endif /* SCHED_H */

/**
 * @}
 */
