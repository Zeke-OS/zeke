/**
 *******************************************************************************
 * @file time.c
 * @author Olli Vanhoja
 * @brief Time functions.
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

#include <sys/linker_set.h>
#include <kinit.h>
#include <errno.h>
#include <hal/core.h>
#include <libkern.h>
#include <kstring.h>
#include <klocks.h>
#include <kerror.h>
#include <syscall.h>
#include <sys/time.h>

#define SEC_MS 1000
#define SEC_US 1000000
#define SEC_NS 1000000000

/*
 * Current system time.
 */
static struct timespec realtime;
static mtx_t timelock;

static uint64_t utime_last;
static uint64_t sec_next;

static void _update_realtime(void);

int __kinit__ clock_init(void)
{
    SUBSYS_INIT("clock");

    mtx_init(&timelock, MTX_TYPE_SPIN, 0);

    return 0;
}

void update_realtime(void)
{
    mtx_lock(&timelock);
    _update_realtime();
    mtx_unlock(&timelock);
}

static void update_realtime_safe(void)
{
    if (mtx_trylock(&timelock))
        return;
    _update_realtime();
    mtx_unlock(&timelock);
}
/* Calculate new realtime at least before scheduling anything. */
DATA_SET(pre_sched_tasks, update_realtime_safe);

/**
 * Update realtime counters.
 */
static void _update_realtime(void)
{
    uint64_t utime = get_utime();
    uint64_t usecdiff;

    KASSERT(mtx_test(&timelock), "timelock should be locked");

    /* Update seconds */
    if (utime >= sec_next) {
        realtime.tv_sec++;
        sec_next = utime + SEC_US;
    }

    /* Update nsecs */
    if (utime_last > utime)
        usecdiff = (~0 - utime_last) + utime;
    else
        usecdiff = utime - utime_last;
    realtime.tv_nsec = (realtime.tv_nsec + usecdiff * 1000);
    realtime.tv_nsec = realtime.tv_nsec - (realtime.tv_nsec / SEC_NS) * SEC_NS;

    utime_last = utime;
}

void nanotime(struct timespec * ts)
{
    update_realtime();
    getnanotime(ts);
}

void getnanotime(struct timespec * tsp)
{
    mtx_lock(&timelock);
    memcpy(tsp, &realtime, sizeof(struct timespec));
    mtx_unlock(&timelock);
}

/* Syscall handlers ***********************************************************/

static int sys_gettime(void * user_args)
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
        /* TODO Currently same as CLOCK_REALTIME */
    case CLOCK_REALTIME:
        nanotime(&ts);
        err = copyout(&ts, args.tp, sizeof(struct timespec));
        if (err) {
            set_errno(EFAULT);
            return -1;
        }
        break;
    default:
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static const syscall_handler_t time_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_TIME_GETTIME, sys_gettime)
};
SYSCALL_HANDLERDEF(time_syscall, time_sysfnmap)
