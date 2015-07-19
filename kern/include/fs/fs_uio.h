/**
 *******************************************************************************
 * @file    fs_uio.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system user io headers.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup fs
 * @{
 */

/**
 * @addtogroup uio
 * @{
 */

#ifndef FS_UIO_H
#define FS_UIO_H

#include <stddef.h>

struct buf;
struct proc_info;

/**
 * File system IO buffer descriptor.
 */
struct fs_uio {
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
int fs_uio_init_kbuf(struct fs_uio * uio, __kernel void * kbuf, size_t size);

/**
 * Initialize a user IO buffer with a user address.
 * @param uio is a pointer to a IO descriptor struct.
 * @param kbuf is a pointer to a user address.
 * @param size is the size of the buffer at ubuf.
 * @return Returns 0 if succeed; Otherwise a negative errno code.
 */
int fs_uio_init_ubuf(struct fs_uio * uio, __user void * ubuf, size_t size,
                     int rw);

/**
 * Initialize a user IO buffer from struct buf.
 * @param[in] bp is a buffer allocated from core.
 * @param[out] uio is the target uio struct.
 */
void fs_uio_buf2kuio(struct buf * bp, struct fs_uio * uio);

/**
 * Copy from fs to uio buffer.
 */
int fs_uio_copyout(const void * src, struct fs_uio * uio, size_t offset,
                   size_t size);

/**
 * Copy form uio buffer to fs.
 */
int fs_uio_copyin(struct fs_uio * uio, void * dst, size_t offset, size_t size);

/**
 * Get uio kernel address.
 */
int fs_uio_get_kaddr(struct fs_uio * uio, __kernel void ** addr);

#endif /* FS_UIO_H */

/**
 * @}
 */

/**
 * @}
 */
