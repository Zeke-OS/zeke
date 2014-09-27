/**
 *******************************************************************************
 * @file    mount.h
 * @author  Olli Vanhoja
 * @brief   Mount or dismount a file system.
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

/** @addtogroup LIBC
 * @{
 */

#pragma once
#ifndef MOUNT_H
#define MOUNT_H

#include <sys/cdefs.h>

#define MNT_RDONLY      0x0001 /*!< Read only. */
#define MNT_SYNCHRONOUS 0x0002 /*!< Synchronous writes. */
#define MNT_ASYNC       0x0040 /*!< Asynchronous writes. */
#define MNT_NOEXEC      0x0004 /*!< No exec for the file system. */
#define MNT_NOSUID      0x0008 /*!< Set uid bits not honored. */
#define MNT_NOATIME     0x0100 /*!< Don't update file access times. */

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * The mount() system call attaches the file system specified by source to the
 * directory specified by target.
 * @throws + EFAULT     One of the argument pointers is outside of the process's
 *                      allocated address space.
 *         + ENOMEM     Not enough memory available to mount a new file system.
 *         + ENOENT     Mount target doesn't exist.
 *         + ENOTSUP    File system type is not supported.
 *         + ENODEV     Mount failed. TODO ??
 */
int mount(const char * source, const char * target, const char * type,
          int flags, char * parms);

int unmount(const char * dir, int flags);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* MOUNT_H */

/**
 * @}
 */
