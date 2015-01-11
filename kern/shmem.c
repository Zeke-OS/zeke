/**
 *******************************************************************************
 * @file    shmem.c
 * @author  Olli Vanhoja
 * @brief   Process shared memory.
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

#define KERNEL_INTERNAL
#include <libkern.h>
#include <errno.h>
#include <kerror.h>
#include <thread.h>
#include <kinit.h>
#include <dllist.h>
#include <syscall.h>
#include <proc.h>
#include <buf.h>
#include <vm/vm.h>
#include <sys/mman.h>

static mtx_t sync_lock;

/**
 * Periodic sync list.
 */
static struct llist * shmem_sync_list;

static void * shmem_sync_thread(void * arg);

int shmem_init(void) __attribute__((constructor));
int shmem_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("shmem");

    mtx_init(&sync_lock, MTX_TYPE_SPIN | MTX_TYPE_SLEEP |
                         MTX_TYPE_PRICEIL);
    sync_lock.pri.p_lock = NICE_MAX;

    shmem_sync_list = dllist_create(struct buf, lentry_);

    /*
     * Create a thread for periodic syncing of mmap buffers.
     */

    struct buf * bp_stack = geteblk(MMU_PGSIZE_COARSE);
    if (!bp_stack)
        panic("Can't allocate a stack for shmem sync thread.");

    pthread_t tid;
    pthread_attr_t attr = {
        .tpriority  = NICE_DEF,
        .stackAddr  = (void *)bp_stack->b_data,
        .stackSize  = bp_stack->b_bcount,
    };
    struct _ds_pthread_create tdef_shmem = {
        .thread = &tid,
        .start  = shmem_sync_thread,
        .def    = &attr,
        .arg1   = 0
    };
    thread_create(&tdef_shmem, 1);
    if (tid < 0)
        panic("Failed to create a thread for shmem sync");

    return 0;
}

int shmem_mmap(struct proc_info * proc, uintptr_t vaddr, size_t bsize, int prot,
             int flags, int fildes, off_t off, struct buf ** out, char ** uaddr)
{
    struct buf * bp = NULL;
    int err;
    struct stat statbuf = { .st_blksize = 0 };

    KASSERT(out, "out buffer pointer must be set");

    if ((prot & PROT_EXEC) && priv_check(curproc, PRIV_VM_PROT_EXEC)) {
        return -EPERM;
    }

    /*
     * TODO Support for:
     * - MAP_SHARED, MAP_PRIVATE
     * - MAP_STACK
     * - MAP_NOSYNC
     * - MAP_EXCL
     * - MAP_NOCORE
     * - MAP_PREFAULT_READ
     * - MAP_32BIT (not needed anytime soon?)
     *
     *   Check prot access from fd?
     */

    if (flags & MAP_ANON) {
        bsize = memalign_size(bsize, MMU_PGSIZE_COARSE);
        bp = geteblk(bsize);
        if (!bp) {
            return -ENOMEM;
        }

        BUF_LOCK(bp);
        bp->b_flags |= B_NOSYNC;
        BUF_UNLOCK(bp);
    } else { /* Map a file */
        file_t * file;
        vnode_t * vnode;
        size_t blkno;

        file = fs_fildes_ref(proc->files, fildes, 1);
        if (!file)
            return -EBADF;
        vnode = file->vnode;

        if (!(vnode->vnode_ops->stat && vnode->vnode_ops->read))
            return -ENOTSUP; /* We'll need these operations */

        /*
         * Calculate block number.
         */
        vnode->vnode_ops->stat(vnode, &statbuf);
        if (!S_ISREG(vnode->vn_mode))
            blkno = off / statbuf.st_blksize;
        else
            blkno = off & ~(statbuf.st_blksize - 1);

        /* Fix bsize alignment. */
        bsize = memalign_size(bsize, statbuf.st_blksize);

        /*
         * We have to do a clone or something because we don't want to share
         * a global buffer.
         */
        bp = geteblk(bsize);
        if (!bp) {
            return -ENOMEM;
        }
        BUF_LOCK(bp);
        bp->b_file.fdflags = file->fdflags;
        bp->b_file.oflags = file->oflags;
        bp->b_file.refcount = 1;
        bp->b_file.vnode = vnode;
        bp->b_file.stream = file->stream;
        mtx_init(&file->lock, MTX_TYPE_SPIN);
        bp->b_blkno = blkno;

        if (!(flags & MAP_SHARED))
            bp->b_flags |= B_NOTSHARED;
        if (flags & MAP_PRIVATE)
            bp->b_flags |= B_NOSYNC;
        BUF_UNLOCK(bp);

        bio_readin(bp);

        fs_fildes_ref(proc->files, fildes, -1);
    }

    bp->b_uflags = prot & (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    bp->b_mmu.control = MMU_CTRL_MEMTYPE_WB;

    if (flags & MAP_FIXED) {
        bp->b_mmu.vaddr = vaddr & ~(MMU_PGSIZE_COARSE - 1);
        err = vm_insert_region(proc, bp, VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
        if (err > 0) /* err >i= 0 => region nr returned; err < -errorno code */
            err = 0;
    } else {
        /* Randomly map bp somewhere into the process address space. */
        if (vm_rndsect(proc, 0 /* Ignored */, 0 /* Ignored */, bp)) {
            err = 0;
        } else {
            err = -ENOMEM;
        }
    }
    if (err && bp->vm_ops->rfree) {
        bp->vm_ops->rfree(bp);
        return err;
    } else if (err) {
        return err;
    }

    /* Set uaddr */
    if (statbuf.st_blksize > 0) {
        /* Adjust returned uaddr by off */
        *uaddr = (char *)(bp->b_mmu.vaddr + off % statbuf.st_blksize);
    } else {
        *uaddr = (char *)(bp->b_mmu.vaddr);
    }

    /*
     * Insert bp to the periodic sync list.
     */
    if (!(bp->b_flags & B_NOSYNC)) {
        mtx_lock(&sync_lock);
        shmem_sync_list->insert_tail(shmem_sync_list, bp);
        mtx_unlock(&sync_lock);
    }

    *out = bp;
    return 0;
}

int shmem_munmap(struct buf * bp, size_t size)
{
    int flags;

    BUF_LOCK(bp);
    flags = bp->b_flags;
    BUF_UNLOCK(bp);

    /* TODO unmapping the region from the process should be moved to here */

    if (!(flags & B_NOSYNC)) {
        mtx_lock(&sync_lock);
        shmem_sync_list->remove(shmem_sync_list, bp);
        mtx_unlock(&sync_lock);
        bio_writeout(bp);
    }

    vrfree(bp);

    return 0;
}

int shmem_sync_enabled = 1;
static void * shmem_sync_thread(void * arg)
{
    struct buf * bp;

    while (shmem_sync_enabled) {
        thread_sleep(500);
        mtx_lock(&sync_lock);

        bp = shmem_sync_list->head;
        if (!bp)
            goto skip;

        do {
            bio_writeout(bp);
        } while ((bp = bp->lentry_.next));

skip:
        mtx_unlock(&sync_lock);
    }

    return NULL;
}

static int sys_mmap(void * user_args)
{
    struct _shmem_mmap_args args;
    struct buf * bp = NULL;
    int err, retval;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        retval = -EFAULT;
        goto fail;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        retval = -EFAULT;
        goto fail;
    }

    err = shmem_mmap(curproc, (uintptr_t)args.addr, args.bsize, args.prot,
                     args.flags, args.fildes, args.off, &bp,
                     (char **)(&args.addr));
    if (err) {
        args.addr = MAP_FAILED;
        retval = err;
        goto fail;
    }
    if (!bp) {
        retval = -ENOMEM;
        goto fail;
    }

    retval = 0;
fail:
    err = copyout(&args, user_args, sizeof(args));
    if (err)
        retval = -EFAULT;

    if (retval != 0) {
        if (bp && bp->vm_ops->rfree) {
            bp->vm_ops->rfree(bp);
        }

        set_errno(-retval);
        retval = -1;
    }

    return retval;
}

static int sys_munmap(void * user_args)
{
    struct _shmem_munmap_args args;
    struct buf * bp;
    int regnr;
    int err, retval;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        retval = -EFAULT;
        goto fail;
    }

    regnr = vm_find_reg(curproc, (uintptr_t)args.addr, &bp);
    if (!bp) {
        retval = -EINVAL;
    }
    vm_replace_region(curproc, NULL, regnr, VM_INSOP_NOFREE);

    /*
     * RFE
     * Currently we only unmap the region if size equals the original
     * allocation size. This may break some fancy userland programs trying
     * to do fancy things like expecting page allocation, allocating a big chunk
     * and the trying to unmap it partially to change some of the pages.
     */
    if (args.size == 0 || args.size == bp->b_bcount) {
        shmem_munmap(bp, args.size);
    } else {
        retval = -EINVAL;
        goto fail;
    }

    retval = 0;
fail:
    if (retval != 0) {
        set_errno(-retval);
        retval = -1;
    }
    return retval;
}

static const syscall_handler_t shmem_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SHMEM_MMAP, sys_mmap),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SHMEM_MUNMAP, sys_munmap),
};
SYSCALL_HANDLERDEF(shmem_syscall, shmem_sysfnmap)
