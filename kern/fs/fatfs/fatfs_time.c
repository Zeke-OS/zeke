/**
 *******************************************************************************
 * @file    fatfs_time.c
 * @author  Olli Vanhoja
 * @brief   File System wrapper for time.
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

#include <sys/time.h>
#include <timeconst.h>
#include "src/ff.h"

#define FATTIME_EPOCH       1980

#define FATTIME_YEAR_MASK   0xFE000000 /* Year origin from the 1980 (0..127) */
#define FATTIME_MON_MASK    0x01E00000 /* Month (1..12) */
#define FATTIME_DAY_MASK    0x001F0000 /* Day of the month(1..31) */
#define FATTIME_HOUR_MASK   0x0000F800 /* Hour (0..23) */
#define FATTIME_MINUTE_MASK 0x000007E0 /* Minute (0..59) */
#define FATTIME_SEC_MASK    0x0000001F /* Second / 2 (0..29) */

DWORD get_fattime(void)
{
    struct timespec ts;
    struct tm tm;
    uint32_t fattime;

    nanotime(&ts);
    gmtime(&tm, &ts.tv_sec);

    fattime  = (tm.tm_year - (FATTIME_EPOCH - EPOCH_YEAR)) & FATTIME_YEAR_MASK;
    fattime |= (tm.tm_mon + 1) & FATTIME_MON_MASK;
    fattime |= (tm.tm_mday + 1) & FATTIME_DAY_MASK;
    fattime |= (tm.tm_hour) & FATTIME_HOUR_MASK;
    fattime |= (tm.tm_min) & FATTIME_MINUTE_MASK;
    fattime |= (tm.tm_sec) & FATTIME_SEC_MASK;

    return fattime;
}
