/**
 *******************************************************************************
 * @file    resource.h
 * @author  Olli Vanhoja
 * @brief   Resource operations.
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

#ifndef _SYS_RESOURCES_H
#define _SYS_RESOURCES_H

#include <sys/types.h>

#define _RLIMIT_ARR_COUNT 7

#define RLIMIT_CORE     0 /*!< Resource id for maximum size of a core file,
                           * in bytes.
                           * A limit of 0 shall prevent the creation of a
                           * core file. If this limit is exceeded, the
                           * writing of a core file shall terminate at this
                           * size. */
#define RLIMIT_CPU      1 /*!< Resource id for max amount of CPU time limit.
                           * This is the maximum amount of CPU time, in
                           * seconds, used by a process. If this limit is
                           * exceeded, SIGXCPU shall be generated for the
                           * process. If the process is catching or
                           * ignoring SIGXCPU, or all threads belonging to
                           * that process are blocking SIGXCPU, the
                           * behavior is unspecified. */
#define RLIMIT_DATA     2 /*!< Resource id for data segment size limit.
                           * This is the maximum size of a process' data
                           * segment, in bytes. If this limit is exceeded,
                           * the malloc() function shall fail with errno set
                           * to [ENOMEM]. */
#define RLIMIT_FSIZE    3 /*!< Resource id for file size limit of a process.
                           * This is the maximum size of a file, in bytes,
                           * that may be created by a process. If a write or
                           * truncate operation would cause this limit to be
                           * exceeded, SIGXFSZ shall be generated for the
                           * thread. If the thread is blocking, or the
                           * process is catching or ignoring SIGXFSZ,
                           * continued attempts to increase the size of a
                           * file from end-of-file to beyond the limit
                           * shall fail with errno set to [EFBIG]. */
#define RLIMIT_NOFILE   4 /*!< Resource id for limit on number of open files.
                           * If this limit is exceeded, functions that
                           * allocate a file descriptor shall fail with errno
                           * set to [EMFILE]. */
#define RLIMIT_STACK    5 /*!< Resource id for maximum size of stack for a
                           * thread, in bytes.
                           * If this limit is exceeded, SIGSEGV shall be
                           * generated for the thread. If the thread is
                           * blocking SIGSEGV, or the process is ignoring
                           * or catching SIGSEGV and has not made
                           * arrangements to use an alternate stack, the
                           * disposition of SIGSEGV shall be set to SIG_DFL
                           * before it is generated. */
#define RLIMIT_AS       6 /*!< Resouce id for limit on address space size.
                           * This is the maximum size of a process' total
                           * available memory, in bytes. If this limit is
                           * exceeded, the malloc() and mmap() functions
                           * shall fail with errno set to [ENOMEM]. In
                           * addition, the automatic stack growth fails
                           * with the effects outlined above. */

/* Rlimit types */
#define RLIM_INFINITY   -1 /*!< A value of rlim_t indicating no limit. */
#define RLIM_SAVED_MAX  -2 /*!< A value of type rlim_t indicating an
                            * unrepresentable saved hard limit. */
#define RLIM_SAVED_CUR  -3 /*!< A value of type rlim_t indicating an
                            * unrepresentable saved soft limit. */

/* Priority identifiers */
#define PRIO_PROCESS    1 /*!< Identifies the who argument as a process ID. */
#if 0
#define PRIO_PGRP       2 /*!< Identifies the who argument as a process group ID. */
#define PRIO_USER       3 /*!< Identifies the who argument as a user ID. */
#endif
#define PRIO_THREAD     4 /*!< Identifies the who argument as a thread id. */

typedef int rlim_t;
#ifndef id_t
typedef int id_t;
#endif

struct rlimit {
    rlim_t  rlim_cur;   /*!< Current (soft) limit */
    rlim_t  rlim_max;   /*!< Maximum value for rlim_cur */
};

#define RUSAGE_SELF     1 /*!< Returns information about the current process. */
#define RUSAGE_CHILDREN 2 /*!< Returns information about children of the current
                           *   process. */

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


struct rusage {
    struct timeval ru_utime; /*!< User time used. */
    struct timeval ru_stime; /*!< System time used. */
};

#ifdef TYPES_PTHREAD_H

/**
 * Argument struct for SYSCALL_SCHED_THREAD_SETPRIORITY
 */
struct _ds_set_priority {
    pthread_t thread_id;    /*!< Thread id */
    int priority;           /*!< Thread priority */
};

#endif

/**
 * Get program scheduling priority.
 */
int getpriority(int which, id_t who);

/**
 * Set program scheduling priority.
 */
int setpriority(int which, id_t who, int prio);

#if 0
/**
 * Get resource limit.
 */
int getrlimit(int, struct rlimit *);
/**
 * Get resource usage.
 */
int getrusage(int, struct rusage *);
/**
 * Set resource limit.
 */
int setrlimit(int, const struct rlimit *);
#endif

/**
 * Get system load averages.
 * The getloadavg() function returns the number of processes in the system run
 * queue averaged over various periods of time. Up to nelem samples are
 * retrieved and assigned to successive elements of loadavg[].  The system
 * imposes a maximum of 3 samples, representing averages over the last 1, 5,
 * and 15 minutes, respectively.
 * @remarks Not in POSIX. Present on the BSD.
 * @todo    This function should be only visible if _BSD_SOURCE is defined and
 *          this function should be available via stdlib.h.
 * @param loadavg   is a double array.
 * @param nelem     is the number of samples to be obtained.
 * @return  If the load average was unobtainable, -1 is returned;
 *          Otherwise, the number of samples actually retrieved is returned.
 */
int getloadavg(double loadavg[3], int nelem);

#endif /* _SYS_RESOURCES_H */
