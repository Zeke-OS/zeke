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

#include <errno.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <syscall.h>
#include <buf.h>
#include <fs/devfs.h>
#include <kerror.h>
#include <kinit.h>
#include <libkern.h>
#include <proc.h>
#include <thread.h>
#include <vm/vm.h>

static mtx_t sync_lock;
static pthread_t sync_thread_tid;

SYSCTL_DECL(_vm_shmem);
SYSCTL_NODE(_vm, OID_AUTO, shmem, CTLFLAG_RW, 0,
            "shmem");

/**
 * Periodic sync list.
 */
static LIST_HEAD(shmem_sync_list_head, buf) shmem_sync_list =
     LIST_HEAD_INITIALIZER(shmem_sync_list);

static void * shmem_sync_thread(void * arg);

int __kinit__ shmem_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("shmem");

    mtx_init(&sync_lock, MTX_TYPE_SPIN,
             MTX_OPT_SLEEP | MTX_OPT_PRICEIL);
    sync_lock.pri.p_lock = NICE_MIN;

    /*
     * Create a thread for periodic syncing of mmap buffers.
     */

    struct buf * bp_stack = geteblk(MMU_PGSIZE_COARSE);
    if (!bp_stack) {
        KERROR(KERROR_ERR, "Can't allocate a stack for shmem sync thread.");
        return -ENOMEM;
    }

    struct _sched_pthread_create_args tdef_shmem = {
        .param.sched_policy = SCHED_FIFO,
        .param.sched_priority = NICE_DEF,
        .stack_addr = (void *)bp_stack->b_data,
        .stack_size = bp_stack->b_bcount,
        .flags      = 0,
        .start      = shmem_sync_thread,
        .arg1       = 0,
    };

    sync_thread_tid = thread_create(&tdef_shmem, 1);
    if (sync_thread_tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a thread for shmem sync");
        return sync_thread_tid;
    }

    return 0;
}

/**
 * @param file is the file to be memory mapped.
 * @param bp_out is a pointer to a buf pointer than should be written if
 *               mmap is succesful.
 * @param Return 0 if succeed; Otherwise a negative errno value is returned.
 */
static int mmap_file(file_t * file, size_t blkno, size_t bsize,
                     int flags, struct buf ** bp_out)
{
    vnode_t * vnode = file->vnode;
    struct buf * bp = NULL;

    /*
     * We have to do a clone or something because we don't want to share
     * a global buffer.
     */
    bp = geteblk(bsize);
    if (!bp)
        return -ENOMEM;

    BUF_LOCK(bp);
    fs_fildes_set(&bp->b_file, vnode, file->oflags);
    bp->b_file.stream = file->stream;
    bp->b_blkno = blkno;

    if (!(flags & MAP_SHARED))
        bp->b_flags |= B_NOTSHARED;
    if (flags & MAP_PRIVATE)
        bp->b_flags |= B_NOSYNC;
    bp->b_mmu.control = MMU_CTRL_MEMTYPE_WB;
    BUF_UNLOCK(bp);

    bio_readin(bp);

    *bp_out = bp;
    return 0;
}

int shmem_mmap(struct proc_info * proc, uintptr_t vaddr, size_t bsize, int prot,
             int flags, int fildes, off_t off, struct buf ** out, char ** uaddr)
{
    struct buf * bp = NULL;
    int err;
    size_t blksize = 0;

    KASSERT(out, "out buffer pointer must be set");

    if ((prot & PROT_EXEC) && priv_check(&curproc->cred, PRIV_VM_PROT_EXEC)) {
        return -EPERM;
    }

    /*
     * TODO Support for:
     * - MAP_STACK
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
        bp->b_mmu.control = MMU_CTRL_MEMTYPE_WB;
        BUF_UNLOCK(bp);
    } else { /* Map a file */
        file_t * file;
        vnode_t * vnode;
        struct dev_info * devnfo; /* If it's a device */
        struct stat statbuf;
        size_t blkno;
        int err;

        file = fs_fildes_ref(proc->files, fildes, 1);
        if (!file)
            return -EBADF;
        vnode = file->vnode;

        /*
         * Calculate block number.
         */
        err = vnode->vnode_ops->stat(vnode, &statbuf);
        if (err)
            goto errout;

        blksize = statbuf.st_blksize;
        bsize = memalign_size(bsize, blksize);

        if (!S_ISREG(vnode->vn_mode))
            blkno = off / blksize;
        else
            blkno = off & ~(blksize - 1);

        devnfo = (struct dev_info *)vnode->vn_specinfo;
        if ((S_ISBLK(statbuf.st_mode) || S_ISCHR(statbuf.st_mode)) &&
                devnfo->mmap) { /* Device specific mmap function. */
            err = devnfo->mmap(devnfo, blkno, bsize, flags, &bp);
        } else { /* Use the generic mmap function. */
            err = mmap_file(file, blkno, bsize, flags, &bp);
        }
        fs_fildes_ref(proc->files, fildes, -1);
errout:
        if (err)
            return err;
    }

    bp->b_uflags = prot & (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);

    if (flags & MAP_FIXED && vaddr < configEXEC_BASE_LIMIT) {
        /* No low mem mappings */
        flags &= ~MAP_FIXED;
    }

    if (flags & MAP_FIXED) {
        bp->b_mmu.vaddr = vaddr & ~(MMU_PGSIZE_COARSE - 1);
        err = vm_insert_region(proc, bp, VM_INSOP_MAP_REG);
        if (err > 0) /* err >i= 0 => region nr returned; err < -errno code */
            err = 0;
    } else {
        /* Randomly map bp somewhere into the process address space. */
        err = 0;
        if (!vm_rndsect(proc, 0 /* Ignored */, 0 /* Ignored */, bp))
            err = -ENOMEM;
    }
    if (err && bp->vm_ops->rfree) {
        bp->vm_ops->rfree(bp);
        return err;
    } else if (err) {
        return err;
    }

    /* Set uaddr */
    if (blksize > 0) {
        /* Adjust returned uaddr by off */
        *uaddr = (char *)(bp->b_mmu.vaddr + off % blksize);
    } else {
        *uaddr = (char *)(bp->b_mmu.vaddr);
    }

    /*
     * Insert bp to the periodic sync list if necessary/allowed.
     */
    if (!(bp->b_flags & B_NOSYNC)) {
        mtx_lock(&sync_lock);
        LIST_INSERT_HEAD(&shmem_sync_list, bp, shmem_entry_);
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
        LIST_REMOVE(bp, shmem_entry_);
        mtx_unlock(&sync_lock);
        bio_writeout(bp);
    }

    if (bp->vm_ops->rfree)
        bp->vm_ops->rfree(bp);

    return 0;
}

int shmem_sync_enabled = 1;
unsigned shmem_sync_period = 500;
static int sysctl_shmem_sync_period(SYSCTL_HANDLER_ARGS)
{
    int new_period = shmem_sync_period;
    int error;

    error = sysctl_handle_int(oidp, &new_period, sizeof(new_period), req);
    if (!error && req->newptr) {
        if (new_period > 0) {
            shmem_sync_enabled = 1;
            shmem_sync_period = new_period;
        } else {
            shmem_sync_enabled = 0;
            shmem_sync_period = 0;
        }
    }

    return error;
}

SYSCTL_PROC(_vm_shmem, OID_AUTO, sync_period,
            CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_SECURE2,
            NULL, 0, sysctl_shmem_sync_period, "I", "Shmem sync period [ms]");

static void * shmem_sync_thread(void * arg)
{
    struct buf * bp;

    while (1) {
        thread_sleep(shmem_sync_period);
        if (!shmem_sync_enabled) {
            thread_sleep(1000);
            continue;
        }

        mtx_lock(&sync_lock);

        LIST_FOREACH(bp, &shmem_sync_list, shmem_entry_) {
            bio_writeout(bp);
        }

        mtx_unlock(&sync_lock);
    }

    return NULL;
}

static int sys_mmap(__user void * user_args)
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
    if (err) {
        panic("mmap failed");
    }

    return retval;
}

static int sys_munmap(__user void * user_args)
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
        goto fail;
    }
    vm_replace_region(curproc, NULL, regnr, 0);

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
