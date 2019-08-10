/**
 *******************************************************************************
 * @file    fcntl.h
 * @author  Olli Vanhoja
 * @brief   File control options.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
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

/* TODO http://pubs.opengroup.org/onlinepubs/9699919799/ */

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef FCNTL_H
#define FCNTL_H

#include <stddef.h>
#include <stdint.h>

#ifndef _MODE_T_DECLARED
typedef int mode_t; /*!< Used for some file attributes. */
#define _MODE_T_DECLARED
#endif
#ifndef _OFF_T_DECLARED
typedef int64_t off_t; /*!< Used for file sizes. */
#define _OFF_T_DECLARED
#endif
#ifndef _PID_T_DECLARED
typedef int pid_t; /*!< Process ID. */
#define _PID_T_DECLARED
#endif

/**
 * @addtogroup fcntl
 * FCNTL
 * Manipulate file descriptor.
 * @{
 */

/* cmd args used by fcntl() */
#define F_DUPFD         0   /*!< Duplicate file descriptor. */
#define F_DUP2FD        1   /*!< Duplicate file descriptor to given number and
                             *   close any existing file. */
#define F_DUPFD_CLOEXEC 2   /*!< Duplicate file descriptor with the
                              close-on- exec flag FD_CLOEXEC set. */
#define F_GETFD         3   /*!< Get file descriptor flags. */
#define F_SETFD         4   /*!< Set file descriptor flags. */
#define F_GETFL         5   /*!< Get file status flags and file access modes. */
#define F_SETFL         6   /*!< Set file status flags. */
#define F_GETLK         7   /*!< Get record locking information. */
#define F_SETLK         8   /*!< Set record locking information. */
#define F_SETLKW        9   /*!< Set record locking information; wait if
                             *   blocked. */
#define F_GETOWN        10  /*!< Get process or process group ID to receive
                              SIGURG signals. */
#define F_SETOWN        11  /*!< Set process or process group ID to receive
                             *   SIGURG signals. */

/* fcntl() fdflags */
#define FD_CLOEXEC      0x1 /*!< Close the file descriptor upon execution of
                             *   an exec family function. */

/* fcntl() l_type arg */
#define F_RDLCK         0   /*!< Shared or read lock. */
#define F_UNLCK         1   /*!< Unlock. */
#define F_WRLCK         2   /*!< Exclusive or write lock. */

/**
 * @}
 */

#ifndef SEEK_SET
#define SEEK_SET        0 /*!< Beginning of file. */
#define SEEK_CUR        1 /*!< Current position */
#define SEEK_END        2 /*!< End of file. */
#endif

/* open() oflags */
#define O_CLOEXEC       0x0001 /*!< Close the file descriptor upon execution of
                                *   an exec family function. */
#define O_CREAT         0x0002 /*!< Create file if it does not exist. */
#define O_DIRECTORY     0x0004 /*!< Fail if not a directory. */
#define O_EXCL          0x0008 /*!< Exclusive use flag. */
#define O_NOCTTY        0x0010 /*!< Do not assign controlling terminal. */
#define O_NOFOLLOW      0x0020 /*!< Do not follow symbolic links. */
#define O_TRUNC         0x0040 /*!< Truncate flag. */
#define O_TTY_INIT      0x0080

/* open() file status flags */
#define O_APPEND        0x0100 /*!< Set append mode. */
#define O_NONBLOCK      0x0200 /*!< Non-blocking mode. */
#define O_SYNC          0x0400

#define O_ACCMODE       0x7000 /*!< Mask for file access modes. */
/* File access modes */
#define O_RDONLY        0x1000 /*!< Open for reading only. */
#define O_WRONLY        0x2000 /*!< Open for writing only. */
#define O_RDWR          0x3000 /*!< Open for reading and writing. */
#define O_SEARCH        0x8000 /*!< Open directory for search only. */
#define O_EXEC          0x4000 /*!< Open for execute only. */
#ifdef KERNEL_INTERNAL /* Internal flags to the kernel */
#define O_USERMASK      0x0ffff
#define O_KFREEABLE     0x10000 /*!< Can be freed with kfree() by
                                 *   fs_fildes_ref(), if not set the file won't
                                 *   be freed by fs subystem. This flag is
                                 *   useful if the file descriptor is static or
                                 *   freed by other subsystem.
                                 */
#define O_EXEC_ALTPCAP  0x20000  /*!< The executable file can set new process
                                  *   bounding capabilities on exec().
                                  */
#endif

#define AT_FDCWD            0x40000000 /*!< Use the current working directory to
                                        *   determine the target of relative
                                        *   file paths. */
#define AT_FDARG            0x01 /*!< Use the file descriptor given as an
                                  *   argument to determine the target of
                                  *   relative file paths. */
#define AT_EACCESS          0x10 /*!< Check access using effective user and
                                  *   group ID. */
#define AT_SYMLINK_NOFOLLOW 0x20 /*!< Do not follow symbolic links. */
#define AT_SYMLINK_FOLLOW   0x40 /*!< Follow symbolic link. */
#define AT_REMOVEDIR        0x80 /*!< Remove directory instead of file. */

/**
 * File lock.
 */
struct flock {
    short  l_type;      /*!< Type of lock; F_RDLCK, F_WRLCK, F_UNLCK. */
    short  l_whence;    /*!< Flag for starting offset. */
    off_t  l_start;     /*!< Relative offset in bytes. */
    off_t  l_len;       /*!< Size; if 0 then until EOF. */
    pid_t  l_pid;       /*!< Process ID of the process holding the lock;
                         *   returned with F_GETLK. */
};

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)

/**
 * Arguments struct for SYSCALL_FS_FCNTL
 */
struct _fs_fcntl_args {
    int fd;
    int cmd;
    union {
        int ival;
        struct flock fl;
    } third;
};

/*
 * Arguments for SYSCALL_FS_OPEN
 */
struct _fs_open_args {
    int fd; /* if AT_FDARG */
    const char * name;
    size_t name_len; /*!< in bytes */
    int oflags;
    int atflags; /* AT_FDCWD or AT_FDARG */
    mode_t mode;
};

#endif

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
int open(const char * path, int oflags, ...);
int openat(int fd, const char * path, int oflags, ...);
int creat(const char *, mode_t);

/**
 * @addtogroup fcntl
 * @{
 */

/**
 * Perform an operation on file descriptor.
 * @param fd    is the file descriptor number.
 * @param cmd   is one of the commands listed.
 * @param arg   is an optional third argument used by some of the commands.
 */
int fcntl(int fd, int cmd, ... /* arg */);

/**
 * @}
 */

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* FCNTL_H */

/**
 * @}
 */
