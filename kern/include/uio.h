/**
 *******************************************************************************
 * @file    uio.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system user io headers.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup uio
 * @{
 */

#pragma once
#ifndef UIO_H
#define UIO_H

#include <stddef.h>

struct buf;
struct proc_info;

/**
 * User IO buffer descriptor.
 */
struct uio {
    __kernel void * kbuf;
    __user void * ubuf;
    struct proc_info * proc;
    size_t bufsize;
};

/**
 * Initialize a user IO buffer with a kernel address.
 * @param uio is a pointer to a IO descriptor struct.
 * @param kbuf is a pointer to a kernel address.
 * @param size is the size of the buffer at kbuf.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int uio_init_kbuf(struct uio * uio, __kernel void * kbuf, size_t size);

/**
 * Initialize a user IO buffer with a user address.
 * @param uio is a pointer to the UIO descriptor.
 * @param kbuf is a pointer to a user address.
 * @param size is the size of the buffer at ubuf.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int uio_init_ubuf(struct uio * uio, __user void * ubuf, size_t size,
                     int rw);

/**
 * INITIAlize a user IO buffer from struct buf.
 * @param[in] bp is a buffer allocated from core.
 * @param[out] uio is a pointer to the target UIO buffer.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int uio_buf2kuio(struct buf * bp, struct uio * uio);

/**
 * Copy from a kernel address to a UIO buffer.
 * @param src is the kernel source address.
 * @param uio is a pointer to the UIO descriptor.
 * @param offset is an offset for writing the UIO buffer.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int uio_copyout(const void * src, struct uio * uio, size_t offset,
                   size_t size);

/**
 * Copy from a UIO buffer to a kernel address.
 * @param uio is a pointer to the UIO descriptor.
 * @param offset is an ofset for reading the UIO buffer.
 * @param dst is the destination address in kernel space.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int uio_copyin(struct uio * uio, void * dst, size_t offset, size_t size);

/**
 * Get UIO kernel address.
 * @param uio is a pointer to the UIO descriptor.
 * @param[out] addr returns a kernel mapped address of the UIO buffer.
 * @return  Returns 0 if succeed;
 *          Otherwise a negative errno is returned and the value of addr is
 *          invalid.
 */
int uio_get_kaddr(struct uio * uio, __kernel void ** addr);

#endif /* UIO_H */

/**
 * @}
 */
