/**
 *******************************************************************************
 * @file    bio.c
 * @author  Olli Vanhoja
 * @brief   IO Buffer Cache.
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

#define KERNEL_INTERNAL
#include <sys/linker_set.h>
#include <errno.h>
#include <buf.h>

int bread(vnode_t * vnode, off_t blkno, int size, struct buf ** bpp)
{
}

int  breadn(vnode_t * vnode, off_t blkno, int size, off_t rablks[],
            int rasizes[], int nrablks, struct buf ** bpp)
{
}

int bwrite(struct buf * bp)
{
}

void bawrite(struct buf * bp)
{
}

void bdwrite(struct buf * bp)
{
}

struct buf * getblk(vnode_t * vnode, off_t blkno, size_t size, int slpflag,
                    int slptimeo)
{
}

struct buf * incore(vnode_t * vnode, off_t blkno)
{
}

void biodone(struct buf * bp)
{
}

int biowait(struct buf * bp)
{
}

/**
 * Add a buffer region to the bio automanagement data structures.
 */
void _add2bioman(struct buf * bp)
{
}

static void bio_idle_task(void)
{
    /* TODO look for unbusied buffers that can be written out. */
}
DATA_SET(sched_idle_tasks, bio_idle_task);
