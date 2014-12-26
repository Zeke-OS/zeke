/**
 *******************************************************************************
 * @file    exec.c
 * @author  Olli Vanhoja
 * @brief   Execute a file.
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

#define KERNEL_INTERNAL 1
#include <syscall.h>
#include <errno.h>
#include <kmalloc.h>
#include <buf.h>
#include <thread.h>
#include <proc.h>
#include <unistd.h>
#include <exec.h>

SET_DECLARE(exec_loader, struct exec_loadfn);

/* TODO Add args and envp */
int exec_file(file_t * file)
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
        .arg1       = 0, /* TODO */
        .arg2       = 0,
        .arg3       = 0,
        .arg4       = 0,
        .del_thread = NULL /* Not needed for main(). */
    };

    tid = thread_create(&ds, 0);
    if (tid <= 0) {
        panic("Exec failed");
    }

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
        //curproc->main_thread->flags |= SCHED_DETACH_FLAG;
        curproc->main_thread = sched_get_thread_info(tid);
        thread_die(0); /* Don't return but die. */
    }

    return retval;
}

static int sys_exec(void * user_args)
{
    struct _exec_args args;
    file_t * file;
    int err, retval;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    /* Increment refcount for the file pointed by fd */
    file = fs_fildes_ref(curproc->files, args.fd, 1);
    if (!file) {
        set_errno(EBADF);
        return -1;
    }


    /* TODO Copy env and args */

    err = exec_file(file);
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
