/**
 *******************************************************************************
 * @file    syscalldef.h
 * @author  Olli Vanhoja
 * @brief   Types and definitions for syscalls.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#pragma once
#ifndef SYSCALLDEF_H
#define SYSCALLDEF_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

/** Argument struct for SYSCALL_SCHED_THREAD_CREATE */
struct _ds_pthread_create {
    pthread_t * thread;     /*!< Returned thread id. */
    start_routine start;    /*!< Thread start routine. */
    pthread_attr_t * def;   /*!< Thread def attributes. */
    void * argument;        /*!< Thread parameter(s) pointer. */
    void (*del_thread)(void *); /*!< Thread exit function. */
};

/** Argument struct for SYSCALL_SCHED_THREAD_SETPRIORITY */
struct _ds_set_priority {
    pthread_t thread_id;    /*!< Thread id */
    osPriority priority;    /*!< Thread priority */
};

/** Argument struct for SYSCALL_SCHED_SIGNAL_SET
 *  and KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR */
struct ds_signal {
    pthread_t thread_id;   /*!< Thread id */
    int32_t signal;         /*!< Thread signals to set */
};

/** Argument struct for SYSCALL_SCHED_SIGNAL_WAIT */
struct _ds_signal_wait_t {
    int32_t signals;        /*!< Thread signal(s) to wait */
    uint32_t millisec;      /*!< Timeout in ms */
};

/** Argument struct for SYSCALL_SEMAPHORE_WAIT */
struct _ds_semaphore_wait {
    uint32_t * s;           /*!< Pointer to the semaphore */
    uint32_t millisec;      /*!< Timeout in ms */
};

struct _sysctl_args {
    int * name;
    unsigned int namelen;
    void * old;
    size_t * oldlenp;
    void * new;
    size_t newlen;
};

/** Arguments struct for SYSCALL_FS_WRITE */
struct _fs_readwrite_args {
    int fildes;
    void * buf;
    size_t nbytes;
};

/** Arguments struct for SYSCALL_FS_LSEEK */
struct _fs_lseek_args {
    int fd;
    off_t offset; /* input and return value */
    int whence;
};

/** Arguments struct for SYSCALL_FS_FCNTL */
struct _fs_fcntl_args {
    int fd;
    int cmd;
    union {
        int ival;
        struct flock fl;
    } third;
};

/** Arguments struct for SYSCALL_FS_MOUNT */
struct _fs_mount_args {
    const char * source;
    size_t source_len; /*!< in bytes */
    const char * target;
    size_t target_len; /*!< in bytes */
    const char fsname[8];
    uint32_t flags;
    const char * parm;
    size_t parm_len; /*!< in bytes */
};

/* Arguments for SYSCALL_FS_OPEN */
struct _fs_open_args {
    int fd; /* if AT_FDARG */
    const char * name;
    size_t name_len; /*!< in bytes */
    int oflags;
    int atflags; /* AT_FDCWD or AT_FDARG */
    mode_t mode;
};

/** Arguments for SYSCALL_FS_GETDENTS */
struct _ds_getdents_args {
    int fd;
    char * buf;
    size_t nbytes;
};

/** Arguments for SYSCALL_FS_STAT */
struct _fs_stat_args {
    int fd;
    const char * path;
    size_t path_len;
    struct stat * buf;
    unsigned flags;
};

/** Arguments for SYSCALL_FS_ACCESS */
struct _fs_access_args {
    int fd;
    const char * path;
    size_t path_len;
    int amode;
    int flag;
};

/** Arguments for SYSCALL_FS_CHMOD */
struct _fs_chmod_args {
    int fd;
    mode_t mode;
};

/** Arguments for SYSCALL_FS_CHOWN */
struct _fs_chown_args {
    int fd;
    uid_t owner;
    gid_t group;
};

/** Arguments for SYSCALL_FS_LINK */
struct _fs_link_args {
    const char * path1;
    size_t path1_len;
    const char * path2;
    size_t path2_len;
};

/** Arguments for SYSCALL_FS_UNLINK */
struct _fs_unlink_args {
    int fd;             /*!< File descriptor number. */
    const char * path;
    size_t path_len;
    int flag;
};

/** Arguments for SYSCALL_FS_MKDIR */
struct _fs_mkdir_args {
    int fd;
    const char * path;
    size_t path_len;
    mode_t mode;
    unsigned atflags;
};

/** Arguments for SYSCALL_FS_RMDIR */
struct _fs_rmdir_args {
    const char * path;
    size_t path_len;
};

struct _fs_umask_args {
    mode_t newumask;
    mode_t oldumask;
};

struct _ioctl_get_args {
    int fd;
    uint32_t request;
    void * arg;
    size_t arg_len;
};

/** Arguments struct for SYSCALL_PROC_GETBREAK */
struct _ds_getbreak {
    void * start;
    void * stop;
};

#endif /* SYSCALLDEF_H */
