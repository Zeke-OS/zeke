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

/** @addtogroup Process
  * @{
  */

#define KERNEL_INTERNAL 1
#include <sched.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <kinit.h>
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <ptmapper.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <vralloc.h>
#include <proc.h>

static proc_info_t *(*_procarr)[];  /*!< processes indexed by pid */
int maxproc = configMAXPROC;        /*!< Maximum # of processes, set. */
static int _cur_maxproc;            /*!< Effective maxproc. */
int nprocs = 1;                     /*!< Current # of procs. */
pid_t current_process_id;           /*!< PID of current process. */
proc_info_t * curproc;              /*!< PCB of the current process. */
static pid_t _lastpid;              /*!< last allocated pid. */

extern vnode_t kerror_vnode;

extern void * __bss_break __attribute__((weak));

#define SIZEOF_PROCARR()    ((maxproc + 1) * sizeof(proc_info_t *))

/* proclock - Protects procarray and variables in proc. */
static mtx_t proclock;
#define PROCARR_LOCK()      mtx_spinlock(&proclock)
#define PROCARR_UNLOCK()    mtx_unlock(&proclock)
#define PROCARR_LOCK_INIT() mtx_init(&proclock, MTX_DEF | MTX_SPIN)

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RWTUN,
    &maxproc, 0, "Maximum number of processes");
SYSCTL_INT(_kern, KERN_MAXPROC, nprocs, CTLFLAG_RD,
    &nprocs, 0, "Current number of processes");

static void init_kernel_proc(void);

pid_t proc_update(void); /* Used in HAL, so not static */

/**
 * Init process handling subsystem.
 */
void proc_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(vralloc_init);

    PROCARR_LOCK_INIT();
    procarr_realloc();
    memset(_procarr, 0, SIZEOF_PROCARR());

    init_kernel_proc();

    /* Do here same as proc_update() would do when running. */
    current_process_id = 0;
    curproc = (*_procarr)[0];

    SUBSYS_INITFINI("Proc OK");
}

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
    vm_region_t * kprocvm_code = kcalloc(1, sizeof(vm_region_t));
    vm_region_t * kprocvm_heap = kcalloc(1, sizeof(vm_region_t));
    if (!(kprocvm_code && kprocvm_heap)) {
        panic(panic_msg);
    }

    kprocvm_code->mmu = mmu_region_kernel;
    kprocvm_heap->mmu = mmu_region_kdata;

    mtx_init(&(kprocvm_code->lock), MTX_DEF | MTX_SPIN);
    mtx_init(&(kprocvm_heap->lock), MTX_DEF | MTX_SPIN);

    (*kernel_proc->mm.regions)[MM_CODE_REGION] = kprocvm_code;
    (*kernel_proc->mm.regions)[MM_STACK_REGION] = 0;
    (*kernel_proc->mm.regions)[MM_HEAP_REGION] = kprocvm_heap;

    /* Break values */
    kernel_proc->brk_start = __bss_break;
    kernel_proc->brk_stop = kprocvm_heap->mmu.vaddr
        + mmu_sizeof_region(&(kprocvm_heap->mmu)) - 1;

    /* File descriptors */
    kernel_proc->files = kmalloc(SIZEOF_FILES(3));
    if (!kernel_proc->files) {
        panic(panic_msg);
    }
    kernel_proc->files->count = 3;

    /* TODO Do this correctly */
    kernel_proc->files->fd[0] = 0;
    kernel_proc->files->fd[1] = 0;
    kernel_proc->files->fd[2] = kcalloc(1, sizeof(file_t)); /* stderr */
    kernel_proc->files->fd[2]->vnode = &kerror_vnode;
}

/**
 * Realloc procarr.
 * Realloc _procarr based on maxproc sysctl variable if necessary.
 * @note    This should be generally called before selecting next pid
 *          from the array.
 */
void procarr_realloc(void)
{
    proc_info_t * (*tmp)[];

    /* Skip if size is not changed */
    if (maxproc == _cur_maxproc)
        return;

    PROCARR_LOCK();
    tmp = krealloc(_procarr, SIZEOF_PROCARR());
    if ((tmp == 0) && (_procarr == 0)) {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "Unable to allocate _procarr (%u bytes)",
                SIZEOF_PROCARR());
        panic(buf);
    }
    _procarr = tmp;
    _cur_maxproc = maxproc;
    PROCARR_UNLOCK();
}

/**
 * Insert a new process to _procarr.
 * @param proc is a pointer to the new process.
 */
void procarr_insert(proc_info_t * new_proc)
{
    PROCARR_LOCK();
    /* TODO Bound check */
    (*_procarr)[new_proc->pid] = new_proc;
    nprocs++;
    PROCARR_UNLOCK();
}

/**
 * Get a random PID for a new process.
 * @return Returns a random PID.
 */
pid_t proc_get_random_pid(void)
{
    pid_t newpid;
    pid_t last_maxproc;

    PROCARR_LOCK();
    last_maxproc = _cur_maxproc;
    PROCARR_UNLOCK();

    newpid = last_maxproc + 1;

    /* The new PID will be "randomly" selected between _lastpid and maxproc */
    do {
        if (newpid > last_maxproc)
            newpid = _lastpid + kunirand(last_maxproc - _lastpid - 1) + 1;
        newpid++;
    } while (proc_get_struct(newpid));

    _lastpid = newpid;

    return newpid;
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

int proc_kill(void)
{
    return -1;
}

/**
 * Replace the image of a given process with a new one.
 * The new image must be mapped in kernel memory space.
 * @param pid   Process id.
 * @param image Process image to be loaded.
 * @param size  Size of the image.
 * @return  Value other than zero if unable to replace process.
 */
int proc_replace(pid_t pid, void * image, size_t size)
{
    return -1;
}

/**
 * Get pointer to a internal proc_info structure.
 */
proc_info_t * proc_get_struct(pid_t pid)
{
    /* TODO do state check properly */
    if (pid > _cur_maxproc) {
#if configDEBUG != 0
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "Invalid PID : %u\ncurrpid : %u\nmaxproc : %i\nstate %x",
                pid, current_process_id, _cur_maxproc, (*_procarr)[pid]->state);
        KERROR(KERROR_DEBUG, buf);
#endif
        return 0;
    }
    return (*_procarr)[pid];
}

/**
 * Remove thread from a process.
 * @param pid       is a process id of the thread owner process.
 * @param thread_id is a thread if of the removed thread.
 */
void proc_thread_removed(pid_t pid, pthread_t thread_id)
{
    /* TODO
     * + Free stack area
     * + Remove thread pointer
     */
}

/**
 * A process enters kernel mode.
 * This function shall be called when a process enters kernel mode and
 * interrupts will be enabled.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_enter_kernel(void)
{
#if configDEBUG != 0
    if (!curproc)
        panic("No current process set");
#endif
    curproc->mm.curr_mpt = &mmu_pagetable_master;
    return curproc->mm.curr_mpt;
}

/**
 * A process exits kernel mode.
 * This function shall be called when a process exits kernel mode.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_exit_kernel(void)
{
#if configDEBUG != 0
    if (!curproc)
        panic("No current proces set");
#endif
    curproc->mm.curr_mpt = &curproc->mm.mpt;
    return curproc->mm.curr_mpt;
}

/**
 * Suspend process.
 */
void proc_suspend(void)
{
#if configDEBUG != 0
    if (!curproc)
        panic("No current proces set");
#endif
    /* TODO set state */
    //curproc->state
}

/**
 * Resume process.
 * A process is begin resumed from suspended state.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_resume(void)
{
#if configDEBUG != 0
    if (!curproc)
        panic("No current proces set");
#endif
    /* TODO set state */
    return curproc->mm.curr_mpt;
}

/**
 * Handle page fault caused by a process.
 * Usually this handler is executed because of cow page table.
 */
int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        threadInfo_t * thread)
{
    const pid_t pid = thread->pid_owner;
    const uintptr_t vaddr = far;
    proc_info_t * pcb;
    vm_region_t * region;
    vm_region_t * new_region;

#if configDEBUG >= KERROR_DEBUG
    char buf[80];
    ksprintf(buf, sizeof(buf), "proc_dab_handler(): MOO, %x", vaddr);
    KERROR(KERROR_DEBUG, buf);
#endif

    pcb = proc_get_struct(pid);
    if (!pcb || (pcb->state == PROC_STATE_INITIAL)) {
        return PROC_DABERR_NOPROC; /* Process doesn't exist. */
    }

    for (int i = 0; i < pcb->mm.nr_regions; i++) {
        region = ((*pcb->mm.regions)[i]);
        ksprintf(buf, sizeof(buf), "reg_vaddr %x, reg_end %x", region->mmu.vaddr, region->mmu.vaddr + MMU_SIZEOF_REGION(&(region->mmu)));
        KERROR(KERROR_DEBUG, buf);

        if (vaddr >= region->mmu.vaddr &&
                vaddr <= (region->mmu.vaddr + MMU_SIZEOF_REGION(&(region->mmu)))) {
            /* Test for COW flag. */
            if ((region->usr_rw & VM_PROT_COW) != VM_PROT_COW) {
                return PROC_DABERR_PROT; /* Memory protection error. */
            }

            if(!(region->vm_ops->rclone)
                    || !(new_region = region->vm_ops->rclone(region))) {
                /* Can't clone region; COW clone failed. */
                return PROC_DABERR_ENOMEM;
            }

            /* Free the old region as this process no longer uses it.
             * (Usually decrements some internal refcount) */
            if (region->vm_ops->rfree)
                region->vm_ops->rfree(region);

            new_region->usr_rw &= ~VM_PROT_COW; /* No more COW. */
            (*pcb->mm.regions)[i] = new_region; /* Replace old region. */
            mmu_map_region(&(new_region->mmu)); /* Map it to the page table. */

            return 0; /* COW done. */
        }
    }

    return PROC_DABERR_INVALID; /* Not found */
}

/**
 * Update process system state.
 * Updates current_process_id and curproc.
 * @note This function is called by interrupt handler(s).
 */
pid_t proc_update(void)
{
    current_process_id = current_thread->pid_owner;
    curproc = proc_get_struct(current_process_id);

    return current_process_id;
}

uintptr_t proc_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_PROC_EXEC: /* note: can only return EAGAIN or ENOMEM */
        set_errno(ENOSYS);
        return -1;

    case SYSCALL_PROC_FORK:
    {
        pid_t pid = proc_fork(current_process_id);
        if (pid < 0)
            return -1;
        else
            return pid;
    }

    case SYSCALL_PROC_WAIT:
        set_errno(ENOSYS);
        return -3;

    case SYSCALL_PROC_EXIT:
        set_errno(ENOSYS);
        return -4;

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

/**
  * @}
  */
