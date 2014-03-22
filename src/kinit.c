/**
 *******************************************************************************
 * @file    kinit.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kernel.h>
#include <kerror.h>
#include <usrinit.h>
#include <sched.h>
#include <proc.h>
#include <libkern.h>
#include <kstring.h>
#include <vralloc.h>
#include <kinit.h>

extern void (*__hw_preinit_array_start[]) (void) __attribute__((weak));
extern void (*__hw_preinit_array_end[]) (void) __attribute__((weak));

extern void (*__hw_postinit_array_start[]) (void) __attribute__((weak));
extern void (*__hw_postinit_array_end[]) (void) __attribute__((weak));

extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));

extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

static void exec_array(void (*a []) (void), int n);
void kinit(void) __attribute__((constructor));

/**
 * Run all kernel module initializers.
 */
void exec_init_array(void)
{
    int n;

    KERROR(KERROR_LOG, "ZeKe PreInit");

    n = __hw_preinit_array_end - __hw_preinit_array_start;
    exec_array(__hw_preinit_array_start, n);

    n  = __init_array_end - __init_array_start;
    exec_array(__init_array_start, n);

    n = __hw_postinit_array_end - __hw_postinit_array_start;
    exec_array(__hw_postinit_array_start, n);
}

/**
 * Run all kernel module finalizers.
 */
void exec_fini_array(void)
{
    int n = __fini_array_end - __fini_array_start;
    exec_array(__fini_array_start, n);
}

/**
 * Initialize main thread.
 */
void kinit(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(sched_init);
    SUBSYS_DEP(proc_init);

    /* Stack for init */
    vm_region_t * init_vmstack = vralloc(configUSRINIT_SSIZE);
    if (!init_vmstack)
        panic("Can't allocate stack for init");
    init_vmstack->usr_rw = VM_PROT_READ | VM_PROT_WRITE;
    init_vmstack->mmu.vaddr = init_vmstack->mmu.paddr;
    init_vmstack->mmu.ap = MMU_AP_RWRW;
    init_vmstack->mmu.control = MMU_CTRL_XN;

    /* Create app_main thread */
    pthread_attr_t init_attr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = (void *)(init_vmstack->mmu.vaddr),
        .stackSize  = configUSRINIT_SSIZE
    };
    ds_pthread_create_t init_ds = {
        .thread     = 0,
        .start      = main,
        .def        = &init_attr,
        .argument   = 0,
        .del_thread = pthread_exit
    };
    pthread_t tid; /* thread id of init main() */
    pid_t     pid; /* pid of init */
    threadInfo_t * init_thread;
    char buf[80];
    proc_info_t * init_proc;

    tid = sched_threadCreate(&init_ds, 0);
    if (tid <= 0) {
        ksprintf(buf, sizeof(buf), "Can't create a thread for init. %i", tid);
        panic(buf);
    }
    pid = proc_fork(0);
    if (pid <= 0) {
        ksprintf(buf, sizeof(buf), "Can't fork a process for init. %i", pid);
        panic(buf);
    }

    init_thread = sched_get_pThreadInfo(tid);
    if (!init_thread) {
        panic("Can't get init thread descriptor!");
    }
    init_thread->pid_owner = pid;
    init_proc = proc_get_struct(pid);
    if (!init_proc) {
        panic("Failed to get proc struct");
    }

    init_vmstack->mmu.pt = &(init_proc->mm.mptable);
    (*init_proc->mm.regions)[MM_CODE_REGION] = init_vmstack;
    init_proc->main_thread = init_thread;

    ksprintf(buf, sizeof(buf), "Init created with pid: %u & tid: %u", pid, tid);
    KERROR(KERROR_DEBUG, buf);
    SUBSYS_INITFINI("Load init");
}

static void random_init(void) __attribute__((constructor));
static void random_init(void)
{
    /* TODO Add some randomness ;) */
    KERROR(KERROR_LOG, "Seed random number generator");
    ksrandom(1);
}

/**
 * Exec intializer/finalizer array created by the linker.
 * @param pointer to the array.
 * @param n number of entries.
 */
static void exec_array(void (*a []) (void), int n)
{
    int i;

    for (i = 0; i < n; i++) {
        a[i]();
    }
}

