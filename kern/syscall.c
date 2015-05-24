/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel's internal Syscall handler that is called from kernel scope.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *******************************************************************************
 */

#include <libkern.h>
#include <kstring.h>
#include <thread.h>
#include <proc.h>
#include <hal/core.h>
#include <kerror.h>
#include <errno.h>
#include <vm/vm.h>
#include <syscall.h>

/* For all Syscall groups */
#define FOR_ALL_SYSCALL_GROUPS(apply)               \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)       \
    apply(SYSCALL_GROUP_THREAD, thread_syscall)     \
    apply(SYSCALL_GROUP_SYSCTL, sysctl_syscall)     \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)    \
    apply(SYSCALL_GROUP_EXEC, exec_syscall)         \
    apply(SYSCALL_GROUP_PROC, proc_syscall)         \
    apply(SYSCALL_GROUP_IPC, ipc_syscall)           \
    apply(SYSCALL_GROUP_FS, fs_syscall)             \
    apply(SYSCALL_GROUP_IOCTL, ioctl_syscall)       \
    apply(SYSCALL_GROUP_SHMEM, shmem_syscall)       \
    apply(SYSCALL_GROUP_TIME, time_syscall)         \
    apply(SYSCALL_GROUP_PRIV, priv_syscall)

/*
 * Declare prototypes of syscall handlers.
 */
#define DECLARE_SCHANDLER(major, function) \
    extern intptr_t function(uint32_t type, __user void * p);
FOR_ALL_SYSCALL_GROUPS(DECLARE_SCHANDLER)
#undef DECLARE_SCHANDLER

static const kernel_syscall_handler_t syscall_callmap[] = {
    #define SYSCALL_MAP_X(major, function) [major] = function,
    FOR_ALL_SYSCALL_GROUPS(SYSCALL_MAP_X)
    #undef SYSCALL_MAP_X
};

/**
 * Kernel's internal Syscall handler/translator.
 *
 * This function is called from interrupt handler. This function calls the
 * actual kernel function and returns a result pointer/data to the interrupt
 * handler which returns it to the original caller, which is usually a library
 * function in kernel.c
 */
void syscall_handler(void)
{
    sw_stack_frame_t * sframe = &current_thread->sframe[SCHED_SFRAME_SVC];
    const uint32_t type = (uint32_t)sframe->r0;
    __user void * p = (__user void *)sframe->r1;
    const uint32_t major = SYSCALL_MAJOR(type);
    intptr_t retval;

    thread_flags_set(current_thread, SCHED_INSYS_FLAG);

    if ((major >= num_elem(syscall_callmap)) || !syscall_callmap[major]) {
        const uint32_t minor = SYSCALL_MINOR(type);

        KERROR(KERROR_WARN,
                 "syscall %u:%u not supported, (pid:%u, tid:%u, pc:%x)\n",
                 major, minor,
                 (unsigned)current_process_id,
                 (unsigned)current_thread->id,
                 sframe->pc);

        set_errno(ENOSYS); /* Not supported. */
        retval = -1;
    } else {
        retval = syscall_callmap[major](type, p);
    }

    thread_flags_clear(current_thread, SCHED_INSYS_FLAG);
    svc_setretval(retval);
}
