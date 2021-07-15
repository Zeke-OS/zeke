/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
 * @section LICENSE
 * Copyright (c) 2019 - 2021 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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
 *
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *        The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *        @(#)kern_sig.c        8.7 (Berkeley) 4/18/94
 *******************************************************************************
 */


#include <errno.h>
#include <sys/_signames.h>
#include <sys/types.h>
#include <ksched.h>
#include <thread.h>
#include <proc.h>
#include <kstring.h>
#include <vm/vm.h>
#include "ksignal.h"

/**
 * Pop from thread stack to dest.
 */
static int thread_stack_pop(struct thread_info * thread, void * buf,
                            size_t size)
{
    sw_stack_frame_t * sframe;
    __user void * sp;
    int err;

    KASSERT(size > 0, "size should be greater than zero.\n");

    sframe = get_usr_sframe(thread);
    if (!sframe)
        return -EINVAL;

    sp = (__user void *)sframe->sp;
    if (!sp)
        return -EFAULT;

    err = copyin(sp, buf, size);
    if (err)
        return err;

    sframe->sp += memalign(size);

    return 0;
}

/**
 * Push 'src' to thread stack.
 * @param thread    thread.
 * @param src       is a pointer to to data to be pushed.
 * @param size      is the size of data pointer by src.
 * @param old_thread_sp[in] returns the old thread stack pointer, can be NULL.
 * @return Error code or zero.
 */
static int thread_stack_push(struct thread_info * thread, const void * src,
                             size_t size, void ** old_thread_sp)
{
    sw_stack_frame_t * sframe;
    void * old_sp;
    void * new_sp;
    int err;

    KASSERT(size > 0, "size should be greater than zero.\n");

    sframe = get_usr_sframe(thread);
    if (!sframe)
        return -EINVAL;

    old_sp = (void *)sframe->sp;
    new_sp = (void *)((uintptr_t)old_sp - memalign(size));
    if (!old_sp)
        return -EFAULT;

    err = copyout(src, (__user void *)new_sp, size);
    if (err)
        return -EFAULT;

    sframe->sp = (uintptr_t)new_sp;
    if (old_thread_sp)
        *old_thread_sp = old_sp;

    return 0;
}

int ksignal_branch_sighandler(int signum,
                              const struct ksigaction * restrict action,
                              const siginfo_t * siginfo)
{
    const uintptr_t usigret = curproc->usigret;
    sw_stack_frame_t * tsfp = get_usr_sframe(current_thread);
    void * old_thread_sp; /* this is used to revert signal handling state
                           * and return to normal execution. */

    if (/* Push current stack frame to the user space thread stack. */
        thread_stack_push(current_thread,
                          tsfp,
                          sizeof(sw_stack_frame_t),
                          NULL) ||
        /* Push siginfo struct. */
        thread_stack_push(current_thread,
                          siginfo,
                          sizeof(siginfo_t),
                          &old_thread_sp /* Address of the prev sframe. */)
       ) {
        KERROR(KERROR_ERR, "Failed to push signum %s\n",
               ksignal_signum2str(signum));

        return -EINVAL;
    }

    if (usigret < configEXEC_BASE_LIMIT) {
        KERROR(KERROR_WARN, "usigret addr probably invalid (%x) for proc %i\n",
               (unsigned)usigret, (int)curproc->pid);
    }

    tsfp->pc = (uintptr_t)action->ks_action.sa_sigaction;
    tsfp->r0 = signum;                      /* arg1 = signum */
    tsfp->r1 = tsfp->sp;                    /* arg2 = siginfo */
    tsfp->r2 = 0;                           /* arg3 = TODO context */
    tsfp->r9 = (uintptr_t)old_thread_sp;    /* old stack frame */
    tsfp->lr = usigret;

    return 0;
}

int ksignal_syscall_exit_stack_fixup_sighandler(int retval)
{
    sw_stack_frame_t * sframe = get_usr_sframe(current_thread);
    sw_stack_frame_t caller;

    KASSERT(sframe != NULL, "Must have exitting sframe");

    /*
     * Set return value for the syscall.
     * FIXME The error returned by copyin() and copyin() should be either
     *       ignored explicitly by using (void) or handled.
     */
    copyin((__user sw_stack_frame_t *)sframe->r9, &caller, sizeof(caller));
    caller.r0 = retval;
    copyout(&caller, (__user sw_stack_frame_t *)sframe->r9, sizeof(caller));

    /* This will be the first argument for the signal handler. */
    return sframe->r0;
}

intptr_t ksignal_sys_signal_return(__user void * user_args)
{
    /* FIXME HW dependent. */
    sw_stack_frame_t * sframe = &current_thread->sframe.s[SCHED_SFRAME_SVC];
    sw_stack_frame_t next;
    uintptr_t sp;
    int err;

    /*
     * TODO
     * Return from signal handler
     * - revert stack frame and alt stack
     */

    sframe->sp = sframe->r9;
    err = thread_stack_pop(current_thread,
                           &next,
                           sizeof(const sw_stack_frame_t));
    if (err) {
        const struct ksignal_param sigparm = { .si_code = ILL_BADSTK };

        /*
         * RFE Should we punish only the thread or whole process?
         */
        ksignal_sendsig_fatal(curproc, SIGILL, &sigparm);
        while (1) {
            thread_wait();
            /* Should not return to here */
        }
    }
    sp = sframe->sp;
    memcpy(sframe, &next, sizeof(const sw_stack_frame_t));
    sframe->sp = sp;

    /*
     * We return for now but the actual return from this system call will
     * happen to the place that was originally interrupted by a signal.
     */
    return sframe->r0;
}
