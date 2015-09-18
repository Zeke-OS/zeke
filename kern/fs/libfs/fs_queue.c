/**
 *******************************************************************************
 * @file    fs_queue.c
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

#include <errno.h>
#include <fs/fs_queue.h>
#include <libkern.h>
#include <machine/atomic.h>

/**
 * FSQ signaling end points.
 */
enum wait4end {
    FSQ_WAIT4READ,
    FSQ_WAIT4WRITE,
};

struct fs_queue * fs_queue_create(size_t nr_blocks, size_t block_size)
{
    struct buf * bp;
    struct fs_queue * fsq;

    bp = geteblk(sizeof(struct fs_queue)
                 + nr_blocks * (sizeof(struct fs_queue_packet) + block_size));
    if (!bp)
        return NULL;

    fsq = (struct fs_queue *)(bp->b_data);

    fsq->qcb = queue_create(fsq->packet, block_size, nr_blocks);
    mtx_init(&fsq->wr_lock, MTX_TYPE_TICKET, MTX_OPT_DEFAULT);
    mtx_init(&fsq->rd_lock, MTX_TYPE_TICKET, MTX_OPT_DEFAULT);
    fsq->bp = bp;

    return fsq;
}

void fs_queue_destroy(struct fs_queue * fsq)
{
    struct buf * bp;

    if (!fsq)
        return;

    bp = fsq->bp;
    KASSERT(bp != NULL, "bp should be valid");
    if (bp->vm_ops->rfree)
        bp->vm_ops->rfree(bp);
}

/**
 * Get a pointer to the end point signals pointer.
 * @param fsq is a pointer to the fs queue object.
 * @param ep selects the end point.
 * @return A pointer to a corresponding signals struct pointer.
 */
static struct signals ** fsq_get_sigs(struct fs_queue * fsq, enum wait4end ep)
{
    switch (ep) {
    case FSQ_WAIT4READ:
        return &fsq->waiting4read;
    case FSQ_WAIT4WRITE:
        return &fsq->waiting4write;
    default:
        panic("Invalid end point for waiting");
    }
}

/**
 * Create a sigset to be used for fsq signal waiting.
 * @return A sigset_t with necessary signals set.
 */
static sigset_t create_fsq_sigset(void)
{
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, _SIGKERN);

    return sigset;
}

/**
 * Initialize sigwait cond for the current thread.
 * @param fsq is a pointer to the fs queue object.
 * @param[out] oldset is the original signal set blocked.
 */
static void fsq_sigwait_init(struct fs_queue * fsq, enum wait4end ep,
                             sigset_t * oldset)
{
    sigset_t newset = create_fsq_sigset();
    struct signals * sigs = &current_thread->sigs;
    struct signals ** waitsigsp = fsq_get_sigs(fsq, ep);

    ksignal_sigsmask(sigs, SIG_BLOCK, &newset, oldset);
    while (atomic_cmpxchg_ptr((void **)waitsigsp, NULL, sigs));
}

/**
 * Wait for a signal telling us to continue the ongoing fsq operation.
 */
static inline void fsq_sigwait(void)
{
    sigset_t newset = create_fsq_sigset();
    siginfo_t retval;

    /*
     * FIXME We should wait for a signal here but for some reason it causes
     * everything to hang.
     */

    ksignal_sigwait(&retval, &newset);
}

/**
 * Clear sigwait cond of the current thread.
 * @param fsq is a pointer to the fs queue object.
 */
static void fsq_sigwait_clear(struct fs_queue * fsq, enum wait4end ep,
                              sigset_t * oldset)
{
    struct signals * sigs = &current_thread->sigs;
    struct signals ** waitsigsp = fsq_get_sigs(fsq, ep);

    ksignal_sigsmask(sigs, SIG_SETMASK, oldset, NULL);
    atomic_set_ptr((void **)waitsigsp, NULL);
}

/**
 * Send a signal to the other end if a tread is waiting there.
 * @param fsq is a pointer to the fs queue object.
 */
static void fsq_sigsend(struct fs_queue * fsq, enum wait4end ep)
{
    struct signals * waitsigs;
    struct ksignal_param param = {
        .si_code = SIGKERN_FSQ,
    };

    waitsigs = atomic_read_ptr((void **)fsq_get_sigs(fsq, ep));
    if (waitsigs)
        ksignal_sendsig(waitsigs, _SIGKERN, &param);
}

ssize_t fs_queue_write(struct fs_queue * fsq, uint8_t * buf, size_t count,
                       int flags)
{
    struct fs_queue_packet * p;
    size_t offset, bytes, left = count;
    ssize_t wr = 0;

    if (count == 0)
        return 0;

    mtx_lock(&fsq->wr_lock);
    if (flags & FS_QUEUE_FLAGS_PACKET) {
        p = NULL;
        offset = 0;
    } else {
        /*
         * Continue writing to the packet pointed by last_wr_packet if it exist.
         */
        p = fsq->last_wr_packet;
        offset = fsq->last_wr;
    }

    while (left > 0) {
        if (offset == 0) {
            if ((flags & FS_QUEUE_FLAGS_NONBLOCK) &&
                !queue_alloc_get(&fsq->qcb)) {
                goto out;
            } else { /* Blocking IO */
                sigset_t oldset;

                fsq_sigwait_init(fsq, FSQ_WAIT4READ, &oldset);
                while (!(p = queue_alloc_get(&fsq->qcb))) {

                    /*
                     * Queue is full.
                     * Wait for the reading end to free some space.
                     */
                    fsq_sigwait();
                }

                /* Reset sigmask and wait state. */
                fsq_sigwait_clear(fsq, FSQ_WAIT4READ, &oldset);
            }
        }

        bytes = min(left, fsq->qcb.b_size - offset);
        if (offset > 0)
            p->size += bytes;
        else
            p->size = bytes;
        memmove(p->data + offset, buf + wr, bytes);
        left -= bytes;
        wr += bytes;

        if (offset + bytes >= fsq->qcb.b_size) {
            bytes = 0;
            offset = 0;
        }

        queue_alloc_commit(&fsq->qcb);
        fsq_sigsend(fsq, FSQ_WAIT4WRITE);
    }

    if (bytes > 0 && !(flags & FS_QUEUE_FLAGS_PACKET)) {
        fsq->last_wr_packet = p;
        fsq->last_wr = offset + bytes;
    } else {
        fsq->last_wr_packet = NULL;
        fsq->last_wr = 0;
    }

out:
    mtx_unlock(&fsq->wr_lock);
    return wr;
}

ssize_t fs_queue_read(struct fs_queue * fsq, uint8_t * buf, size_t count,
                      int flags)
{
    ssize_t rd = 0;
    size_t offset, bytes, left = count;

    /*
     * Freeze last_wr_packet because we might be reading it next,
     * thus it's not ok to modify it anymore.
     */
    mtx_lock(&fsq->wr_lock);
    fsq->last_wr_packet = NULL;
    fsq->last_wr = 0;
    mtx_unlock(&fsq->wr_lock);

    mtx_lock(&fsq->rd_lock);

    offset = fsq->last_rd;

    if (flags & FS_QUEUE_FLAGS_PACKET && count == 0) {
        queue_skip(&fsq->qcb, 1);
        goto out;
    }

    while (left > 0) {
        struct fs_queue_packet * p;

        if ((flags & FS_QUEUE_FLAGS_NONBLOCK) &&
            !queue_peek(&fsq->qcb, (void **)(&p))) {
            goto out;
        } else { /* Blocking IO */
            sigset_t oldset;

            fsq_sigwait_init(fsq, FSQ_WAIT4WRITE, &oldset);
            while (!queue_peek(&fsq->qcb, (void **)(&p))) {
                /*
                 * Queue is empty.
                 * Wait for the writing end to write something.
                 */
                fsq_sigwait();
            }

            /* Reset sigmask and wait state. */
            fsq_sigwait_clear(fsq, FSQ_WAIT4WRITE, &oldset);
        }

        bytes = min(left, p->size - offset);
        memmove(buf + rd, p->data + offset, bytes);
        left -= bytes;
        rd += bytes;

        if (flags & FS_QUEUE_FLAGS_PACKET) {
            queue_skip(&fsq->qcb, 1);
            bytes = 0;
            break;
        } else if (offset + bytes >= p->size) {
            queue_skip(&fsq->qcb, 1);
            bytes = 0;
            offset = 0;
        }
    }

    fsq->last_rd = bytes;

out:
    fsq_sigsend(fsq, FSQ_WAIT4READ);
    mtx_unlock(&fsq->rd_lock);
    return rd;
}
