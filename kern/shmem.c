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
#include <errno.h>
#include <kerror.h>
#include <syscall.h>
#include <proc.h>
#include <buf.h>
#include <vm/vm.h>
#include <sys/mman.h>

int shm_mmap(struct proc_info * proc, uintptr_t vaddr, size_t bsize, int prot,
             int flags, int fildes, off_t off, struct buf ** out)
{
    struct buf * bp;
    int err;

    KASSERT(out, "out buffer pointer must be set");

    if (flags & MAP_ANON) {
        bp = geteblk(bsize);
    } else { /* Map a file */
        file_t * file;
        size_t blkno = 0; /* TODO */

        file = fs_fildes_ref(proc->files, fildes, 1);
        if (!file)
            return -EBADF;

        bp = getblk(file->vnode, blkno, bsize, 0);

        fs_fildes_ref(proc->files, fildes, -1);
    }
    if (!bp) {
        return -ENOMEM;
    }

     bp->b_uflags = prot & (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
     bp->b_mmu.control = MMU_CTRL_MEMTYPE_WB;

    if (flags & MAP_FIXED) {
        bp->b_mmu.vaddr = vaddr & ~(MMU_PGSIZE_COARSE - 1);
        err = vm_insert_region(proc, bp, VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
        if (err > 0)
            err = 0;
    } else {
        /* Randomly map bp somwhere into the process address space. */
        if (vm_rndsect(proc, 0 /* Ignored */, 0 /* Ignored */, bp))
            err = 0;
        else
            err = -ENOMEM;
    }
    if (err && bp->vm_ops->rfree) {
        bp->vm_ops->rfree(bp);
        return err;
    }

    *out = bp;
    return 0;
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

    err = shm_mmap(curproc, (uintptr_t)args.addr, args.bsize, args.prot,
                   args.flags, args.fildes, args.off, &bp);
    if (err) {
        retval = err;
        goto fail;
    }
    if (!bp) {
        retval = -ENOMEM;
        goto fail;
    }

    args.addr = (void *)(bp->b_mmu.vaddr);
    err = copyout(&args, user_args, sizeof(args));
    if (err) {
        retval = -EFAULT;
        goto fail;
    }

    retval = 0;
fail:
    if (retval != 0) {
        if (bp && bp->vm_ops->rfree) {
            bp->vm_ops->rfree(bp);
        }

        set_errno(-retval);
        retval = -1;
    }

    return retval;
}

static const syscall_handler_t shmem_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SHMEM_MMAP, sys_mmap),
};
SYSCALL_HANDLERDEF(shmem_syscall, shmem_sysfnmap)
