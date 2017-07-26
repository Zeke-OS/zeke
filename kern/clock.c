/**
 *******************************************************************************
 * @file time.c
 * @author Olli Vanhoja
 * @brief Time functions.
 * @section LICENSE
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/time.h>
#include <syscall.h>
#include <hal/hw_timers.h>
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <ksched.h>
#include <kstring.h>
#include <libkern.h>

#define SEC_MS 1000
#define SEC_US 1000000
#define SEC_NS 1000000000

/**
 * Current system time.
 */
static struct timespec uptime;
static struct timespec realtime_off;
static mtx_t timelock = MTX_INITIALIZER(MTX_TYPE_SPIN, 0);

/**
 * Update time counters.
 */
static void _update_time(void)
{
    static uint64_t utime_last;
    static uint64_t sec_next;
    uint64_t utime = get_utime();
    uint64_t usecdiff;

    KASSERT(mtx_test(&timelock), "timelock should be locked");

    /* Update seconds */
    if (utime >= sec_next) {
        uptime.tv_sec++;
        sec_next = utime + SEC_US;
    }

    /* Update nsecs */
    if (utime_last > utime)
        usecdiff = (~0 - utime_last) + utime;
    else
        usecdiff = utime - utime_last;
    uptime.tv_nsec = (uptime.tv_nsec + usecdiff * 1000);
    uptime.tv_nsec = uptime.tv_nsec - (uptime.tv_nsec / SEC_NS) * SEC_NS;

    utime_last = utime;
}

void update_time(void)
{
    mtx_lock(&timelock);
    _update_time();
    mtx_unlock(&timelock);
}

static void update_time_nonblocking(void)
{
    if (mtx_trylock(&timelock))
        return;
    _update_time();
    mtx_unlock(&timelock);
}

/* Calculate a new value for realtime at least before scheduling anything. */
SCHED_PRE_SCHED_TASK(update_time_nonblocking);

void nanotime(struct timespec * ts)
{
    update_time();
    getnanotime(ts);
}

void getnanotime(struct timespec * tsp)
{
    mtx_lock(&timelock);
    *tsp = uptime;
    mtx_unlock(&timelock);
}

void getrealtime(struct timespec * tsp)
{
    mtx_lock(&timelock);
    timespec_add(tsp, &uptime, &realtime_off);
    mtx_unlock(&timelock);
}

void setrealtime(struct timespec * tsp)
{
    mtx_lock(&timelock);
    timespec_sub(&realtime_off, tsp, &uptime);
    mtx_unlock(&timelock);
}

/* Syscall handlers ***********************************************************/

static intptr_t sys_gettime(__user void * user_args)
{
    struct _time_gettime_args args;
    struct timespec ts;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    switch (args.clk_id) {
    case CLOCK_UPTIME:
    case CLOCK_MONOTONIC:
        nanotime(&ts);
        break;
    case CLOCK_REALTIME:
        getrealtime(&ts);
        break;
    default:
        set_errno(EINVAL);
        return -1;
    }

    err = copyout(&ts, (__user struct timespec *)args.tp,
                  sizeof(struct timespec));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static intptr_t sys_settime(__user void * user_args)
{
    struct _time_settime_args args;
    struct timespec ts;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    switch (args.clk_id) {
    case CLOCK_REALTIME:
        setrealtime(&ts);
        break;
    default:
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static const syscall_handler_t time_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_TIME_GETTIME, sys_gettime),
    ARRDECL_SYSCALL_HNDL(SYSCALL_TIME_SETTIME, sys_settime),
};
SYSCALL_HANDLERDEF(time_syscall, time_sysfnmap)
