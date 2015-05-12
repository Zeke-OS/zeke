/**
 *******************************************************************************
 * @file    sys/time.h
 * @author  Olli Vanhoja
 * @brief   time types.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef SYS_TIME_H
#define SYS_TIME_H

#include <time.h>
#include <sys/_suseconds_t.h>

#ifndef TIMEVAL_DEFINED
#define TIMEVAL_DEFINED
/**
 * timeval.
 */
struct timeval {
    time_t tv_sec;          /*!< Seconds. */
    suseconds_t tv_usec;    /*!< Microseconds. */
};
#endif

#ifndef KERNEL_INTERNAL

/**
 * Set file access and modification times.
 */
int utimes(const char * path, const struct timespec times[2]);


#else

/**
 * Update realtime counters.
 */
void update_realtime(void);

/**
 * Get realtime precise as possible by first updating the time counter.
 */
void nanotime(struct timespec * ts);

/**
 * Get less precise realtime value but much faster.
 */
void getnanotime(struct timespec * tsp);


/* ctime */

void ctime(char * result, time_t * t);
void asctime(char * result, struct tm * timeptr);

/**
 * Get GMT time.
 * @param[out] tm       is modified.
 * @param[in]  clock    is a unix time.
 */
void gmtime(struct tm * tm, time_t * clock);

/**
 * @param[out] tm
 */
void offtime(struct tm * tm, time_t * clock, long offset);

#endif
#endif /* SYS_TIME_H */
