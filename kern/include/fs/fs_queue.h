/**
 *******************************************************************************
 * @file    fs_queue.h
 * @author  Olli Vanhoja
 * @brief   Generic queue for fs implementations.
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

#ifndef _FS_QUEUE_H_
#define _FS_QUEUE_H_

#include <fcntl.h>
#include <buf.h>
#include <queue_r.h>
#include <ksignal.h>

struct fs_queue_packet {
    size_t size;
    char data[];
};

/**
 * fsq object descriptor.
 */
struct fs_queue {
    struct queue_cb qcb;
    struct buf * bp; /*!< self */
    struct fs_queue_packet * last_wr_packet; /* Last packet written. */
    size_t last_wr; /*!< Write index of the last packet in non-packet mode. */
    size_t last_rd; /*!< peek offset if the read count is less that block size. */
    mtx_t wr_lock;
    mtx_t rd_lock;
    struct signals * waiting4read;
    struct signals * waiting4write;
    struct fs_queue_packet packet[];
};

/*
 * FSQ Flags.
 */
#define FS_QUEUE_FLAGS_NONBLOCK 0x02
#define FS_QUEUE_FLAGS_PACKET   0x01

/**
 * Create a fs queue object.
 */
struct fs_queue * fs_queue_create(size_t nr_blocks, size_t block_size);

/**
 * Destroy a fs queue object.
 * @param fsq is a pointer to the fs queue object.
 */
void fs_queue_destroy(struct fs_queue * fsq);

static inline int oflags2fsq_flags(int oflags)
{
    return (oflags & O_NONBLOCK) ? FS_QUEUE_FLAGS_NONBLOCK : 0;
}

/**
 * Write to a fs queue.
 * @param fsq is a pointer to the fs queue object.
 */
ssize_t fs_queue_write(struct fs_queue * fsq, uint8_t * buf, size_t count,
                       int flags);
/**
 * Read from a fs queue.
 *
 * - count == 0 and FS_QUEUE_FLAGS_PACKET: Discard one packet
 * @param fsq is a pointer to the fs queue object.
 */
ssize_t fs_queue_read(struct fs_queue * fsq, uint8_t * buf, size_t count,
                      int flags);

#endif /* _FS_QUEUE_H_ */
