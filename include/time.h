/**
 *******************************************************************************
 * @file    time.h
 * @author  Olli Vanhoja
 * @brief   time types.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup LIBC
 * @{
 */

#ifndef TIME_H
#define TIME_H

#include <sys/cdefs.h>
#include <sys/_null.h>
#include "sys/types.h" /* TODO Not OK, instead of this only needed types should be defined */

struct tm {
    int tm_sec;     /*!< Seconds [0,60]. */
    int tm_min;     /*!< Minutes [0,59]. */
    int tm_hour;    /*!< Hour [0,23]. */
    int tm_mday;    /*!< Day of month [1,31]. */
    int tm_mon;     /*!< Month of year [0,11]. */
    int tm_year;    /*!< Years since 1900. */
    int tm_wday;    /*!< Day of week [0,6] (Sunday =0). */
    int tm_yday;    /*!< Day of year [0,365]. */
    int tm_isdst;   /*!< Daylight Savings flag. */
};

struct timespec {
    time_t tv_sec;  /*!< Seconds. */
    long tv_nsec;   /*!< Nanoseconds. */
};

struct itimerspec {
    struct timespec it_interval;    /*!< Timer period.  */
    struct timespec it_value;       /*!< Timer expiration. */
};

#define CLOCK_REALTIME              0
#define CLOCK_UPTIME                5
#define CLOCK_PROCESS_CPUTIME_ID    14
#define CLOCK_THREAD_CPUTIME_ID     15

#define CLOCKS_PER_SEC              1000000

/**
 * Argument struct for SYSCALL_TIME_GETTIME
 */
struct _ds_gettime {
    clockid_t clk_id;
    struct timespec * tp;
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

int clock_gettime(clockid_t clk_id, struct timespec * tp);
time_t time(time_t * t);

__END_DECLS
#endif /* KERNEL_INTERNAL*/

/* TODO Add POSIX test macro:
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_02 */

/* TODO http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/time.h.html */

#endif /* TIME_H */

/**
 * @}
 */
