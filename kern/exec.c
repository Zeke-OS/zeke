/**
 *******************************************************************************
 * @file    exec.c
 * @author  Olli Vanhoja
 * @brief   Execute a file.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <buf.h>
#include <thread.h>
#include <proc.h>
#include <unistd.h>
#include <errno.h>
#include <syscall.h>
#include <exec.h>

SET_DECLARE(exec_loader, struct exec_loadfn);

int exec_file(file_t * file, char name[PROC_NAME_LEN], struct buf * env_bp,
              int uargc, uintptr_t uargv, uintptr_t uenvp)
{
    struct exec_loadfn ** loader;
    uintptr_t vaddr;
    pthread_t tid;
    struct buf * stack_region;
    struct buf * code_region;
    int err, retval = 0;

    if (!file)
        return -ENOENT;

    SET_FOREACH(loader, exec_loader) {
        err = (*loader)->fn(curproc, file, &vaddr);
        if (err == 0 || err != -ENOEXEC)
            break;
    }
    if (err) {
        retval = err;
        goto fail;
    }

    /* Map new environment */
    err = vm_insert_region(curproc, env_bp, VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
    if (err < 0) {
        if (env_bp->vm_ops && env_bp->vm_ops->rfree)
            env_bp->vm_ops->rfree(env_bp);
        else {
            KERROR(KERROR_ERR, "Can't free env_bp\n");
        }
        retval = err;
        goto fail;
    }

    /* Change proc name */
    strlcpy(curproc->name, name, sizeof(curproc->name));

    /* Create a new thread for executing main() */
    stack_region = (*curproc->mm.regions)[MM_STACK_REGION];
    code_region = (*curproc->mm.regions)[MM_CODE_REGION];
    pthread_attr_t pattr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = (void *)(stack_region->b_mmu.vaddr),
        .stackSize  = MMU_SIZEOF_REGION(&stack_region->b_mmu)
    };
    struct _ds_pthread_create ds = {
        .thread     = 0, /* return value */
        .start      = (void *(*)(void *))(code_region->b_mmu.vaddr),
        .def        = &pattr,
        .arg1       = uargc,
        .arg2       = uargv,
        .arg3       = uenvp,
        .arg4       = 0, /* TODO */
        .del_thread = NULL /* Not needed for main(). */
    };

    tid = thread_create(&ds, 0);
    if (tid <= 0) {
        panic("Exec failed");
    }
    err = 0;

    goto out;
fail:
out:
    if (err == 0) {
        disable_interrupt();
        /*
         * Mark main thread for deletion, it's up to user space to kill any
         * children. If there however is any child threads those may or may
         * not cause a segmentation fault depending on when the scheduler
         * starts removing stuff. This decission was made because we want to
         * keep disable_interrupt() time as short as possible and POSIX seems
         * to be quite silent about this issue anyway.
         */
        current_thread->inh.first_child = NULL;
        current_thread->inh.parent = NULL;
        curproc->main_thread = sched_get_thread_info(tid);
        sched_thread_detach(current_thread->id);
        /* Don't return but die. */
        thread_die(0);
    }

    return retval;
}

/**
 * Copyin an array of strings and remap to a new section mapped in user space.
 * @note vaddr must be set to the final value.
 */
static int copyin_aa(struct buf * bp, char * uarr, size_t uarr_len,
                     size_t * doffset)
{
    char ** arg = (char **)(bp->b_data + *doffset);
    char * val = (char *)(bp->b_data + *doffset);
    size_t left = bp->b_bcount - *doffset;
    size_t offset = *doffset;
    int err;

    if (uarr_len == 0)
        return 0;

    if (left <= uarr_len)
        return -ENOMEM;

    err = copyin(uarr, arg, uarr_len * sizeof(char *));
    if (err)
        return err;

    arg[uarr_len] = NULL;
    offset = uarr_len * sizeof(char *) + sizeof(char *);
    left -= offset;

    for (size_t i = 0; i < uarr_len; i++) {
        size_t copied;

        if (!arg[i])
            continue;

        err = copyinstr(arg[i], val + offset, left, &copied);
        if (err)
            return err;

        /* new pointer from agg[i] to the string, valid in user space. */
        arg[i] = (char *)(bp->b_mmu.vaddr + *doffset + offset);

        offset += copied;
        left -= copied;
    }

    *doffset = offset;

    return 0;
}

static int sys_exec(void * user_args)
{
    struct _exec_args args;
    char name[PROC_NAME_LEN];
    file_t * file;
    struct buf * env_bp;
    size_t arg_offset = 0;
    uintptr_t envp;
    int err, retval;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (!args.argv || !args.env) {
        set_errno(EINVAL);
        return -1;
    }

    /* Increment refcount for the file pointed by fd */
    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }

    /*
     * Copy in & out arguments and environ.
     */
    env_bp = geteblk(MMU_PGSIZE_COARSE);
    if (!env_bp) {
        set_errno(ENOMEM);
        retval = -1;
        goto out;
    }

    /* Currently copyin_aa() requires vaddr to be set. */
    env_bp->b_mmu.vaddr = configUENV_BASE_ADDR;
    env_bp->b_uflags = VM_PROT_READ;

    /* Copyin argv */
    err = copyin_aa(env_bp, (char *)args.argv, args.nargv, &arg_offset);
    if (err) {
        set_errno(-err);
        retval = -1;
        goto out;
    }
    arg_offset = memalign(arg_offset);
    envp = env_bp->b_mmu.vaddr + arg_offset;

    /* Copyin env */
    err = copyin_aa(env_bp, (char *)args.env, args.nenv, &arg_offset);
    if (err) {
        set_errno(-err);
        retval = -1;
        goto out;
    }

    strlcpy(name, (char *)(env_bp->b_data) + (args.nargv + 1) * sizeof(char *),
            sizeof(name));

    /*
     * Execute.
     */
    err = exec_file(file, name, env_bp, args.nargv, env_bp->b_mmu.vaddr, envp);
    if (err) {
        set_errno(-err);
        retval = -1;
        goto out;
    }

    retval = 0;
out:
    /* Decrement refcount for the file pointed by fd */
    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

static const syscall_handler_t exec_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_EXEC_EXEC, sys_exec),
};
SYSCALL_HANDLERDEF(exec_syscall, exec_sysfnmap)
