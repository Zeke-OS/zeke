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

#include <unistd.h>
#include <errno.h>
#include <buf.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <syscall.h>
#include <sys/sysctl.h>
#include <thread.h>
#include <exec.h>

SET_DECLARE(exec_loader, struct exec_loadfn);

SYSCTL_INT(_kern, KERN_ARGMAX, argmax, CTLFLAG_RD, 0, MMU_PGSIZE_COARSE,
           "Max args to exec");

static int load_proc_image(file_t * file, uintptr_t * vaddr_base)
{
    struct exec_loadfn ** loader;
    int err = 0;

    SET_FOREACH(loader, exec_loader) {
        err = (*loader)->fn(curproc, file, vaddr_base);
        if (err == 0 || err != -ENOEXEC)
            break;
    }

    return err;
}

/**
 * Create a new thread for executing main()
 */
static pthread_t new_main_thread(int uargc, uintptr_t uargv, uintptr_t uenvp)
{
    struct buf * stack_region = (*curproc->mm.regions)[MM_STACK_REGION];
    struct buf * code_region = (*curproc->mm.regions)[MM_CODE_REGION];
    struct _sched_pthread_create_args args = {
        .param.sched_policy = current_thread->param.sched_policy,
        .param.sched_priority = current_thread->param.sched_priority,
        .stack_addr = (void *)(stack_region->b_mmu.vaddr),
        .stack_size = mmu_sizeof_region(&stack_region->b_mmu),
        .flags      = 0,
        .start      = (void *(*)(void *))(code_region->b_mmu.vaddr),
        .arg1       = uargc,
        .arg2       = uargv,
        .arg3       = uenvp,
        .arg4       = 0, /* TODO */
        .del_thread = NULL /* Not needed for main(). */
    };

    KASSERT(args.stack_size != 0,
            "Size of the main stack must be greater than zero\n");

    return thread_create(&args, 0);
}

/**
 * Execute a file.
 * File can be elf binary, she-bang file, etc.
 * @param file  is the executable file.
 */
int exec_file(file_t * file, char name[PROC_NAME_LEN], struct buf * env_bp,
              int uargc, uintptr_t uargv, uintptr_t uenvp)
{
    uintptr_t vaddr = 0; /* RFE Shouldn't matter if elf is not dyn? */
    pthread_t tid;
    int err, retval = 0;

#if defined(configEXEC_DEBUG)
    KERROR(KERROR_DEBUG, "exec_file(file %p, name \"%s\", env_bp %p, "
           "uargc %d, uargv %x,  uenvp %x)\n",
           file, name, env_bp, uargc, (uint32_t)uargv, (uint32_t)uenvp);
#endif

    err = load_proc_image(file, &vaddr);
    if (err) {
        retval = err;
        goto fail;
    }

    /* Map new environment */
    err = vm_insert_region(curproc, env_bp, VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
    if (err < 0) {
        if (env_bp->vm_ops->rfree)
            env_bp->vm_ops->rfree(env_bp);
        else {
            KERROR(KERROR_ERR, "Can't free env_bp\n");
        }
        retval = err;
        goto fail;
    }

    /* Change proc name */
    strlcpy(curproc->name, name, sizeof(curproc->name));

    /* Create main() thread */
    tid = new_main_thread(uargc - 1, uargv, uenvp);
    if (tid <= 0) {
        if (ksignal_sendsig_fatal(curproc, SIGKILL)) {
            /* Panic if the signal delivery fails. */
            panic("Exec failed");
        }
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
        curproc->main_thread = thread_lookup(tid);
        thread_flags_set(current_thread, SCHED_DETACH_FLAG);

        /* Don't return but die. */
        thread_die(0);
    }

    return retval;
}

/**
 * Clone an array of strings and remap to a new section mapped in user space.
 * @note vaddr must be set to the final value by the caller.
 */
static int clone_aa(struct buf * bp, __user char * uarr, size_t n_entries,
                    size_t * doffset)
{
    char ** arg = (char **)(bp->b_data + *doffset);
    char * val  = (char *)(bp->b_data + *doffset);
    size_t bytesleft = bp->b_bcount - *doffset;
    size_t offset = *doffset;
    int err;

    if (n_entries == 0)
        return 0;

    if (bytesleft <= n_entries * sizeof(char *))
        return -ENOMEM;

    err = copyin(uarr, arg, n_entries * sizeof(char *));
    if (err)
        return err;

    arg[n_entries - 1] = NULL;
    offset = n_entries * sizeof(char *) + sizeof(char *);
    bytesleft -= offset;

    for (size_t i = 0; i < n_entries; i++) {
        size_t copied;

        if (!arg[i])
            continue;

        err = copyinstr((__user char *)arg[i], val + offset, bytesleft,
                        &copied);
        if (err)
            return err;

        /* new pointer from arg[i] to the string, valid in user space. */
        arg[i] = (char *)(bp->b_mmu.vaddr + *doffset + offset);

        offset += copied;
        bytesleft -= copied;
    }

    *doffset = offset;

    return 0;
}

static int sys_exec(__user void * user_args)
{
    file_t * file;
    struct _exec_args args;
    char name[PROC_NAME_LEN];
    struct buf * env_bp;
    size_t arg_offset = 0;
    uintptr_t envp;
    int err, retval;

#if defined(configEXEC_DEBUG)
    KERROR(KERROR_DEBUG, "exec\n");
#endif

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (!args.argv || !args.env) {
        set_errno(EINVAL);
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
    env_bp->b_uflags = VM_PROT_READ | VM_PROT_WRITE;

    /* Clone argv */
    err = clone_aa(env_bp, (__user char *)args.argv, args.nargv, &arg_offset);
    if (err) {
#if defined(configEXEC_DEBUG)
        KERROR(KERROR_DEBUG, "Failed to clone args (%d)\n", err);
#endif
        set_errno(-err);
        retval = -1;
        goto out;
    }
    arg_offset = memalign(arg_offset);
    envp = env_bp->b_mmu.vaddr + arg_offset;

    /* Clone env */
    err = clone_aa(env_bp, (__user char *)args.env, args.nenv, &arg_offset);
    if (err) {
#if defined(configEXEC_DEBUG)
        KERROR(KERROR_DEBUG, "Failed to clone env (%d)\n", err);
#endif
        set_errno(-err);
        retval = -1;
        goto out;
    }

    strlcpy(name, (char *)(env_bp->b_data) + (args.nargv + 1) * sizeof(char *),
            sizeof(name));

    /*
     * Execute.
     */
    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        retval = -1;
        goto out;
    }
    err = exec_file(file, name, env_bp, args.nargv, env_bp->b_mmu.vaddr,
                    envp);
    fs_fildes_ref(curproc->files, args.fd, -1);
    if (err) {
        set_errno(-err);
        retval = -1;
        goto out;
    }

    retval = 0;
out:
    return retval;
}

static const syscall_handler_t exec_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_EXEC_EXEC, sys_exec),
};
SYSCALL_HANDLERDEF(exec_syscall, exec_sysfnmap)
