/**
 *******************************************************************************
 * @file    errno.h
 * @author  Olli Vanhoja
 * @brief   System error numbers.
 *          IEEE Std 1003.1, 2013
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013, 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup errno
 * Number of last error.
 * @since IEEE Std 1003.1, 2013.
 * @{
 */

#ifndef ERRNO_H
#define ERRNO_H

/**
 * Argument list too long.
 * @since POSIX.1-2017
 */
#define E2BIG           1
/**
 * Permission denied.
 * @since POSIX.1-2017
 */
#define EACCES          2
/**
 * Address in use.
 * @since POSIX.1-2017
 */
#define EADDRINUSE      3
/**
 * Address not available.
 * @since POSIX.1-2017
 */
#define EADDRNOTAVAIL   4
/**
 * Address family not supported.
 * @since POSIX.1-2017
 */
#define EAFNOSUPPORT    5
/**
 * Resource unavailable, try again.
 * @since POSIX.1-2017
 */
#define EAGAIN          6
/**
 * Connection already in progress.
 * @since POSIX.1-2017
 */
#define EALREADY        7
/**
 * Bad file descriptor.
 * @since POSIX.1-2017
 */
#define EBADF           8
/**
 * Bad message.
 * @since POSIX.1-2017
 */
#define EBADMSG         9
/**
 * Device or resource busy.
 * @since POSIX.1-2017
 */
#define EBUSY           10
/**
 * Operation canceled.
 * @since POSIX.1-2017
 */
#define ECANCELED       11
/**
 * No child processes.
 * @since POSIX.1-2017
 */
#define ECHILD          12
/**
 * Connection aborted.
 * @since POSIX.1-2017
 */
#define ECONNABORTED    13
/**
 * Connection refused.
 * POSIX.1-2017
 */
#define ECONNREFUSED    14
/**
 * Connection reset.
 * @since POSIX.1-2017
 */
#define ECONNRESET      15
/**
 * Resource deadlock would occur.
 * @since POSIX.1-2017
 */
#define EDEADLK         16
/**
 * Destination address required.
 * @since POSIX.1-2017
 */
#define EDESTADDRREQ    17
/**
 * Mathematics argument out of domain of function.
 * @since POSIX.1-2017
 */
#define EDOM            18
/**
 * Disk quota exceeded.
 * This is defined as reserved in POSIX but it doesn't give a meaning for the
 * error. The reason is likely that while the error is common the standard
 * doesn't define disk quotas.
 * @since POSIX.1-2017
 */
#define EDQUOT          19
/**
 * File exists.
 * @since POSIX.1-2017
 */
#define EEXIST          20
/**
 * Bad address.
 * @since POSIX.1-2017
 */
#define EFAULT          21
/**
 * File too large.
 * @since POSIX.1-2017
 */
#define EFBIG           22
/**
 * Host is unreachable.
 * @since POSIX.1-2017
 */
#define EHOSTUNREACH    23
/**
 * Identifier removed.
 * @since POSIX.1-2017
 */
#define EIDRM           24
/**
 * Illegal byte sequence.
 * @since POSIX.1-2017
 */
#define EILSEQ          25
/**
 * Operation in progress.
 * @since POSIX.1-2017
 */
#define EINPROGRESS     26
/**
 * Interrupted function.
 * @since POSIX.1-2017
 */
#define EINTR           27
/**
 * Invalid argument.
 * @since POSIX.1-2017
 */
#define EINVAL          28
/**
 * I/O error.
 * @since POSIX.1-2017
 */
#define EIO             29
/**
 * Socket is connected.
 * @since POSIX.1-2017
 */
#define EISCONN         30
/**
 * Is a directory.
 * @since POSIX.1-2017
 */
#define EISDIR          31
/**
 * Too many levels of symbolic links.
 * @since POSIX.1-2017
 */
#define ELOOP           32
/**
 * File descriptor value too large.
 * @since POSIX.1-2017
 */
#define EMFILE          33
/**
 * Too many links.
 * @since POSIX.1-2017
 */
#define EMLINK          34
/**
 * Message too large.
 * @since POSIX.1-2017
 */
#define EMSGSIZE        35
/**
 * Multihop attempted.
 * Marked as reserved in POSIX.
 * @since POSIX.1-2017
 */
#define EMULTIHOP       36
/**
 * Filename too long.
 * @since POSIX.1-2017
 */
#define ENAMETOOLONG    37
/**
 * Network is down.
 * @since POSIX.1-2017
 */
#define ENETDOWN        38
/**
 * Connection aborted by network.
 * @since POSIX.1-2017
 */
#define ENETRESET       39
/**
 * Network unreachable.
 * @since POSIX.1-2017
 */
#define ENETUNREACH     40
/**
 * Too many files open in system.
 * @since POSIX.1-2017
 */
#define ENFILE          41
/**
 * No buffer space available.
 * @since POSIX.1-2017
 */
#define ENOBUFS         42
/**
 * No such device.
 * @since POSIX.1-2017
 */
#define ENODEV          44
/**
 * No such file or directory.
 * @since POSIX.1-2017
 */
#define ENOENT          45
/**
 * Executable file format error.
 * @since POSIX.1-2017
 */
#define ENOEXEC         46
/**
 * No locks available.
 * @since POSIX.1-2017
 */
#define ENOLCK          47
/**
 * Link has been severed.
 * Marked as reserved in POSIX.
 * @since POSIX.1-2017
 */
#define ENOLINK         48
/**
 * Not enough space.
 * @since POSIX.1-2017
 */
#define ENOMEM          49
/**
 * No message of the desired type.
 * @since POSIX.1-2017
 */
#define ENOMSG          50
/**
 * Protocol not available.
 * @since POSIX.1-2017
 */
#define ENOPROTOOPT     51
/**
 * No space left on device.
 * @since POSIX.1-2017
 */
#define ENOSPC          52
/**
 * Functionality not supported.
 * @since POSIX.1-2017
 */
#define ENOSYS          55
/**
 * The socket is not connected.
 * @since POSIX.1-2017
 */
#define ENOTCONN        56
/**
 * Not a directory or a symbolic link to a directory.
 * @since POSIX.1-2017
 */
#define ENOTDIR         57
/**
 * Directory not empty.
 * @since POSIX.1-2017
 */
#define ENOTEMPTY       58
/**
 * State not recoverable.
 * @since POSIX.1-2017
 */
#define ENOTRECOVERABLE 59
/**
 * Not a socket.
 * @since POSIX.1-2017
 */
#define ENOTSOCK        60
/**
 * Not supported.
 * @since POSIX.1-2017
 */
#define ENOTSUP         61
/**
 * Inappropriate I/O control operation.
 * @since POSIX.1-2017
 */
#define ENOTTY          62
/**
 * No such device or address.
 * @since POSIX.1-2017
 */
#define ENXIO           63
/**
 * Operation not supported on socket.
 * Same as ENOTSUP.
 * @since POSIX.1-2017
 */
#define EOPNOTSUPP      ENOTSUP
/**
 * Value too large to be stored in data type.
 * @since POSIX.1-2017
 */
#define EOVERFLOW       64
/**
 * Previous owner died.
 * @since POSIX.1-2017
 */
#define EOWNERDEAD      66
/**
 * Operation not permitted.
 * @since POSIX.1-2017
 */
#define EPERM           67
/**
 * Broken pipe.
 * There is no process reading from the other end of a pipe.
 * @since POSIX.1-2017
 */
#define EPIPE           68
/**
 * Protocol error.
 * @since POSIX.1-2017
 */
#define EPROTO          69
/**
 * Protocol not supported.
 * @since POSIX.1-2017
 */
#define EPROTONOSUPPORT 70
/**
 * Protocol wrong type for socket.
 * @since POSIX.1-2017
 */
#define EPROTOTYPE      71
/**
 * Result too large.
 * @since POSIX.1-2017
 */
#define ERANGE          72
/**
 * Read-only file system.
 * An attempt to modify a read-only file system.
 * @since POSIX.1-2017
 */
#define EROFS           73
/**
 * Invalid seek.
 * @since POSIX.1-2017
 */
#define ESPIPE          74
/**
 * No such process.
 * @since POSIX.1-2017
 */
#define ESRCH           75
/**
 * Stale file handle.
 * Internal error of a file system where a file handle or descriptor exist but
 * perhaps the data is gone.
 * Marked as reserved in POSIX.
 * @since POSIX.1-2017
 */
#define ESTALE          76
/**
 * Connection timed out.
 * @since POSIX.1-2017
 */
#define ETIMEDOUT       78
/**
 * Text file busy.
 * @since POSIX.1-2017
 */
#define ETXTBSY         79
/**
 * Operation would block.
 * @since POSIX.1-2017
 */
#define EWOULDBLOCK     EAGAIN
/**
 * Cross-device link or Improper link.
 * @since POSIX.1-2017
 */
#define EXDEV           80
/**
 * Block device required.
 * Typically returned when a regular file is attempted to mount as a file
 * system.
 */
#define ENOTBLK         81

#ifndef KERNEL_INTERNAL
/**
 * Get pointer to the thread local errno.
 */
int * __error(void);
#define errno (*__error())
#else /* KERNEL_INTERNAL */
#include <vm/vm.h>
#include <thread.h>

/*
 * A type for errno.
 * Even this is a type definition it doesn't mean that type of errno is subject
 * to change but this just makes some parts of the code easier to read.
 * According to ISO C standard errno should be a modifiable lvalue of
 * type int, and must not be explicitly declared; errno may be a macro.
 * POSIX then suggests that errno should be thread local, which is why we have
 * that __erno() defined.
 */
#include <sys/types/_errno_t.h>

/**
 * Set errno of the current thread.
 * @param new_value is the new value of errno for the current_thread.
 */
inline void set_errno(int new_value)
{
    copyout(&new_value, &current_thread->tls_uaddr->errno_val, sizeof(errno_t));
}

/**
 * Get errno of the current thread.
 * @return Returns the errno of the current thread.
 */
inline errno_t get_errno(void)
{
    errno_t value;

    copyin(&current_thread->tls_uaddr->errno_val, &value, sizeof(errno_t));

    return value;
}
#endif /* KERNEL_INTERNAL */

#endif /* ERRNO */

/**
 * @}
 */

/**
 * @}
 */
