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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/environ.h>
#include <tsched.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <kinit.h>
#include <syscall.h>
#include <vm/vm.h>
#include <vm/vm_copyinstruct.h>
#include <fs/procfs.h>
#include <ptmapper.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <buf.h>
#include <thread.h>
#include <exec.h>
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

int proc_init(void)
{
    SUBSYS_DEP(vralloc_init);
    SUBSYS_INIT("proc");

    PROC_LOCK_INIT();
    procarr_realloc();
    memset(_procarr, 0, SIZEOF_PROCARR());

    init_kernel_proc();

    /* Do here same as proc_update() would do when running. */
    current_process_id = 0;
    curproc = (*_procarr)[0];

    return 0;
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

    /*
     * Create regions
     */
    kernel_proc->mm.regions = kcalloc(3, sizeof(void *));
    kernel_proc->mm.nr_regions = 3;
    if (!kernel_proc->mm.regions) {
        panic(panic_msg);
    }

    /*
     * Copy region descriptors
     */
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

    /*
     * Break values
     */
    kernel_proc->brk_start = &__bss_break;
    kernel_proc->brk_stop = (void *)(kprocvm_heap->b_mmu.vaddr
        + mmu_sizeof_region(&(kprocvm_heap->b_mmu)) - 1);

    /*
     * Environ
     */
    kernel_proc->environ = geteblk(MMU_PGSIZE_COARSE);
    if (!kernel_proc->environ) {
        panic(panic_msg);
    }
    kernel_proc->environ->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
    /* TODO Remove magic value */
    if (vm_addrmap_region(kernel_proc, kernel_proc->environ, 0x10000000)) {
        panic(panic_msg);
    }
    if (proc_setenv(kernel_proc->environ,
                    (char **){ NULL }, (char **){ NULL })) {
        panic(panic_msg);
    }

    /* Call constructor for signals struct */
    ksignal_signals_ctor(&kernel_proc->sigs);

    /*
     * File descriptors
     *
     * TODO We have a hard limit of 8 files here now but this should be tunable
     * by using setrlimit() Also we may want to set this smaller at some point.
     */
    kernel_proc->files = kcalloc(1, SIZEOF_FILES(8));
    if (!kernel_proc->files) {
        panic(panic_msg);
    }
    kernel_proc->files->count = 8;
    kernel_proc->files->umask = CMASK; /* File creation mask. */

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

#ifdef configPROC_DEBUG
    char buf[80];

    ksprintf(buf, sizeof(buf), "realloc procarr maxproc = %u, act_maxproc = %u\n",
             maxproc, act_maxproc);
    KERROR(KERROR_DEBUG, buf);
#endif

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
    proc_info_t * p = proc_get_struct_l(pid);

    /* TODO free everything */
    if (!p)
        return;

#ifdef configPROCFS
    procfs_rmentry(pid);
#endif

    _proc_free(p);
    procarr_remove(pid);
}

void _proc_free(proc_info_t * p)
{
    if (!p) {
        KERROR(KERROR_WARN, "Got NULL as a proc_info struct, double free?\n");

        return;
    }

    /* Free environ (argv and env) */
    if (p->environ) {
        p->environ->vm_ops->rfree(p->environ);
    }

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

proc_info_t * proc_get_struct_l(pid_t pid)
{
    istate_t s;
    proc_info_t * retval;

    s = get_interrupt_state();
    if (!(s & PSR_INT_I))
        PROC_LOCK();
    retval = proc_get_struct(pid);
    if (!(s & PSR_INT_I))
        PROC_UNLOCK();

    return retval;
}

proc_info_t * proc_get_struct(pid_t pid)
{
    istate_t s;

    s = get_interrupt_state();
    if (!(s & PSR_INT_I)) {
        KASSERT(PROC_TESTLOCK(),
                "proclock is required before entering proc_get_struct()\n");
    }

    /* TODO do state check properly */
    if (pid > act_maxproc) {
        char buf[80];

        /* Following may cause nasty things if pid is out of bounds */
        ksprintf(buf, sizeof(buf), "Invalid PID (%d > %d)\n", pid, act_maxproc);
        KERROR(KERROR_ERR, buf);

        return NULL;
    }
    return (*_procarr)[pid];
}

threadInfo_t * proc_iterate_threads(proc_info_t * proc,
        threadInfo_t ** thread_it)
{
    if (*thread_it == NULL) {
        *thread_it = proc->main_thread;
    } else {
        if (*thread_it == proc->main_thread) {
            *thread_it = (*thread_it)->inh.first_child;
        } else {
            *thread_it = (*thread_it)->inh.next_child;
        }
    }

    return *thread_it;
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
#ifdef configPROC_DEBUG
    if (!curproc)
        panic("No current process set");
#endif
    curproc->mm.curr_mpt = &mmu_pagetable_master;
}

mmu_pagetable_t * proc_exit_kernel(void)
{
    KASSERT(curproc != NULL, "Current proces should be set");

    curproc->mm.curr_mpt = &curproc->mm.mpt;

    return curproc->mm.curr_mpt;
}

void proc_suspend(void)
{
    KASSERT(curproc != NULL, "Current proces should be set");
    /* TODO set state */
    //curproc->state
}

mmu_pagetable_t * proc_resume(void)
{
    KASSERT(curproc != NULL, "Current proces should be set");
    /* TODO set state */

    return curproc->mm.curr_mpt;
}

void proc_update_times(void)
{
    if (current_thread->flags & SCHED_INSYS_FLAG)
        curproc->tms.tms_stime++;
    else
        curproc->tms.tms_utime++;
}

int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        threadInfo_t * thread)
{
    const pid_t pid = thread->pid_owner;
    const uintptr_t vaddr = far;
    proc_info_t * pcb;
    struct buf * region;
    struct buf * new_region;
#ifdef configPROC_DEBUG
    char buf[80];

    ksprintf(buf, sizeof(buf), "proc_dab_handler(): MOO, %x @ %x\n", vaddr, lr);
    KERROR(KERROR_DEBUG, buf);
#endif

    pcb = proc_get_struct_l(pid);
    if (!pcb || (pcb->state == PROC_STATE_INITIAL)) {
        return -ESRCH; /* Process doesn't exist. */
    }

    for (int i = 0; i < pcb->mm.nr_regions; i++) {
        region = ((*pcb->mm.regions)[i]);
#ifdef configPROC_DEBUG
        ksprintf(buf, sizeof(buf), "reg_vaddr %x, reg_end %x\n",
                region->b_mmu.vaddr,
                region->b_mmu.vaddr + MMU_SIZEOF_REGION(&(region->b_mmu)));
        KERROR(KERROR_DEBUG, buf);
#endif

        if (vaddr >= region->b_mmu.vaddr &&
                vaddr <= (region->b_mmu.vaddr + MMU_SIZEOF_REGION(&(region->b_mmu)))) {
            /* Test for COW flag. */
            if ((region->b_uflags & VM_PROT_COW) != VM_PROT_COW) {
                return -EACCES; /* Memory protection error. */
            }

            if (!(region->vm_ops->rclone)
                    || !(new_region = region->vm_ops->rclone(region))) {
                /* Can't clone region; COW clone failed. */
                return -ENOMEM;
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

    return -EFAULT; /* Not found */
}

pid_t proc_update(void)
{
    current_process_id = current_thread->pid_owner;
    curproc = proc_get_struct_l(current_process_id);

    KASSERT(curproc, "curproc should be valid");

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
        .del_thread = 0 /* TODO should be libc: pthread_exit */
    };

    const pthread_t tid = thread_create(&ds, 0);
    if (tid <= 0) {
        panic("Exec failed");
    }

    /* interrupts will be enabled automatically */
    thread_die(0);

    return 0; /* Never returns */
}

static size_t copyvars(char ** dp, char ** vp, size_t left)
{
    char * data = *dp;
    size_t i, len;

    if (!vp)
        return left;

    i = 0;
    while (vp[i]) {
        len = strlcpy(data, vp[i], left);
        left -= len + 1;
        data = data + len;

        i++;
        if (left == 0 || vp[i] == NULL)
            break;
    }

    *dp = data;
    return left;
}

int proc_setenv(struct buf * environ_bp, char * argv[], char * env[])
{
    char * data = (char *)environ_bp->b_data;
    size_t left = ARG_MAX;

    /*
     * First arguments and then environmental variables.
     */

    left = copyvars(&data, argv, left);

    if (left == 0)
        return -ENOMEM;

    *data = ENVIRON_FS;
    left = copyvars(&data, env, left);

    return 0;
}

/**
 * Copyin argv or argc array from user space to a kernel buffer pointed by kdata.
 * @param kdata is a character buffer of at least left bytes in size.
 * @param uaddr is a user space pointer to a argv or env array.
 * @param n is the count of elements in uaddr array.
 * @param left is the number of bytes left in kdata.
 */
static int copyin_envvars(char ** kdata, char ** uaddr, const size_t n,
        size_t * left)
{
    char ** udata;
    const size_t udata_size = n * sizeof(char *);
    size_t i;
    int err, retval = 0;

    if (n == 0)
        return 0;

    udata = kmalloc(udata_size);
    if (!udata)
        return -ENOMEM;

    err = copyin(uaddr, udata, udata_size);
    if (err) {
        retval = err;
        goto out;
    }

    for (i = 0; i < n; i++) {
        size_t copied;

        if (!udata[i])
            break;

        err = copyinstr(udata[i], *kdata, *left, &copied);
        if (err) {
            retval = err;
            goto out;
        }

        *left -= copied;
        *kdata += copied;
    }

out:
    kfree(udata);
    return retval;
}

int proc_copyinenv(struct buf * environ_bp, char * uargv[], size_t nargv,
        char * uenv[], size_t nenv)
{
    char * data = (char *)environ_bp->b_data;
    size_t left = ARG_MAX;
    int err;

    err = copyin_envvars(&data, uargv, nargv, &left);
    if (err)
        return err;
    if (left == 0)
        return -ENOMEM;

    *data = ENVIRON_FS;

    left = copyin_envvars(&data, uenv, nenv, &left);

    return 0;
}

/* Syscall handlers ***********************************************************/

static int sys_proc_exec(void * user_args)
{
    struct _proc_exec_args args;
    file_t * file;
    struct buf * new_environ;
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

    new_environ = curproc->environ->vm_ops->rclone(curproc->environ);
    if (!new_environ) {
        set_errno(ENOMEM);
        retval = -1;
        goto out;
    }

    proc_copyinenv(new_environ, args.argv, args.nargv, args.env, args.nenv);

    exec_file(file, new_environ);

    retval = 0;
out:
    /* Decrement refcount for the file pointed by fd */
    fs_fildes_ref(curproc->files, args.fd, -1);
    return retval;
}

static int sys_proc_fork(void * user_args)
{
    pid_t pid = proc_fork(current_process_id);
    if (pid < 0) {
        set_errno(-pid);
        return -1;
    } else {
        return pid;
    }
}

static int sys_proc_wait(void * user_args)
{
    pid_t pid_child;
    proc_info_t * child;

    if (!useracc(user_args, sizeof(pid_t), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    child = curproc->inh.first_child;
    if (!child) {
        /* The calling process has no existing unwaited-for child
         * processes. */
        set_errno(ECHILD);
        return -1;
    }

    /* Get the thread number we are waiting for */
    pid_child = curproc->inh.first_child->pid;

    /* curproc->state = PROC_STATE_WAITING needs some other flag? */
    while (child->state != PROC_STATE_ZOMBIE) {
        idle_sleep();
        /* TODO In some cases we have to return early without waiting.
         * eg. signal received */
    }

    /* Increment children times.
     * We do this only for wait() and waitpid().
     */
    curproc->tms.tms_cutime += child->tms.tms_utime;
    curproc->tms.tms_cstime += child->tms.tms_stime;

    copyout(&child->exit_code, user_args, sizeof(int));

    /* Remove wait'd thread */
    proc_remove(pid_child);

    return (uintptr_t)pid_child;
}

static int sys_proc_exit(void * user_args)
{
    curproc->exit_code = get_errno();
    sched_thread_detach(current_thread->id);
    thread_die(curproc->exit_code);

    return 0; /* Never reached */
}

/**
 * Set and get process credentials (user and group ids).
 * This function can serve following POSIX functions:
 * - getuid(), geteuid()
 * - setuid(), seteuid(), setreuid()
 * - getegid(), getgid()
 * - setgid(), setegid(), setregid()
 */
static int sys_proc_getsetcred(void * user_args)
{
    struct _ds_proc_credctl cred;
    uid_t ruid = curproc->uid;
    /*uid_t euid = curproc->euid;*/
    uid_t suid = curproc->suid;
    gid_t rgid = curproc->gid;
    gid_t sgid = curproc->sgid;
    int retval = 0;

    if (!useracc(user_args, sizeof(cred), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &cred, sizeof(cred));

    /* Set uid */
    if (cred.ruid >= 0) {
        if (priv_check(curproc, PRIV_CRED_SETUID) == 0)
            curproc->uid = cred.ruid;
        else
            retval = -1;
    }

    /* Set euid */
    if (cred.euid >= 0) {
        uid_t new_euid = cred.euid;

        if ((priv_check(curproc, PRIV_CRED_SETEUID) == 0) ||
                new_euid == ruid || new_euid == suid)
            curproc->euid = new_euid;
        else
            retval = -1;
    }

    /* Set suid */
    if (cred.suid >= 0) {
        if (priv_check(curproc, PRIV_CRED_SETSUID) == 0)
            curproc->suid = cred.suid;
        else
            retval = -1;
    }

    /* Set gid */
    if (cred.rgid >= 0) {
        if (priv_check(curproc, PRIV_CRED_SETGID) == 0)
            curproc->gid = cred.rgid;
        else
            retval = -1;
    }

    /* Set egid */
    if (cred.egid >= 0) {
        gid_t new_egid = cred.egid;

        if ((priv_check(curproc, PRIV_CRED_SETEGID) == 0) ||
                new_egid == rgid || new_egid == sgid)
            curproc->egid = cred.egid;
        else
            retval = -1;
    }

    /* Set sgid */
    if (cred.sgid >= 0) {
        if (priv_check(curproc, PRIV_CRED_SETSGID) == 0)
            curproc->sgid = cred.sgid;
        else
            retval = -1;
    }

    if (retval)
        set_errno(EPERM);

    /* Get current values */
    cred.ruid = curproc->uid;
    cred.euid = curproc->euid;
    cred.suid = curproc->suid;
    cred.rgid = curproc->gid;
    cred.egid = curproc->egid;
    cred.sgid = curproc->sgid;

    copyout(&cred, user_args, sizeof(cred));

    return retval;
}

static int sys_proc_getpid(void * user_args)
{
    if (copyout(&curproc->pid, user_args, sizeof(pid_t))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getppid(void * user_args)
{
    pid_t parent;

    if (!curproc->inh.parent)
        parent = 0;
    else
        parent = curproc->inh.parent->pid;

    if (copyout(&parent, user_args, sizeof(pid_t))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_alarm(void * user_args)
{
    set_errno(ENOSYS);
    return -1;
}

static int sys_proc_chdir(void * user_args)
{
    struct _proc_chdir_args * args = 0;
    vnode_t * vn;
    int err, retval = -1;

    err = copyinstruct(user_args, (void **)(&args),
            sizeof(struct _proc_chdir_args),
            GET_STRUCT_OFFSETS(struct _proc_chdir_args,
                name, name_len));
    if (err) {
        set_errno(EFAULT);
        goto out;
    }

    /* Validate name string */
    if (!strvalid(args->name, args->name_len)) {
        set_errno(ENAMETOOLONG);
        goto out;
    }

    err = fs_namei_proc(&vn, args->fd, args->name, args->atflags &
            (AT_FDCWD | AT_FDARG | AT_SYMLINK_NOFOLLOW | AT_SYMLINK_FOLLOW));
    if (err) {
        set_errno(-err);
        goto out;
    }

    if (!S_ISDIR(vn->vn_mode)) {
        vrele(vn);
        set_errno(ENOTDIR);
        goto out;
    }

    /* Change cwd */
    vrele(curproc->cwd);
    curproc->cwd = vn;
    /* Leave vnode refcount +1 */

    retval = 0;
out:
    freecpystruct(args);
    return retval;
}

static int sys_proc_setpriority(void * user_args)
{
    set_errno(ENOSYS);
    return -1;
}

static int sys_proc_getpriority(void * user_args)
{
    set_errno(ENOSYS);
    return -1;
}

/* TODO Implementation */
static int sys_proc_times(void * user_args)
{
    struct tms tms;

    if (copyout(&curproc->tms, user_args, sizeof(struct tms))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getbreak(void * user_args)
{
    struct _ds_getbreak ds;

    if (!useracc(user_args, sizeof(struct _ds_getbreak), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    copyin(user_args, &ds, sizeof(ds));
    ds.start = curproc->brk_start;
    ds.stop = curproc->brk_stop;
    copyout(&ds, user_args, sizeof(ds));

    return 0;
}

static const syscall_handler_t proc_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_EXEC, sys_proc_exec),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_FORK, sys_proc_fork),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_WAIT, sys_proc_wait),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_EXIT, sys_proc_exit),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_CRED, sys_proc_getsetcred),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPID, sys_proc_getpid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPPID, sys_proc_getppid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_ALARM, sys_proc_alarm),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_CHDIR, sys_proc_chdir),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETPRIORITY, sys_proc_setpriority),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPRIORITY, sys_proc_getpriority),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_TIMES, sys_proc_times),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETBREAK, sys_proc_getbreak)
};
SYSCALL_HANDLERDEF(proc_syscall, proc_sysfnmap)
