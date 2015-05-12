/**
 *******************************************************************************
 * @file    types.h
 * @author  Olli Vanhoja
 * @brief   Types.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef size_t blkcnt_t; /*!< Used for file block counts. */
typedef int blksize_t; /*!< Used for block sizes. */
typedef uint32_t clock_t; /*!< Used for system times in clock ticks
                            or CLOCKS_PER_SEC. */
typedef int clockid_t; /*!< Used for clock ID type in the clock and
                         timer functions. */
typedef uint32_t dev_t; /*!< Device identifier */
typedef size_t fsblkcnt_t; /*!< Used for file system block counts. */
typedef size_t fsfilcnt_t; /*!< Used for file system file counts. */
#include <sys/_uid_t.h>
#include <sys/_gid_t.h>
typedef int id_t; /*!< Used as a general identifier; can be used to contain at
                    least a pid_t, uid_t, or gid_t. */
#define id_t id_t
typedef uint64_t ino_t; /*!< Used for file serial numbers.*/
typedef uint32_t key_t; /*!< Used for XSI interprocess communication. */
typedef uint32_t fflags_t;     /*!< file flags */
#ifndef _MODE_T_DECLARED
typedef int mode_t; /*!< Used for some file attributes. */
#define _MODE_T_DECLARED
#endif
typedef int nlink_t; /*!< Used for link counts. */
#include <sys/_off_t.h>
#ifdef _UOFF_T_DECLARED
#define _UOFF_T_DECLARED
typedef uint64_t uoff_t;
#endif
#include <sys/_pid_t.h>
#include <sys/_ssize_t.h>
typedef int64_t time_t; /*!< Used for time in seconds. */
typedef int64_t useconds_t; /*!< Used for time in microseconds. */
typedef int64_t suseconds_t; /*!< Used for time in microseconds */
typedef int timer_t; /*!< Used for timer ID returned by timer_create(). */


/* TODO Missing types:
 * - trace_attr_t
 * - trace_event_id_t
 * - trace_event_set_t
 * - trace_id_t
 * -
 */

#endif /* TYPES_H */
