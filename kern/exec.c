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
#include <errno.h>
#include <kmalloc.h>
#include <buf.h>
#include <thread.h>
#include <proc.h>
#include <exec.h>

SET_DECLARE(exec_loader, struct exec_loadfn);

int exec_file(file_t * file, struct buf * environ)
{
    struct exec_loadfn ** loader;
    struct buf * old_environ;
    uintptr_t vaddr;
    pthread_t tid;
    int err, retval = 0;

    if (!file)
        return -ENOENT;

    old_environ = curproc->environ;
    curproc->environ = environ;

    SET_FOREACH(loader, exec_loader) {
        err = (*loader)->fn(file, &vaddr);
        if (err == 0 || err != -ENOEXEC)
            break;
    }
    if (err) {
        retval = err;
        goto fail;
    }

    /* Create a new thread for executing main() */
    pthread_attr_t pattr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = (void *)((*curproc->mm.regions)[MM_STACK_REGION]->b_mmu.vaddr),
        .stackSize  = configUSRINIT_SSIZE
    };
    struct _ds_pthread_create ds = {
        .thread     = 0, /* return value */
        .start      = (void *(*)(void *))((*curproc->mm.regions)[MM_CODE_REGION]->b_mmu.vaddr),
        .def        = &pattr,
        .argument   = NULL, /* TODO */
        .del_thread = NULL /* TODO should be libc: pthread_exit */
    };

    tid = thread_create(&ds, 0);
    if (tid <= 0) {
        panic("Exec failed");
    }

    goto out;
fail:
    curproc->environ = old_environ;
out:
    old_environ->vm_ops->rfree(old_environ);

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
