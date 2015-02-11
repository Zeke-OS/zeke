/**
 *******************************************************************************
 * @file    proc.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
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

/**
 * proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
mtx_t proclock;

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RW,
    &maxproc, 0, "Maximum number of processes");
SYSCTL_INT(_kern, OID_AUTO, nprocs, CTLFLAG_RD,
    &nprocs, 0, "Current number of processes");

static void init_kernel_proc(void);
static void procarr_remove(pid_t pid);
static void proc_remove(struct proc_info * proc);
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

static void init_rlims(struct rlimit (*rlim)[_RLIMIT_ARR_COUNT])
{
    /*
     * These are only initialized once and inherited for childred even
     * over execs.
     */
    (*rlim)[RLIMIT_CORE] =      (struct rlimit){ configRLIMIT_CORE,
                                                 configRLIMIT_CORE };
    (*rlim)[RLIMIT_CPU] =       (struct rlimit){ configRLIMIT_CPU,
                                                 configRLIMIT_CPU };
    (*rlim)[RLIMIT_DATA] =      (struct rlimit){ configRLIMIT_DATA,
                                                 configRLIMIT_DATA };
    (*rlim)[RLIMIT_FSIZE] =     (struct rlimit){ configRLIMIT_FSIZE,
                                                 configRLIMIT_FSIZE };
    (*rlim)[RLIMIT_NOFILE] =    (struct rlimit){ configRLIMIT_NOFILE,
                                                 configRLIMIT_NOFILE };
    (*rlim)[RLIMIT_STACK] =     (struct rlimit){ configRLIMIT_STACK,
                                                 configRLIMIT_STACK };
    (*rlim)[RLIMIT_AS] =        (struct rlimit){ configRLIMIT_AS,
                                                 configRLIMIT_AS };
}

/**
 * Initialize kernel process 0.
 */
static void init_kernel_proc(void)
{
    const char panic_msg[] = "Can't init kernel process";
    proc_info_t * kernel_proc;
    int err;

    (*_procarr)[0] = kcalloc(1, sizeof(proc_info_t));
    kernel_proc = (*_procarr)[0];

    kernel_proc->pid = 0;
    kernel_proc->state = PROC_STATE_READY;
    strcpy(kernel_proc->name, "kernel");

    RB_INIT(&(kernel_proc->mm.ptlist_head));

    /* Copy master page table descriptor */
    memcpy(&(kernel_proc->mm.mpt), &mmu_pagetable_master,
            sizeof(mmu_pagetable_t));

    /*
     * Create regions
     */
    err = realloc_mm_regions(&kernel_proc->mm, 3);
    if (err) {
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

    mtx_lock(&kernel_proc->mm.regions_lock);
    (*kernel_proc->mm.regions)[MM_CODE_REGION] = kprocvm_code;
    (*kernel_proc->mm.regions)[MM_STACK_REGION] = NULL;
    (*kernel_proc->mm.regions)[MM_HEAP_REGION] = kprocvm_heap;
    mtx_unlock(&kernel_proc->mm.regions_lock);

    /*
     * Break values
     */
    kernel_proc->brk_start = &__bss_break;
    kernel_proc->brk_stop = (void *)(kprocvm_heap->b_mmu.vaddr
        + mmu_sizeof_region(&(kprocvm_heap->b_mmu)) - 1);

    /* Call constructor for signals struct */
    ksignal_signals_ctor(&kernel_proc->sigs);

    /*
     * File descriptors
     *
     * We have a hard limit of 8 files here now but this is actually tunable
     * for child processes by using setrlimit().
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
#if configKLOGGER != 0
    if (fs_fildes_set(kernel_proc->files->fd[STDERR_FILENO],
                      &kerror_vnode, O_WRONLY)) {
        panic(panic_msg);
    }
#endif

    init_rlims(&kernel_proc->rlim);

    mtx_init(&kernel_proc->inh.lock, PROC_INH_LOCK_TYPE);
}

void procarr_realloc(void)
{
    proc_info_t * (*tmp)[];

    /* Skip if size is not changed */
    if (maxproc == act_maxproc)
        return;

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "realloc procarr maxproc = %u, act_maxproc = %u\n",
             maxproc, act_maxproc);
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
    KASSERT(new_proc, "new_proc can't be NULL");

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "procarr_insert(%d)\n", new_proc->pid);
#endif

    PROC_LOCK();
    if (new_proc->pid > act_maxproc || new_proc->pid < 0) {
        KERROR(KERROR_ERR, "Inserted new_proc out of bounds");
        return;
    }

    (*_procarr)[new_proc->pid] = new_proc;
    nprocs++;
    PROC_UNLOCK();
}

static void procarr_remove(pid_t pid)
{
    PROC_LOCK();
    if (pid > act_maxproc || pid < 0) {
        KERROR(KERROR_ERR, "Attempt to remove a nonexistent process");
        return;
    }

    (*_procarr)[pid] = NULL;
    nprocs--;
    PROC_UNLOCK();
}

/**
 * Remove zombie process from the system.
 */
static void proc_remove(struct proc_info * proc)
{
    struct proc_info * parent;

    KASSERT(proc, "Attempt to remove NULL proc");

    proc->state = PROC_STATE_DEFUNCT;

#ifdef configPROCFS
    procfs_rmentry(proc->pid);
#endif

    /*
     * Remove inheritance.
     */
    parent = proc->inh.parent;
    if (parent) {
        struct proc_info * tmp;
        struct proc_info * prev_node = NULL;

        mtx_lock(&parent->inh.lock);

        tmp = parent->inh.first_child;
        do {
            if (tmp == proc) {
                if (!prev_node)
                    parent->inh.first_child = proc->inh.next_child;
                else
                    prev_node->inh.next_child = proc->inh.next_child;
                break;
            }
            prev_node = tmp;
        } while ((tmp = prev_node->inh.next_child) != NULL);

        mtx_unlock(&parent->inh.lock);
    }

    _proc_free(proc);
    procarr_remove(proc->pid);
}

void _proc_free(proc_info_t * p)
{
    if (!p) {
        KERROR(KERROR_WARN, "Got NULL as a proc_info struct, double free?\n");

        return;
    }

    /* Free files */
    if (p->files) {
        for (int i = 0; i < p->files->count; i++) {
            fs_fildes_ref(p->files, i, -1); /* null pointer safe */
        }
        kfree(p->files);
    }

    /* Free regions */
    mtx_lock(&p->mm.regions_lock);
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

        /* RFE should not unlock regions? */
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
        /* Following may cause nasty things if pid is out of bounds */
        KERROR(KERROR_ERR, "Invalid PID (%d > %d)\n", pid, act_maxproc);

        return NULL;
    }
    return (*_procarr)[pid];
}

struct thread_info * proc_iterate_threads(proc_info_t * proc,
        struct thread_info ** thread_it)
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

    /* Go zombie if removed thread was main() */
    if (p->main_thread && (p->main_thread->id == thread_id)) {
        p->main_thread = NULL;
        p->state = PROC_STATE_ZOMBIE;
    }
}

void proc_update_times(void)
{
    if (current_thread->flags & SCHED_INSYS_FLAG)
        curproc->tms.tms_stime++;
    else
        curproc->tms.tms_utime++;
}

int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread)
{
    const pid_t pid = thread->pid_owner;
    const uintptr_t vaddr = far;
    proc_info_t * pcb;
    struct vm_mm_struct * mm;
    struct buf * region;
    struct buf * new_region;

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "proc_dab_handler(): MOO, %x @ %x\n", vaddr, lr);
#endif

    pcb = proc_get_struct_l(pid);
    if (!pcb || (pcb->state == PROC_STATE_INITIAL)) {
        return -ESRCH; /* Process doesn't exist. */
    }
    mm = &pcb->mm;

    mtx_lock(&mm->regions_lock);
    for (size_t i = 0; i < mm->nr_regions; i++) {
        region = (*mm->regions)[i];
        uintptr_t reg_start, reg_end;
        if (!region)
            continue;

        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + MMU_SIZEOF_REGION(&region->b_mmu);

#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "reg_vaddr %x, reg_end %x\n",
                reg_start, reg_end);
#endif

        if (VM_ADDR_IS_IN_RANGE(vaddr, reg_start, reg_end)) {
            int err;

            /* Test for COW flag. */
            if ((region->b_uflags & VM_PROT_COW) != VM_PROT_COW) {
                mtx_unlock(&mm->regions_lock);
                return -EACCES; /* Memory protection error. */
            }

            if (!(region->vm_ops->rclone)
                    || !(new_region = region->vm_ops->rclone(region))) {
                /* Can't clone region; COW clone failed. */
                mtx_unlock(&mm->regions_lock);
                return -ENOMEM;
            }

            new_region->b_uflags &= ~VM_PROT_COW; /* No more COW. */
            mtx_unlock(&mm->regions_lock);
            err = vm_replace_region(pcb, new_region, i,
                                    VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
            if (err)
                return err;

            return 0; /* COW done. */
        }
    }
    mtx_unlock(&mm->regions_lock);

    return -EFAULT; /* Not found */
}

pid_t proc_update(void)
{
    current_process_id = current_thread->pid_owner;
    curproc = proc_get_struct_l(current_process_id);

    KASSERT(curproc, "curproc should be valid");

    return current_process_id;
}

/* Syscall handlers ***********************************************************/

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
    struct _proc_wait_args args;
    pid_t pid_child;
    proc_info_t * child = NULL;
    int * state;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE) ||
            copyin(user_args, &args, sizeof(args))) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.pid == 0) {
        /*
         * TODO
         * If pid is 0, status is requested for any child process whose
         * process group ID is equal to that of the calling process.
         */
        set_errno(ENOTSUP);
        return -1;
    } else if (args.pid == -1) {
        child = curproc->inh.first_child;
    } else if (args.pid < -1) {
        /*
         * TODO
         * If pid is less than (pid_t)-1, status is requested for any child
         * process whose process group ID is equal to the absolute value of pid.
         */
        set_errno(ENOTSUP);
        return -1;
    } else if (args.pid > 0) {
        proc_info_t * p;
        proc_info_t * last_node;
        proc_info_t * tmp;


        p = proc_get_struct_l(args.pid);
        tmp = curproc->inh.first_child;
        if (!p || !tmp) {
            set_errno(ECHILD);
            return -1;
        }

        /*
         * Check that p is a child of curproc.
         */
        mtx_lock(&curproc->inh.lock);
        do {
            if (tmp->pid == p->pid) {
                child = p;
                break;
            }
            last_node = tmp;
        } while ((tmp = last_node->inh.next_child) != NULL);
        mtx_unlock(&curproc->inh.lock);
    }

    if (!child) {
        /*
         * The calling process has no existing unwaited-for child
         * processes.
         */
        set_errno(ECHILD);
        return -1;
    }

    /* Get the thread number we are waiting for */
    pid_child = child->pid;
    state = &child->state;

    if ((args.options & WNOHANG) && (*state != PROC_STATE_ZOMBIE)) {
        /*
         * WNOHANG = Do not suspend execution of the calling thread if status
         * is not immediately available.
         */
        return 0;
    }

    /* TODO Implement options WCONTINUED and WUNTRACED. */

    while (*state != PROC_STATE_ZOMBIE) {
#if 0
        /* This is what we'd like to do but it wont work if signal is sent
         * between (*state != PROC_STATE_ZOMBIE) and
         * ksignal_sigwait(&retval, &set) the signal will be ignored and
         * we'll stay in thread_wait() forever.
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        ksignal_sigwait(&retval, &set);
        /* Thus we are doing that :( \/ */
#endif
        idle_sleep();

        /*
         * TODO In some cases we have to return early without waiting.
         * eg. signal received
         */
    }

    /*
     * Construct a status value.
     * TODO Evaluate rest of needed codes like signals etc.
     */
    args.status = (child->exit_code & 0xff) << 8;

    if (args.options & WNOWAIT) {
        /* Leave the proc around, available for later waits. */
        copyout(&args, user_args, sizeof(args));
        return (uintptr_t)pid_child;
    }

    /*
     * Increment children times.
     * We do this only for wait() and waitpid().
     */
    curproc->tms.tms_cutime += child->tms.tms_utime;
    curproc->tms.tms_cstime += child->tms.tms_stime;

    copyout(&args, user_args, sizeof(args));

    /* Remove wait'd thread */
    proc_remove(child);

    return (uintptr_t)pid_child;
}

static int sys_proc_exit(void * user_args)
{
    KASSERT(curproc->inh.parent, "parent should exist");

    curproc->exit_code = get_errno();

    (void)ksignal_sendsig(&curproc->inh.parent->sigs, SIGCHLD, SI_KERNEL);
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
    int err;

    if (!useracc(user_args, sizeof(struct _ds_getbreak), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    err = copyin(user_args, &ds, sizeof(ds));
    ds.start = curproc->brk_start;
    ds.stop = curproc->brk_stop;
    err |= copyout(&ds, user_args, sizeof(ds));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static const syscall_handler_t proc_sysfnmap[] = {
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
