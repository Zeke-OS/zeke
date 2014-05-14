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
#include "../sbin/init/init.h" /* TODO To be removed */
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

/**
 * Run all kernel module initializers.
 */
void exec_init_array(void)
{
    int n;

    KERROR(KERROR_INFO, "ZeKe PreInit");

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
 * Create init process.
 */
void kinit(void) __attribute__((constructor));
void kinit(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(sched_init);
    SUBSYS_DEP(proc_init);

    char buf[80]; /* Buffer for panic messages. */

    /* User stack for init */
    vm_region_t * init_vmstack = vralloc(configUSRINIT_SSIZE);
    if (!init_vmstack)
        panic("Can't allocate a stack for init");

    init_vmstack->usr_rw = VM_PROT_READ | VM_PROT_WRITE;
    init_vmstack->mmu.vaddr = init_vmstack->mmu.paddr;
    init_vmstack->mmu.ap = MMU_AP_RWRW;
    init_vmstack->mmu.control = MMU_CTRL_XN;

    /* Create a thread for init */
    pthread_attr_t init_attr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = (void *)(init_vmstack->mmu.paddr),
        .stackSize  = configUSRINIT_SSIZE
    };
    ds_pthread_create_t init_ds = {
        .thread     = 0, /* return value */
        .start      = main,
        .def        = &init_attr,
        .argument   = 0,
        .del_thread = pthread_exit
    };

    /* thread id of init main() */
    const pthread_t tid = sched_threadCreate(&init_ds, 0);
    if (tid <= 0) {
        ksprintf(buf, sizeof(buf), "Can't create a thread for init. %i", tid);
        panic(buf);
    }

    /* pid of init */
    const pid_t pid = proc_fork(0);
    if (pid <= 0) {
        ksprintf(buf, sizeof(buf), "Can't fork a process for init. %i", pid);
        panic(buf);
    }

    threadInfo_t * const init_thread = sched_get_pThreadInfo(tid);
    if (!init_thread) {
        panic("Can't get thread descriptor of init_thread!");
    }

    proc_info_t * const init_proc = proc_get_struct(pid);
    if (!init_proc || (init_proc->state == PROC_STATE_INITIAL)) {
        panic("Failed to get proc struct or invalid struct");
    }

    init_thread->pid_owner = pid;

    /* Map previously created user stack with init process page table. */
    {
        struct vm_pt * vpt;

        (*init_proc->mm.regions)[MM_STACK_REGION] = init_vmstack;
        vm_updateusr_ap(init_vmstack);

        vpt = ptlist_get_pt(
                &(init_proc->mm.ptlist_head),
                &(init_proc->mm.mpt),
                init_vmstack->mmu.vaddr);
        if (vpt == 0)
            panic("Couldn't get vpt for init stack");

        init_vmstack->mmu.pt = &(vpt->pt);
        vm_map_region(init_vmstack, vpt);
    }

    /* Map tkstack of init with mmu_pagetable_system */
    mmu_map_region(&(init_thread->kstack_region->mmu));
    init_proc->main_thread = init_thread;

#if configDEBUG >= KERROR_INFO
    ksprintf(buf, sizeof(buf), "Init created with pid: %u, tid: %u, stack: %x",
            pid, tid, init_vmstack->mmu.vaddr);
    KERROR(KERROR_DEBUG, buf);
#endif
    SUBSYS_INITFINI("Load init");
}

static void random_init(void) __attribute__((constructor));
static void random_init(void)
{
    SUBSYS_INIT();

    /* TODO Add some randomness ;) */
    ksrandom(1);

    SUBSYS_INITFINI("Seed random number generator");
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

