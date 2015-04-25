/**
 *******************************************************************************
 * @file    kinit.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kerror.h>
#include <proc.h>
#include <thread.h>
#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <buf.h>
#include <kinit.h>

extern int (*__hw_preinit_array_start[]) (void) __attribute__((weak));
extern int (*__hw_preinit_array_end[]) (void) __attribute__((weak));

extern int (*__hw_postinit_array_start[]) (void) __attribute__((weak));
extern int (*__hw_postinit_array_end[]) (void) __attribute__((weak));

extern int (*__init_array_start []) (void) __attribute__((weak));
extern int (*__init_array_end []) (void) __attribute__((weak));

extern int (*__fini_array_start []) (void) __attribute__((weak));
extern int (*__fini_array_end []) (void) __attribute__((weak));

extern void kmalloc_init(void);
extern void * uinit(void * arg);
static void exec_array(int (*a []) (void), int n);

/**
 * Run all kernel module initializers.
 */
int exec_init_array(void)
{
    int n;

    KERROR(KERROR_INFO, "ZeKe PreInit\n");

    n = __hw_preinit_array_end - __hw_preinit_array_start;
    exec_array(__hw_preinit_array_start, n);

    kmalloc_init();

    n  = __init_array_end - __init_array_start;
    exec_array(__init_array_start, n);

    disable_interrupt();
    n = __hw_postinit_array_end - __hw_postinit_array_start;
    exec_array(__hw_postinit_array_start, n);
    enable_interrupt();

    return 0;
}

/**
 * Run all kernel module finalizers.
 */
void exec_fini_array(void)
{
    int n = __fini_array_end - __fini_array_start;
    exec_array(__fini_array_start, n);
}

static pthread_t create_uinit_main(void * stack_addr)
{
    struct _sched_pthread_create_args init_ds = {
        .param.sched_policy = SCHED_OTHER,
        .param.sched_priority = configUSRINIT_PRI,
        .stack_addr = stack_addr,
        .stack_size = configUSRINIT_SSIZE,
        .flags      = 0,
        .start      = uinit, /* We have to first get into user space to use exec
                              * and mount the rootfs.
                              */
        .arg1       = 0,
        .del_thread = 0xBADBAD /* TODO  Should be libc pthread_exit but we don't
                                *       yet know where it will be.
                                */
    };

    return thread_create(&init_ds, 0);
}

static void mount_rootfs(void)
{
    const char failed[] = "Failed to mount rootfs";
    vnode_t * tmp = NULL;
    struct proc_info * kernel_proc;
    int ret;

    kernel_proc = proc_get_struct_l(0);
    if (!kernel_proc) {
        panic(failed);
    }

    /* Root dir */
    tmp = kmalloc(sizeof(vnode_t));
    if (!tmp) {
        panic(failed);
    }
    kernel_proc->croot = tmp;
    kernel_proc->croot->vn_next_mountpoint = kernel_proc->croot;
    kernel_proc->croot->vn_prev_mountpoint = kernel_proc->croot;
    mtx_init(&tmp->vn_lock, MTX_TYPE_SPIN, 0);
    vrefset(kernel_proc->croot, 2);

    ret = fs_mount(kernel_proc->croot, "", "ramfs", 0, "", 1);
    if (ret) {
        KERROR(KERROR_ERR, "%s : %i\n", failed, ret);
        goto out;
    }

    kernel_proc->croot->vn_next_mountpoint->vn_prev_mountpoint =
        kernel_proc->croot->vn_next_mountpoint;
    kernel_proc->croot = kernel_proc->croot->vn_next_mountpoint;
    kernel_proc->cwd = kernel_proc->croot;

out:
    kfree(tmp);
}

static struct buf * create_vmstack(void)
{
    struct buf * vmstack;

    vmstack = geteblk(configUSRINIT_SSIZE);
    if (!vmstack)
        return NULL;

    vmstack->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
    vmstack->b_mmu.vaddr = vmstack->b_mmu.paddr;
    vmstack->b_mmu.ap = MMU_AP_RWRW;
    vmstack->b_mmu.control = MMU_CTRL_XN;

    return vmstack;
}

/**
 * Map vmstack to proc.
 */
static void map_vmstack2proc(struct proc_info * proc, struct buf * vmstack)
{
    struct vm_pt * vpt;

    (*proc->mm.regions)[MM_STACK_REGION] = vmstack;
    vm_updateusr_ap(vmstack);

    vpt = ptlist_get_pt(&proc->mm, vmstack->b_mmu.vaddr);
    if (vpt == 0)
        panic("Couldn't get vpt for init stack");

    vmstack->b_mmu.pt = &(vpt->pt);
    vm_map_region(vmstack, vpt);
}

/**
 * Create init process.
 */
int kinit(void) __attribute__((constructor));
int kinit(void)
{
    SUBSYS_DEP(sched_init);
    SUBSYS_DEP(proc_init);
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("kinit");

    char strbuf[80]; /* Buffer for panic messages. */
    struct buf * init_vmstack;
    pthread_t tid;
    pid_t pid;
    struct thread_info * init_thread;
    struct proc_info * init_proc;

    mount_rootfs();

    /*
     * User stack for init
     */
    init_vmstack = create_vmstack();
    if (!init_vmstack)
        panic("Can't allocate a stack for init");

    /*
     * Create a thread for init
     */
    tid = create_uinit_main((void *)(init_vmstack->b_mmu.paddr));
    if (tid < 0) {
        ksprintf(strbuf, sizeof(strbuf), "Can't create a thread for init. %i",
                 tid);
        panic(strbuf);
    }

    /*
     * pid of init
     */
    pid = proc_fork(0);
    if (pid <= 0) {
        ksprintf(strbuf, sizeof(strbuf), "Can't fork a process for init. %i",
                 pid);
        panic(strbuf);
    }

    init_thread = thread_lookup(tid);
    if (!init_thread) {
        panic("Can't get thread descriptor of init_thread!");
    }

    init_proc = proc_get_struct_l(pid);
    if (!init_proc || (init_proc->state == PROC_STATE_INITIAL)) {
        panic("Failed to get proc struct or invalid struct");
    }

    init_thread->pid_owner = pid;
    init_thread->curr_mpt = &init_proc->mm.mpt;

    /*
     * Map the previously created user stack with init process page table.
     */
    map_vmstack2proc(init_proc, init_vmstack);

    /*
     * Map tkstack of init with mmu_pagetable_system
     */
    mmu_map_region(&(init_thread->kstack_region->b_mmu));
    init_proc->main_thread = init_thread;

#if configDEBUG >= KERROR_INFO
    KERROR(KERROR_DEBUG,
           "Init created with pid: %u, tid: %u, stack: %x\n",
           pid, tid, init_vmstack->b_mmu.vaddr);
#endif

    return 0;
}

/**
 * Exec intializer/finalizer array created by the linker.
 * @param pointer to the array.
 * @param n number of entries.
 */
static void exec_array(int (*a []) (void), int n)
{
    int i;

    for (i = 0; i < n; i++) {
        exec_initfn(a[i]);
    }
}

void exec_initfn(int (*fn)(void))
{
    int err;

    err = fn();

    if (err == 0)
        kputs("\r\t\t\t\tOK\n");
    else if (err != -EAGAIN)
        kputs("\r\t\t\t\tFAILED\n");
}
