/**
 *******************************************************************************
 * @file    proc.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
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

#define KERNEL_INTERNAL 1
#include <autoconf.h>
#include <tsched.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <kinit.h>
#include <syscall.h>
#include <unistd.h>
#include <errno.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <fcntl.h>
#include <ptmapper.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <buf.h>
#include <thread.h>
#include <proc.h>
#include "_proc.h"

static proc_info_t *(*_procarr)[];  /*!< processes indexed by pid */
int maxproc = configMAXPROC;        /*!< Maximum # of processes, set. */
int act_maxproc;                    /*!< Effective maxproc. */
int nprocs = 1;                     /*!< Current # of procs. */
pid_t current_process_id;           /*!< PID of current process. */
proc_info_t * curproc;              /*!< PCB of the current process. */

extern vnode_t kerror_vnode;

extern void * __bss_break __attribute__((weak));

#define SIZEOF_PROCARR()    ((maxproc + 1) * sizeof(proc_info_t *))

/** proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
mtx_t proclock;

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RWTUN,
    &maxproc, 0, "Maximum number of processes");
SYSCTL_INT(_kern, OID_AUTO, nprocs, CTLFLAG_RD,
    &nprocs, 0, "Current number of processes");

static void init_kernel_proc(void);
static void procarr_remove(pid_t pid);
static void proc_remove(pid_t pid);
pid_t proc_update(void); /* Used in HAL, so not static but not in headeaders. */

void proc_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(vralloc_init);

    PROC_LOCK_INIT();
    procarr_realloc();
    memset(_procarr, 0, SIZEOF_PROCARR());

    init_kernel_proc();

    /* Do here same as proc_update() would do when running. */
    current_process_id = 0;
    curproc = (*_procarr)[0];

    SUBSYS_INITFINI("proc OK");
}

/**
 * Initialize kernel process 0.
 */
static void init_kernel_proc(void)
{
    const char panic_msg[] = "Can't init kernel process";
    proc_info_t * kernel_proc;

    (*_procarr)[0] = kcalloc(1, sizeof(proc_info_t));
    kernel_proc = (*_procarr)[0];

    kernel_proc->pid = 0;
    kernel_proc->state = PROC_STATE_RUNNING; /* TODO */
    strcpy(kernel_proc->name, "kernel");

    RB_INIT(&(kernel_proc->mm.ptlist_head));

    /* Copy master page table descriptor */
    memcpy(&(kernel_proc->mm.mpt), &mmu_pagetable_master,
            sizeof(mmu_pagetable_t));
    kernel_proc->mm.curr_mpt = &(kernel_proc->mm.mpt);

    /* Insert page tables */
    /* TODO Remove following lines completely? */
#if 0
    struct vm_pt * vpt;
    vpt = kmalloc(sizeof(struct vm_pt));
    vpt->pt = mmu_pagetable_system;
    vpt->linkcount = 1;
    RB_INSERT(ptlist, &(kernel_proc->mm.ptlist_head), vpt);
#endif

    /* Create regions */
    kernel_proc->mm.regions = kcalloc(3, sizeof(void *));
    kernel_proc->mm.nr_regions = 3;
    if (!kernel_proc->mm.regions) {
        panic(panic_msg);
    }

    /* Copy region descriptors */
    struct buf * kprocvm_code = kcalloc(1, sizeof(struct buf));
    struct buf * kprocvm_heap = kcalloc(1, sizeof(struct buf));
    if (!(kprocvm_code && kprocvm_heap)) {
        panic(panic_msg);
    }

    kprocvm_code->b_mmu = mmu_region_kernel;
    kprocvm_heap->b_mmu = mmu_region_kdata;

    mtx_init(&(kprocvm_code->lock), MTX_TYPE_SPIN);
    mtx_init(&(kprocvm_heap->lock), MTX_TYPE_SPIN);

    (*kernel_proc->mm.regions)[MM_CODE_REGION] = kprocvm_code;
    (*kernel_proc->mm.regions)[MM_STACK_REGION] = 0;
    (*kernel_proc->mm.regions)[MM_HEAP_REGION] = kprocvm_heap;

    /* Break values */
    kernel_proc->brk_start = &__bss_break;
    kernel_proc->brk_stop = (void *)(kprocvm_heap->b_mmu.vaddr
        + mmu_sizeof_region(&(kprocvm_heap->b_mmu)) - 1);

    /* File descriptors */
    /* TODO We have a hard limit of 8 files here now but this should be tunable
     * by using setrlimit() Also we may want to set this smaller at some point.
     */
    kernel_proc->files = kcalloc(1, SIZEOF_FILES(8));
    if (!kernel_proc->files) {
        panic(panic_msg);
    }
    kernel_proc->files->count = 8;
    kernel_proc->files->umask = 022; /* File creation mask: S_IWGRP|S_IWOTH */

    /* TODO Do this correctly */
    kernel_proc->files->fd[STDIN_FILENO] = 0;
    kernel_proc->files->fd[STDOUT_FILENO] = 0;
    /* stderr */
    kernel_proc->files->fd[STDERR_FILENO] = kcalloc(1, sizeof(file_t));
    if (fs_fildes_set(kernel_proc->files->fd[STDERR_FILENO],
                &kerror_vnode, O_WRONLY))
        panic(panic_msg);
}

void procarr_realloc(void)
{
    proc_info_t * (*tmp)[];

    /* Skip if size is not changed */
    if (maxproc == act_maxproc)
        return;

    PROC_LOCK();
    tmp = krealloc(_procarr, SIZEOF_PROCARR());
    if ((tmp == 0) && (_procarr == 0)) {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "Unable to allocate _procarr (%u bytes)",
                SIZEOF_PROCARR());
        panic(buf);
    }
    _procarr = tmp;
    act_maxproc = maxproc;
    PROC_UNLOCK();
}

void procarr_insert(proc_info_t * new_proc)
{
    PROC_LOCK();
    /* TODO Bounds check */
    (*_procarr)[new_proc->pid] = new_proc;
    nprocs++;
    PROC_UNLOCK();
}

static void procarr_remove(pid_t pid)
{
    PROC_LOCK();
    /* TODO Bounds check */
    (*_procarr)[pid] = 0;
    nprocs--;
    PROC_UNLOCK();
}

/**
 * Remove zombie process from the system.
 */
static void proc_remove(pid_t pid)
{
    proc_info_t * p;

    /* TODO free everything */
    if (!(p = proc_get_struct(pid)))
        return;

    _proc_free(p);
    procarr_remove(pid);
}

void _proc_free(proc_info_t * p)
{
    if (p)
        return;

    /* Free files */
    if (p->files) {
        for (int i = 0; i < p->files->count; i++) {
            fs_fildes_ref(p->files, i, -1); /* null pointer safe */
        }
        kfree(p->files);
    }

    /* Free regions */
    if (p->mm.regions) {
        for (int i = 0; i < p->mm.nr_regions; i++) {
            if ((*p->mm.regions)[i]->vm_ops->rfree)
                    (*p->mm.regions)[i]->vm_ops->rfree((*p->mm.regions)[i]);
        }
        p->mm.nr_regions = 0;

        /* Free page table list */
        ptlist_free(&(p->mm.ptlist_head));

        /* Free regions array */
        kfree(p->mm.regions);
    }

    if (p->mm.mpt.pt_addr)
        ptmapper_free(&(p->mm.mpt));

    kfree(p);
}

#if 0
/**
 * Initialize a new process.
 * @param image Process image to be loaded.
 * @param size  Size of the image.
 * @return  PID; -1 if unable to initialize.
 */
pid_t process_init(void * image, size_t size)
{
    return -1;
}
#endif

proc_info_t * proc_get_struct(pid_t pid)
{
    /* TODO We actually want to require proclock */
#if 0
    if (!PROC_TESTLOCK()) {
        KERROR(KERROR_WARN, "proclock is required before entering proc_get_struct()");
    }
#endif

    /* TODO do state check properly */
    if (pid > act_maxproc) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "Invalid PID : %u\ncurrpid : %u\nmaxproc : %i\nstate %x",
                pid, current_process_id, act_maxproc,
                ((*_procarr)[pid]) ? (*_procarr)[pid]->state : 0);
        KERROR(KERROR_DEBUG, buf);

        return 0;
    }
    return (*_procarr)[pid];
}

/* Called when thread is completely removed from the scheduler */
void proc_thread_removed(pid_t pid, pthread_t thread_id)
{
    proc_info_t * p;

    if (!(p = proc_get_struct(pid)))
        return;

    if (p->main_thread && (p->main_thread->id == thread_id)) {
        p->main_thread = 0;
        p->state = PROC_STATE_ZOMBIE;
    }
}

void proc_enter_kernel(void)
{
#if configDEBUG >= KERROR_DEBUG
    if (!curproc)
        panic("No current process set");
#endif
    curproc->mm.curr_mpt = &mmu_pagetable_master;
}

mmu_pagetable_t * proc_exit_kernel(void)
{
#if configDEBUG >= KERROR_DEBUG
    if (!curproc)
        panic("No current proces set");
#endif
    curproc->mm.curr_mpt = &curproc->mm.mpt;
    return curproc->mm.curr_mpt;
}

void proc_suspend(void)
{
#if configDEBUG >= KERROR_DEBUG
    if (!curproc)
        panic("No current proces set");
#endif
    /* TODO set state */
    //curproc->state
}

mmu_pagetable_t * proc_resume(void)
{
#if configDEBUG >= KERROR_DEBUG
    if (!curproc)
        panic("No current proces set");
#endif
    /* TODO set state */
    return curproc->mm.curr_mpt;
}

int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        threadInfo_t * thread)
{
    const pid_t pid = thread->pid_owner;
    const uintptr_t vaddr = far;
    proc_info_t * pcb;
    struct buf * region;
    struct buf * new_region;

#if configDEBUG >= KERROR_DEBUG
    char buf[80];
    ksprintf(buf, sizeof(buf), "proc_dab_handler(): MOO, %x @ %x", vaddr, lr);
    KERROR(KERROR_DEBUG, buf);
#endif

    pcb = proc_get_struct(pid);
    if (!pcb || (pcb->state == PROC_STATE_INITIAL)) {
        return PROC_DABERR_NOPROC; /* Process doesn't exist. */
    }

    for (int i = 0; i < pcb->mm.nr_regions; i++) {
        region = ((*pcb->mm.regions)[i]);
        ksprintf(buf, sizeof(buf), "reg_vaddr %x, reg_end %x",
                region->b_mmu.vaddr,
                region->b_mmu.vaddr + MMU_SIZEOF_REGION(&(region->b_mmu)));
        KERROR(KERROR_DEBUG, buf);

        if (vaddr >= region->b_mmu.vaddr &&
                vaddr <= (region->b_mmu.vaddr + MMU_SIZEOF_REGION(&(region->b_mmu)))) {
            /* Test for COW flag. */
            if ((region->b_uflags & VM_PROT_COW) != VM_PROT_COW) {
                return PROC_DABERR_PROT; /* Memory protection error. */
            }

            if (!(region->vm_ops->rclone)
                    || !(new_region = region->vm_ops->rclone(region))) {
                /* Can't clone region; COW clone failed. */
                return PROC_DABERR_ENOMEM;
            }

            /* Free the old region as this process no longer uses it.
             * (Usually decrements some internal refcount) */
            if (region->vm_ops->rfree)
                region->vm_ops->rfree(region);

            new_region->b_uflags &= ~VM_PROT_COW; /* No more COW. */
            (*pcb->mm.regions)[i] = new_region; /* Replace old region. */
            mmu_map_region(&(new_region->b_mmu)); /* Map it to the page table. */

            return 0; /* COW done. */
        }
    }

    return PROC_DABERR_INVALID; /* Not found */
}

pid_t proc_update(void)
{
    current_process_id = current_thread->pid_owner;
    curproc = proc_get_struct(current_process_id);

    return current_process_id;
}

struct buf * proc_newsect(uintptr_t vaddr, size_t size, int prot)
{
    struct buf * new_region = geteblk(size);

    if (!new_region)
        return NULL;

    new_region->b_uflags = prot & ~VM_PROT_COW;
    new_region->b_mmu.vaddr = vaddr;
    new_region->b_mmu.ap = MMU_AP_NANA;
    new_region->b_mmu.control = MMU_CTRL_NG |
        ((prot | VM_PROT_EXECUTE) ? 0 : MMU_CTRL_XN);
    vm_updateusr_ap(new_region);

    return new_region;
}

int proc_replace(pid_t pid, struct buf * (*regions)[], int nr_regions)
{
    proc_info_t * p = proc_get_struct(pid);

    if (!p)
        return -ESRCH;

    /*
     * Must disable interrupts here because there is no way getting back here
     * after a context switch.
     */
    disable_interrupt();

    /*
     * Mark main thread for deletion, it's up to user space to kill any
     * children. If there however is any child threads those may or may
     * not cause a segmentation fault depending on when the scheduler
     * starts removing stuff. This decission was made because we want to
     * keep disable_interrupt() time as short as possible and POSIX seems
     * to be quite silent about this issue anyway.
     */
    p->main_thread->flags |= SCHED_DETACH_FLAG; /* This kills the man. */

    /* TODO Free old regions struct and its contents */

    p->mm.regions = regions;
    p->mm.nr_regions = nr_regions;

    /* Map regions */
    for (int i = 0; i < nr_regions; i++) {
        struct vm_pt * vpt = ptlist_get_pt(&p->mm.ptlist_head, &p->mm.mpt,
                (*regions)[i]->b_mmu.vaddr);
        if (!vpt) {
            panic("Exec failed");
        }

        (*regions)[i]->b_mmu.pt = &vpt->pt;
        vm_map_region((*regions)[i], vpt);
    }

    /* Create a new thread for executing main() */
    pthread_attr_t pattr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = (void *)((*regions)[MM_STACK_REGION]->b_mmu.paddr),
        .stackSize  = configUSRINIT_SSIZE
    };
    struct _ds_pthread_create ds = {
        .thread     = 0, /* return value */
        .start      = (void *(*)(void *))((*regions)[MM_CODE_REGION]->b_mmu.vaddr),
        .def        = &pattr,
        .argument   = 0, /* TODO */
        .del_thread = pthread_exit /* TODO */
    };

    const pthread_t tid = thread_create(&ds, 0);
    if (tid <= 0) {
        panic("Exec failed");
    }

    /* interrupts will be enabled automatically */
    thread_die(0);

    return 0; /* Never returns */
}

static uintptr_t procsys_wait(void * p)
{
    pid_t pid_child;

    if (!curproc->inh.first_child) {
        /* The calling process has no existing unwaited-for child
         * processes. */
        set_errno(ECHILD);
        return -1;
    }
    pid_child = curproc->inh.first_child->pid;

    /* curproc->state = PROC_STATE_WAITING needs some other flag? */
    while (curproc->inh.first_child->state != PROC_STATE_ZOMBIE) {
        idle_sleep();
        /* TODO In some cases we have to return early without waiting.
         * eg. signal received */
    }

    if (!useracc(p, sizeof(pid_t), VM_PROT_WRITE)) {
        /* TODO Signal */
        return -1;
    }

    copyout(&curproc->inh.first_child->exit_code, p, sizeof(int));

    proc_remove(pid_child);
    return (uintptr_t)pid_child;
}

intptr_t proc_syscall(uint32_t type, void * p)
{
    switch (type) {
    case SYSCALL_PROC_EXEC: /* note: can only return EAGAIN or ENOMEM */
        set_errno(ENOSYS);
        return -1;

    case SYSCALL_PROC_FORK:
    {
        pid_t pid = proc_fork(current_process_id);
        if (pid < 0) {
            set_errno(-pid);
            return -1;
        } else {
            return pid;
        }
    }

    case SYSCALL_PROC_WAIT:
        return procsys_wait(p);

    case SYSCALL_PROC_EXIT:
        curproc->exit_code = get_errno();
        sched_thread_detach(current_thread->id);
        thread_die(curproc->exit_code);
        return 0; /* Never reached */

    case SYSCALL_PROC_GETUID:
        set_errno(ENOSYS);
        return -5;

    case SYSCALL_PROC_GETEUID:
        set_errno(ENOSYS);
        return -6;

    case SYSCALL_PROC_GETGID:
        set_errno(ENOSYS);
        return -7;

    case SYSCALL_PROC_GETEGID:
        set_errno(ENOSYS);
        return -8;

    case SYSCALL_PROC_GETPID:
        set_errno(ENOSYS);
        return -9;

    case SYSCALL_PROC_GETPPID:
        set_errno(ENOSYS);
        return -10;

    case SYSCALL_PROC_ALARM:
        set_errno(ENOSYS);
        return -13;

    case SYSCALL_PROC_CHDIR:
        set_errno(ENOSYS);
        return -14;

    case SYSCALL_PROC_SETPRIORITY:
        set_errno(ENOSYS);
        return -15;

    case SYSCALL_PROC_GETPRIORITY:
        set_errno(ENOSYS);
        return -15;

    case SYSCALL_PROC_GETBREAK:
        {
        struct _ds_getbreak ds;

        if (!useracc(p, sizeof(struct _ds_getbreak), VM_PROT_WRITE)) {
            /* No permission to read/write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -1;
        }
        copyin(p, &ds, sizeof(struct _ds_getbreak));
        ds.start = curproc->brk_start;
        ds.stop = curproc->brk_stop;
        copyout(&ds, p, sizeof(struct _ds_getbreak));
        }
        return 0;

    default:
        return (uint32_t)NULL;
    }
}
