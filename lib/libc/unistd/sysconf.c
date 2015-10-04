/**
 *******************************************************************************
 * @file    unistd_sysconf.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
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

#include <errno.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <syscall.h>
#include <unistd.h>

static long sysconf_getpagesize(void)
{
    int mib[] = { CTL_HW, HW_PAGESIZE };
    static long pagesize;
    size_t len = sizeof(pagesize);

    if (pagesize)
        return pagesize;

    if (sysctl(mib, num_elem(mib), &pagesize, &len, NULL, 0) == -1)
        return 4096; /* Safe fallback value */
    return pagesize;
}

long sysconf(int name)
{
    int sysctl_mib[2];
    int sysctl_len = num_elem(sysctl_mib);
    long value = -1;
    size_t len = sizeof(value);
    struct rlimit rl;

    switch (name) {
    case _SC_AIO_LISTIO_MAX:
        /* TODO _SC_AIO_LISTIO_MAX */
        break;
    case _SC_AIO_MAX:
        /* TODO _SC_AIO_MAX */
        break;
    case _SC_AIO_PRIO_DELTA_MAX:
        /* TODO _SC_AIO_PRIO_DELTA_MAX */
        break;
    case _SC_ARG_MAX:
        sysctl_mib[0] = CTL_KERN;
        sysctl_mib[1] = KERN_ARGMAX;
        if (sysctl(sysctl_mib, sysctl_len, &value, &len, NULL, 0) == -1)
            value = -1;
        break;
    case _SC_ATEXIT_MAX:
        /* TODO _SC_ATEXIT_MAX */
        break;
    case _SC_BC_BASE_MAX:
        /* TODO _SC_BC_BASE_MAX */
        break;
    case _SC_BC_DIM_MAX:
        /* TODO _SC_BC_DIM_MAX */
        break;
    case _SC_BC_SCALE_MAX:
        /* TODO _SC_BC_SCALE_MAX */
        break;
    case _SC_BC_STRING_MAX:
        /* TODO _SC_BC_STRING_MAX */
        break;
    case _SC_CHILD_MAX:
        break;
    case _SC_CLK_TCK:
        sysctl_len = sysctlnametomib("kern.hz", sysctl_mib,
                                     num_elem(sysctl_mib));
        if (sysctl(sysctl_mib, sysctl_len, &value, &len, NULL, 0)) {
            value = -1;
            errno = EINVAL;
        }
        break;
    case _SC_COLL_WEIGHTS_MAX:
        /* TODO _SC_COLL_WEIGHTS_MAX */
        break;
    case _SC_DELAYTIMER_MAX:
        /* TODO _SC_DELAYTIMER_MAX */
        break;
    case _SC_EXPR_NEST_MAX:
        /* TODO _SC_EXPR_NEST_MAX */
        break;
    case _SC_HOST_NAME_MAX:
        value = (long)HOST_NAME_MAX;
        break;
    case _SC_IOV_MAX:
        /* TODO _SC_IOV_MAX */
        break;
    case _SC_LINE_MAX:
        value = (long)LINE_MAX;
        break;
    case _SC_LOGIN_NAME_MAX:
        value = MAXLOGNAME;
        break;
    case _SC_NGROUPS_MAX:
        value = NGROUPS_MAX;
        break;
    case _SC_GETGR_R_SIZE_MAX:
        /* TODO _SC_GETGR_R_SIZE_MAX */
        break;
    case _SC_GETPW_R_SIZE_MAX:
        /* TODO _SC_GETPW_R_SIZE_MAX */
        break;
    case _SC_MQ_OPEN_MAX:
        /* TODO _SC_MQ_OPEN_MAX */
        break;
    case _SC_MQ_PRIO_MAX:
        /* TODO _SC_MQ_PRIO_MAX */
        break;
    case _SC_OPEN_MAX:
        if ((getrlimit(RLIMIT_NOFILE, &rl) != 0) ||
            (rl.rlim_cur == RLIM_INFINITY))
            break;
        if (rl.rlim_cur > LONG_MAX) {
            errno = EOVERFLOW;
            break;
        }
        value = (long)rl.rlim_cur;
        break;
    case _SC_ADVISORY_INFO:
        value = _POSIX_ADVISORY_INFO;
        break;
    case _SC_BARRIERS:
        /* TODO _SC_BARRIERS */
        break;
    case _SC_ASYNCHRONOUS_IO:
        /* TODO _SC_ASYNCHRONOUS_IO */
        break;
    case _SC_CLOCK_SELECTION:
        /* TODO _SC_CLOCK_SELECTION */
        break;
    case _SC_CPUTIME:
        /* TODO _SC_CPUTIME */
        break;
    case _SC_FSYNC:
        /* TODO _SC_FSYNC */
        break;
    case _SC_IPV6:
        /* TODO _SC_IPV6 */
        break;
    case _SC_JOB_CONTROL:
        value = -1; /* TODO support _SC_JOB_CONTROL? */
        break;
    case _SC_MAPPED_FILES:
        value = _POSIX_MAPPED_FILES;
        break;
    case _SC_MEMLOCK:
        /* TODO _SC_MEMLOCK */
        break;
    case _SC_MEMLOCK_RANGE:
        /* TODO _SC_MEMLOCK_RANGE */
        break;
    case _SC_MEMORY_PROTECTION:
        /* TODO _SC_MEMORY_PROTECTION */
        break;
    case _SC_MESSAGE_PASSING:
        /* TODO _SC_MESSAGE_PASSING */
        break;
    case _SC_MONOTONIC_CLOCK:
        /* TODO _SC_MONOTONIC_CLOCK */
        break;
    case _SC_PRIORITIZED_IO:
        /* TODO _SC_PRIORITIZED_IO */
        break;
    case _SC_PRIORITY_SCHEDULING:
        value = _POSIX_PRIORITY_SCHEDULING;
        break;
    case _SC_RAW_SOCKETS:
        /* TODO _SC_RAW_SOCKETS */
        break;
    case _SC_READER_WRITER_LOCKS:
        /* TODO _SC_READER_WRITER_LOCKS */
        break;
    case _SC_REALTIME_SIGNALS:
        /* TODO _SC_REALTIME_SIGNALS */
        break;
    case _SC_REGEXP:
        /* TODO _SC_REGEXP */
        break;
    case _SC_SAVED_IDS:
        value = 1;
        break;
    case _SC_SEMAPHORES:
        /* TODO _SC_SEMAPHORES */
        break;
    case _SC_SHARED_MEMORY_OBJECTS:
        /* TODO _SC_SHARED_MEMORY_OBJECTS */
        break;
    case _SC_SHELL:
        value = _POSIX_SHELL;
        break;
    case _SC_SPAWN:
        /* RFE Probably won't be supported */
        break;
    case _SC_SPIN_LOCKS:
        /* TODO _SC_SPIN_LOCKS */
        break;
    case _SC_SPORADIC_SERVER:
        /* TODO _SC_SPORADIC_SERVER */
        break;
    case _SC_SS_REPL_MAX:
        /* TODO _SC_SS_REPL_MAX */
        break;
    case _SC_SYNCHRONIZED_IO:
        /* TODO _SC_SYNCHRONIZED_IO */
        break;
    case _SC_THREAD_ATTR_STACKADDR:
        /* TODO _SC_THREAD_ATTR_STACKADDR */
        break;
    case _SC_THREAD_ATTR_STACKSIZE:
        /* TODO _SC_THREAD_ATTR_STACKSIZE */
        break;
    case _SC_THREAD_CPUTIME:
        /* TODO _SC_THREAD_CPUTIME */
        break;
    case _SC_THREAD_PRIO_INHERIT:
        /* TODO _SC_THREAD_PRIO_INHERIT */
        break;
    case _SC_THREAD_PRIO_PROTECT:
        /* TODO _SC_THREAD_PRIO_PROTECT */
        break;
    case _SC_THREAD_PRIORITY_SCHEDULING:
        /* TODO _SC_THREAD_PRIORITY_SCHEDULING */
        break;
    case _SC_THREAD_PROCESS_SHARED:
        /* TODO _SC_THREAD_PROCESS_SHARED */
        break;
    case _SC_THREAD_ROBUST_PRIO_INHERIT:
        /* TODO _SC_THREAD_ROBUST_PRIO_INHERIT */
        break;
    case _SC_THREAD_ROBUST_PRIO_PROTECT:
        /* TODO _SC_THREAD_ROBUST_PRIO_PROTECT */
        break;
    case _SC_THREAD_SAFE_FUNCTIONS:
        value = _POSIX_THREAD_SAFE_FUNCTIONS;
        break;
    case _SC_THREAD_SPORADIC_SERVER:
        value = _POSIX_SPORADIC_SERVER;
        break;
    case _SC_THREADS:
        value = _POSIX_THREADS;
        break;
    case _SC_TIMEOUTS:
        /* TODO _SC_TIMEOUTS */
        break;
    case _SC_TIMERS:
        /* TODO _SC_TIMERS */
        break;
    case _SC_TRACE:
        /* TODO _SC_TRACE */
        break;
    case _SC_TRACE_EVENT_FILTER:
        /* TODO _SC_TRACE_EVENT_FILTER */
        break;
    case _SC_TRACE_EVENT_NAME_MAX:
        /* TODO _SC_TRACE_EVENT_NAME_MAX */
        break;
    case _SC_TRACE_INHERIT:
        /* TODO _SC_TRACE_INHERIT */
        break;
    case _SC_TRACE_LOG:
        /* TODO _SC_TRACE_LOG */
        break;
    case _SC_TRACE_NAME_MAX:
        /* TODO _SC_TRACE_NAME_MAX */
        break;
    case _SC_TRACE_SYS_MAX:
        /* TODO _SC_TRACE_SYS_MAX */
        break;
    case _SC_TRACE_USER_EVENT_MAX:
        /* TODO _SC_TRACE_USER_EVENT_MAX */
        break;
    case _SC_TYPED_MEMORY_OBJECTS:
        /* TODO _SC_TYPED_MEMORY_OBJECTS */
        break;
    case _SC_VERSION:
        /* TODO _SC_VERSION */
        break;
    case _SC_V7_ILP32_OFF32:
        /* TODO _SC_V7_ILP32_OFF32 */
        break;
    case _SC_V7_ILP32_OFFBIG:
        /* TODO _SC_V7_ILP32_OFFBIG */
        break;
    case _SC_V7_LP64_OFF64:
        /* TODO _SC_V7_LP64_OFF64 */
        break;
    case _SC_V7_LPBIG_OFFBIG:
        /* TODO _SC_V7_LPBIG_OFFBIG */
        break;
    case _SC_PAGE_SIZE:
    case _SC_PAGESIZE:
        value = sysconf_getpagesize();
        break;
    default:
        errno = EINVAL;
    }

    return value;
}
