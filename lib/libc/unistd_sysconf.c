/**
 *******************************************************************************
 * @file    unistd_sysconf.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/sysctl.h>
#include <syscall.h>
#include <unistd.h>

#define num_elem(x) (sizeof(x) / sizeof(*(x)))

long sysconf(int name)
{
    int sysctl_mib[2];
    int sysctl_len;
    size_t len;
    long value = -1;

    switch (name) {
    case _SC_AIO_LISTIO_MAX:
        /* TODO */
        break;
    case _SC_AIO_MAX:
        /* TODO */
        break;
    case _SC_AIO_PRIO_DELTA_MAX:
        /* TODO */
        break;
    case _SC_ARG_MAX:
        /* TODO */
        break;
    case _SC_ATEXIT_MAX:
        /* TODO */
        break;
    case _SC_BC_BASE_MAX:
        /* TODO */
        break;
    case _SC_BC_DIM_MAX:
        /* TODO */
        break;
    case _SC_BC_SCALE_MAX:
        /* TODO */
        break;
    case _SC_BC_STRING_MAX:
        /* TODO */
        break;
    case _SC_CHILD_MAX:
        /* TODO */
        break;
    case _SC_CLK_TCK:
        sysctl_len = sysctlnametomib("kern.hz", sysctl_mib, num_elem(sysctl_mib));
        len = sizeof(value);
        if (sysctl(sysctl_mib, sysctl_len, &value, &len, 0, 0)) {
            value = -1;
            errno = EINVAL;
        }
        break;
    case _SC_COLL_WEIGHTS_MAX:
        /* TODO */
        break;
    case _SC_DELAYTIMER_MAX:
        /* TODO */
        break;
    case _SC_EXPR_NEST_MAX:
        /* TODO */
        break;
    case _SC_HOST_NAME_MAX:
        /* TODO */
        break;
    case _SC_IOV_MAX:
        /* TODO */
        break;
    case _SC_LINE_MAX:
        /* TODO */
        break;
    case _SC_LOGIN_NAME_MAX:
        /* TODO */
        break;
    case _SC_NGROUPS_MAX:
        /* TODO */
        break;
    case _SC_GETGR_R_SIZE_MAX:
        /* TODO */
        break;
    case _SC_GETPW_R_SIZE_MAX:
        /* TODO */
        break;
    case _SC_MQ_OPEN_MAX:
        /* TODO */
        break;
    case _SC_MQ_PRIO_MAX:
        /* TODO */
        break;
    case _SC_OPEN_MAX:
        /* TODO */
        break;
    case _SC_ADVISORY_INFO:
        /* TODO */
        break;
    case _SC_BARRIERS:
        /* TODO */
        break;
    case _SC_ASYNCHRONOUS_IO:
        /* TODO */
        break;
    case _SC_CLOCK_SELECTION:
        /* TODO */
        break;
    case _SC_CPUTIME:
        /* TODO */
        break;
    case _SC_FSYNC:
        /* TODO */
        break;
    case _SC_IPV6:
        /* TODO */
        break;
    case _SC_JOB_CONTROL:
        /* TODO */
        break;
    case _SC_MAPPED_FILES:
        /* TODO */
        break;
    case _SC_MEMLOCK:
        /* TODO */
        break;
    case _SC_MEMLOCK_RANGE:
        /* TODO */
        break;
    case _SC_MEMORY_PROTECTION:
        /* TODO */
        break;
    case _SC_MESSAGE_PASSING:
        /* TODO */
        break;
    case _SC_MONOTONIC_CLOCK:
        /* TODO */
        break;
    case _SC_PRIORITIZED_IO:
        /* TODO */
        break;
    case _SC_PRIORITY_SCHEDULING:
        /* TODO */
        break;
    case _SC_RAW_SOCKETS:
        /* TODO */
        break;
    case _SC_READER_WRITER_LOCKS:
        /* TODO */
        break;
    case _SC_REALTIME_SIGNALS:
        /* TODO */
        break;
    case _SC_REGEXP:
        /* TODO */
        break;
    case _SC_SAVED_IDS:
        /* TODO */
        break;
    case _SC_SEMAPHORES:
        /* TODO */
        break;
    case _SC_SHARED_MEMORY_OBJECTS:
        /* TODO */
        break;
    case _SC_SHELL:
        /* TODO */
        break;
    case _SC_SPAWN:
        /* TODO */
        break;
    case _SC_SPIN_LOCKS:
        /* TODO */
        break;
    case _SC_SPORADIC_SERVER:
        /* TODO */
        break;
    case _SC_SS_REPL_MAX:
        /* TODO */
        break;
    case _SC_SYNCHRONIZED_IO:
        /* TODO */
        break;
    case _SC_THREAD_ATTR_STACKADDR:
        /* TODO */
        break;
    case _SC_THREAD_ATTR_STACKSIZE:
        /* TODO */
        break;
    case _SC_THREAD_CPUTIME:
        /* TODO */
        break;
    case _SC_THREAD_PRIO_INHERIT:
        /* TODO */
        break;
    case _SC_THREAD_PRIO_PROTECT:
        /* TODO */
        break;
    case _SC_THREAD_PRIORITY_SCHEDULING:
        /* TODO */
        break;
    case _SC_THREAD_PROCESS_SHARED:
        /* TODO */
        break;
    case _SC_THREAD_ROBUST_PRIO_INHERIT:
        /* TODO */
        break;
    case _SC_THREAD_ROBUST_PRIO_PROTECT:
        /* TODO */
        break;
    case _SC_THREAD_SAFE_FUNCTIONS:
        /* TODO */
        break;
    case _SC_THREAD_SPORADIC_SERVER:
        /* TODO */
        break;
    case _SC_THREADS:
        /* TODO */
        break;
    case _SC_TIMEOUTS:
        /* TODO */
        break;
    case _SC_TIMERS:
        /* TODO */
        break;
    case _SC_TRACE:
        /* TODO */
        break;
    case _SC_TRACE_EVENT_FILTER:
        /* TODO */
        break;
    case _SC_TRACE_EVENT_NAME_MAX:
        /* TODO */
        break;
    case _SC_TRACE_INHERIT:
        /* TODO */
        break;
    case _SC_TRACE_LOG:
        /* TODO */
        break;
    case _SC_TRACE_NAME_MAX:
        /* TODO */
        break;
    case _SC_TRACE_SYS_MAX:
        /* TODO */
        break;
    case _SC_TRACE_USER_EVENT_MAX:
        /* TODO */
        break;
    case _SC_TYPED_MEMORY_OBJECTS:
        /* TODO */
        break;
    case _SC_VERSION:
        /* TODO */
        break;
    case _SC_V7_ILP32_OFF32:
        /* TODO */
        break;
    case _SC_V7_ILP32_OFFBIG:
        /* TODO */
        break;
    case _SC_V7_LP64_OFF64:
        /* TODO */
        break;
    case _SC_V7_LPBIG_OFFBIG:
        /* TODO */
        break;
    default:
        errno = EINVAL;
    }

    return value;
}
