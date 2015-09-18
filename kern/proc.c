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

#define PROC_INTERNAL

#include <errno.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <buf.h>
#include <dynmem.h>
#include <exec.h>
#include <fs/procfs.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <ksched.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <ptmapper.h>
#include <syscall.h>
#include <vm/vm_copyinstruct.h>

static struct proc_info *(*_procarr)[]; /*!< processes indexed by pid */
int maxproc = configMAXPROC;            /*!< Maximum # of processes, set. */
int act_maxproc;                        /*!< Effective maxproc. */
int nprocs = 1;                         /*!< Current # of procs. */
struct proc_info * curproc;             /*!< PCB of the current process. */

static const struct vm_ops sys_vm_ops; /* NOOP struct for system regions. */

extern vnode_t kerror_vnode;
extern void * __bss_break __attribute__((weak));

#define SIZEOF_PROCARR()    ((maxproc + 1) * sizeof(struct proc_info *))

/**
 * proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
mtx_t proclock;

static const char * const proc_state_names[] = {
    "PROC_STATE_INITIAL",
    "PROC_STATE_RUNNING", /* not used atm */
    "PROC_STATE_READY",
    "PROC_STATE_WAITING", /* not used atm */
    "PROC_STATE_STOPPED",
    "PROC_STATE_ZOMBIE",
    "PROC_STATE_DEFUNCT",
};

SYSCTL_INT(_kern, OID_AUTO, nprocs, CTLFLAG_RD,
    &nprocs, 0, "Current number of processes");

static void init_kernel_proc(void);
static void procarr_remove(pid_t pid);
static void proc_remove(struct proc_info * proc);
pid_t proc_update(void); /* Used in HAL, so not static but not in headeaders. */

int __kinit__ proc_init(void)
{
    SUBSYS_DEP(vralloc_init);
    SUBSYS_INIT("proc");

    int err;

    PROC_LOCK_INIT();
    err = procarr_realloc();
    if (err) /* This is a critical failure so we just panic. */
        panic("proc initialization failed");
    memset(_procarr, 0, SIZEOF_PROCARR());

    init_kernel_proc();

    /* Do here same as proc_update() would do when running. */
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
    struct proc_info * kernel_proc;
    struct session * ses;
    int err;

    (*_procarr)[0] = kzalloc(sizeof(struct proc_info));
    kernel_proc = (*_procarr)[0];

    kernel_proc->pid = 0;
    kernel_proc->state = PROC_STATE_READY;
    strcpy(kernel_proc->name, "kernel");

    /*
     * Initialize a session.
     */
    ses = proc_session_create(kernel_proc, "root");
    PROC_LOCK();
    proc_pgrp_create(ses, kernel_proc);
    PROC_UNLOCK();
    if (!kernel_proc->pgrp) {
        panic(panic_msg);
    }

    priv_cred_init(&kernel_proc->cred);

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
    struct buf * kprocvm_code = kzalloc(sizeof(struct buf));
    struct buf * kprocvm_heap = kzalloc(sizeof(struct buf));
    if (!(kprocvm_code && kprocvm_heap)) {
        panic(panic_msg);
    }

    kprocvm_code->b_mmu = mmu_region_kernel;
    kprocvm_code->b_bufsize = mmu_sizeof_region(&mmu_region_kernel);
    kprocvm_heap->b_mmu = mmu_region_kdata;
    /*
     * FIXME For some unkown reason the kernel fails
     *       if kprocvm_heap->b_bufsize != 0
     */
#if 0
    kprocvm_heap->b_bufsize = mmu_sizeof_region(&mmu_region_kdata);
#endif

    kprocvm_code->vm_ops = &sys_vm_ops;
    kprocvm_heap->vm_ops = &sys_vm_ops;

    mtx_init(&(kprocvm_code->lock), MTX_TYPE_SPIN, 0);
    mtx_init(&(kprocvm_heap->lock), MTX_TYPE_SPIN, 0);

    mtx_lock(&kernel_proc->mm.regions_lock);
    (*kernel_proc->mm.regions)[MM_CODE_REGION] = kprocvm_code;
    /*
     * proc 0 stack shouldn't be set here because NULL for
     * MM_STACK_REGION is a special case for intialization because
     * proc 1 is really not forked from the kernel but rather just
     * spawned and constructed by hand in kinit.
     */
    (*kernel_proc->mm.regions)[MM_STACK_REGION] = NULL;
    (*kernel_proc->mm.regions)[MM_HEAP_REGION] = kprocvm_heap;
    mtx_unlock(&kernel_proc->mm.regions_lock);

    /*
     * Break values.
     */
    kernel_proc->brk_start = &__bss_break;
    kernel_proc->brk_stop = (void *)(kprocvm_heap->b_mmu.vaddr
        + mmu_sizeof_region(&(kprocvm_heap->b_mmu)) - 1);

    /* Call constructor for signals struct */
    ksignal_signals_ctor(&kernel_proc->sigs, SIGNALS_OWNER_PROCESS);

    /*
     * File descriptors.
     *
     * We have a hard limit of 8 files here now but this is actually tunable
     * for child processes by using setrlimit().
     */
    kernel_proc->files = kzalloc(SIZEOF_FILES(8));
    if (!kernel_proc->files) {
        panic(panic_msg);
    }
    kernel_proc->files->count = 8;
    kernel_proc->files->umask = CMASK; /* File creation mask. */

    /* RFE Is there still something wrong? */
    kernel_proc->files->fd[STDIN_FILENO] = 0;
    /* stderr */
#ifdef configKLOGGER
    kernel_proc->files->fd[STDERR_FILENO] = kzalloc(sizeof(file_t));
    if (fs_fildes_set(kernel_proc->files->fd[STDERR_FILENO],
                      &kerror_vnode, O_WRONLY)) {
        panic(panic_msg);
    }
    kernel_proc->files->fd[STDOUT_FILENO] = fs_fildes_ref(kernel_proc->files,
                                                          STDERR_FILENO, 1);
#else
    kernel_proc->files->fd[STDOUT_FILENO] = 0;
    kernel_proc->files->fd[STDERR_FILENO] = 0;
#endif

    init_rlims(&kernel_proc->rlim);

    mtx_init(&kernel_proc->inh.lock, PROC_INH_LOCK_TYPE, 0);
}

int procarr_realloc(void)
{
    struct proc_info * (*tmp)[];

    /* Skip if size is not changed */
    if (maxproc == act_maxproc)
        return 0;

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "realloc procarr maxproc = %u, act_maxproc = %u\n",
             maxproc, act_maxproc);
#endif

    PROC_LOCK();
    tmp = krealloc(_procarr, SIZEOF_PROCARR());
    if ((tmp == 0) && (_procarr == 0)) {
        KERROR(KERROR_WARN, "Unable to allocate _procarr (%u bytes)",
               SIZEOF_PROCARR());

        return -ENOMEM;
    }
    _procarr = tmp;
    act_maxproc = maxproc;
    PROC_UNLOCK();

    return 0;
}

void procarr_insert(struct proc_info * new_proc)
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
     * Remove from parent's child list.
     */
    parent = proc->inh.parent;
    if (parent) {
        mtx_lock(&parent->inh.lock);
        SLIST_REMOVE(&parent->inh.child_list_head, proc, proc_info,
                     inh.child_list_entry);
        mtx_unlock(&parent->inh.lock);
    }

    /*
     * Adopt children to PID 1 if any.
     */
    if (!SLIST_EMPTY(&proc->inh.child_list_head)) {
        struct proc_info * init;
        struct proc_info * child;
        struct proc_info * child_tmp;

        init = proc_get_struct_l(1);
        if (!init)
            panic("init not found\n");

        mtx_lock(&proc->inh.lock);
        SLIST_FOREACH_SAFE(child, &proc->inh.child_list_head,
                           inh.child_list_entry, child_tmp) {
            SLIST_REMOVE(&proc->inh.child_list_head, child,
                         proc_info, inh.child_list_entry);

            child->inh.parent = init; /* re-parent */
            mtx_lock(&init->inh.lock);
            SLIST_INSERT_HEAD(&init->inh.child_list_head, child,
                              inh.child_list_entry);
           mtx_unlock(&init->inh.lock);
        }
        mtx_unlock(&proc->inh.lock);
    }

    _proc_free(proc);
    procarr_remove(proc->pid);
}

void _proc_free(struct proc_info * p)
{
    if (!p) {
        KERROR(KERROR_WARN, "Got NULL as a proc_info struct, double free?\n");

        return;
    }

    /* Close all file descriptors and free files struct. */
    fs_fildes_close_all(p, 0);
    kfree(p->files);

    /*
     * Free regions
     *
     * We don't use lock here because the lock data is invalidated soon and any
     * thread trying to wait for it will break anyway, so we just hope there
     * is no-one trying to lock this process anymore. Technically there
     * shouldn't be any threads locking this process struct anymore.
     */
    if (p->mm.regions) {
        for (int i = 0; i < p->mm.nr_regions; i++) {
            if ((*p->mm.regions)[i]->vm_ops->rfree)
                    (*p->mm.regions)[i]->vm_ops->rfree((*p->mm.regions)[i]);
        }
        p->mm.nr_regions = 0;

        /* Free page table list */
        ptlist_free(&p->mm.ptlist_head);

        /* Free regions array */
        kfree(p->mm.regions);

        /* RFE should not unlock regions? */
    }

    /* Free mpt */
    if (p->mm.mpt.pt_addr)
        ptmapper_free(&p->mm.mpt);

    PROC_LOCK();
    proc_pgrp_remove(p);
    PROC_UNLOCK();
    kfree(p);
}

struct proc_info * proc_get_struct_l(pid_t pid)
{
    istate_t s;
    struct proc_info * retval;

    s = get_interrupt_state();
    if (!(s & PSR_INT_I))
        PROC_LOCK();
    retval = proc_get_struct(pid);
    if (!(s & PSR_INT_I))
        PROC_UNLOCK();

    return retval;
}

struct proc_info * proc_get_struct(pid_t pid)
{
    istate_t s;

    s = get_interrupt_state();
    if (!(s & PSR_INT_I)) {
        KASSERT(PROC_TESTLOCK(),
                "proclock is required before entering proc_get_struct()\n");
    }

    /*
     * Note that we don't do any process state checks here because this function
     * is used in places where the state is invalid and also because doing state
     * check here is jus overhead.
     */
    if (pid > act_maxproc) {
        KERROR(KERROR_ERR, "Invalid PID (%d > %d)\n", pid, act_maxproc);

        return NULL;
    }
    return (*_procarr)[pid];
}

struct vm_mm_struct * proc_get_locked_mm(pid_t pid)
{
    struct proc_info * proc;
    struct vm_mm_struct * mm;

    PROC_LOCK();
    proc = proc_get_struct(pid);
    if (!proc) {
        PROC_UNLOCK();
        return NULL;
    }

    mm = &proc->mm;
    mtx_lock(&mm->regions_lock);
    PROC_UNLOCK();

    return mm;
}

const char * proc_state2str(enum proc_state state)
{
    if ((unsigned)state > sizeof(proc_state_names))
        return NULL;
    return proc_state_names[state];
}

struct thread_info * proc_iterate_threads(struct proc_info * proc,
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
    struct proc_info * p;

    if (!(p = proc_get_struct_l(pid)))
        return;

    /* Go zombie if removed thread was main() */
    if (p->main_thread && (p->main_thread->id == thread_id)) {
        /* Propagate exit signal */
        p->exit_signal = p->main_thread->exit_signal;

        p->main_thread = NULL;
        p->state = PROC_STATE_ZOMBIE;

        /*
         * Invalidate sigs.
         */
        ksignal_signals_dtor(&p->sigs);

        /*
         * Close file descriptors to signal everyone that the process is dead.
         */
        fs_fildes_close_all(p, 0);
    }
}

void proc_update_times(void)
{
    /*
     * TODO Inaccurate proc times calculation
     * A thread is sometimes also blocked in a actual syscall waiting for some
     * event causing a tick to fall in utime.
     */
    if (thread_flags_is_set(current_thread, SCHED_INSYS_FLAG) &&
        thread_state_get(current_thread) != THREAD_STATE_BLOCKED)
        curproc->tms.tms_stime++;
    else
        curproc->tms.tms_utime++;
}

int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread)
{
    const uintptr_t vaddr = far;
    struct vm_mm_struct * mm;
    int err;

    if (!proc) {
        return -ESRCH;
    }

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "%s: MOO, (%s) %x @ %x by %d\n",
           __func__, get_dab_strerror(fsr), vaddr, lr, proc->pid);
#endif

    mm = &proc->mm;

    mtx_lock(&mm->regions_lock);
    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];
        uintptr_t reg_start, reg_end;
#ifdef configPROC_DEBUG
        char uap[5];
#endif

        if (!region)
            continue;

        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + region->b_bufsize - 1;

#ifdef configPROC_DEBUG
        vm_get_uapstring(uap, region);
        KERROR(KERROR_DEBUG, "sect %d: vaddr: %x - %x paddr: %x uap: %s\n",
               i, reg_start, reg_end, region->b_mmu.paddr, uap);
#endif

        if (!VM_ADDR_IS_IN_RANGE(vaddr, reg_start, reg_end))
            continue; /* if not in range then try next region. */

        /*
         * This is the correct region.
         */

        if (MMU_ABORT_IS_TRANSLATION_FAULT(fsr)) { /* Translation fault */
            /*
             * Sometimes we see translation faults due to ordering of region
             * replacements during exec. This is something we have to accept
             * with the current implementation if we want to make exec more
             * robust in failure cases.
             * This may happen if new section A is bigger that old_A and
             * due to that old_B is close to old_A and also overlaps A. Now
             * if old_A is replaced with A but old_B is unmapped later in time
             * it will cause the unmap operation to unmap some of the pages
             * now belonging to A.
             */
            mtx_unlock(&mm->regions_lock);
            vm_mapproc_region(curproc, region);

#ifdef configPROC_DEBUG
            KERROR(KERROR_DEBUG,
                   "DAB \"%s\" of a valid memory region (%d) fixed by remapping the region\n",
                   get_dab_strerror(fsr), i);
#endif

            return 0;
        }

        /* Test for COW flag. */
        if ((region->b_uflags & VM_PROT_COW) != VM_PROT_COW) {
            err = -EACCES; /* Memory protection error. */
            goto fail;
        }

        if (!region->vm_ops->rclone) {
            err = -ENOTSUP; /* rclone() not supported. */
            goto fail;
        }

        region = region->vm_ops->rclone(region);
        if (!region) {
            err = -ENOMEM; /* Can't clone region; COW failed. */
            goto fail;
        }
        /*
         * The old region remains marked as COW as it would be racy to change
         * its state.
         */

        mtx_unlock(&mm->regions_lock);
        err = vm_replace_region(proc, region, i, VM_INSOP_MAP_REG);

#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "COW done (%d)\n", err);
#endif
        return err; /* COW done. */
    }

    err = -EFAULT;
fail:
    mtx_unlock(&mm->regions_lock);

    return err; /* Not found or error occured. */
}

pid_t proc_update(void)
{
    pid_t current_pid;

    current_pid = current_thread->pid_owner;
    curproc = proc_get_struct_l(current_pid);

    KASSERT(curproc, "curproc should be valid");

    return current_pid;
}

/**
 * Get/Set maxproc value.
 * Note that setting the value doesn't cause procarr to be resized until
 * it's actually necessary.
 */
static int sysctl_proc_maxproc(SYSCTL_HANDLER_ARGS)
{
    int new_maxproc = maxproc;
    int error;

    error = sysctl_handle_int(oidp, &new_maxproc, sizeof(new_maxproc), req);
    if (!error && req->newptr) {
        if (new_maxproc < nprocs)
            error = -EINVAL;
        else
            maxproc = new_maxproc;
    }

    return error;
}
SYSCTL_PROC(_kern, KERN_MAXPROC, maxproc, CTLTYPE_INT | CTLFLAG_RW,
            NULL, 0, sysctl_proc_maxproc, "I", "Maximum number of processes");

/* Syscall handlers ***********************************************************/

static int sys_proc_fork(__user void * user_args)
{
    pid_t pid = proc_fork(curproc->pid);
    if (pid < 0) {
        set_errno(-pid);
        return -1;
    } else {
        return pid;
    }
}

static int sys_proc_wait(__user void * user_args)
{
    struct _proc_wait_args args;
    pid_t pid_child;
    struct proc_info * child = NULL;
    enum proc_state * state;

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
        child = SLIST_FIRST(&curproc->inh.child_list_head);
    } else if (args.pid < -1) {
        /*
         * TODO
         * If pid is less than (pid_t)-1, status is requested for any child
         * process whose process group ID is equal to the absolute value of pid.
         */
        set_errno(ENOTSUP);
        return -1;
    } else if (args.pid > 0) {
        struct proc_info * p;
        struct proc_info * tmp;

        p = proc_get_struct_l(args.pid);
        tmp = SLIST_FIRST(&curproc->inh.child_list_head);
        if (!p || !tmp) {
            set_errno(ECHILD);
            return -1;
        }

        /*
         * Check that p is a child of curproc.
         */
        mtx_lock(&curproc->inh.lock);
        SLIST_FOREACH(tmp, &curproc->inh.child_list_head, inh.child_list_entry)
        {
            if (tmp->pid == p->pid) {
                child = p;
                break;
            }
        }
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
        sigset_t set;
        const struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
        siginfo_t sigretval;

        /*
         * We may have already miss SIGCHLD that's ignored by default,
         * so we have to use timedwait and periodically check is the
         * child is zombie.
         */
        sigemptyset(&set);
        sigaddset(&set, SIGCHLD);
        ksignal_sigtimedwait(&sigretval, &set, &ts);

        /*
         * TODO In some cases we have to return early without waiting.
         * eg. signal received
         */
    }

    /*
     * Construct a status value.
     */
    args.status = (child->exit_code & 0xff) << 8 | (child->exit_signal & 0177);

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

static int sys_proc_exit(__user void * user_args)
{
    const struct ksignal_param sigparm = {
        .si_code = SI_USER,
    };

    KASSERT(curproc->inh.parent, "parent should exist");

    curproc->exit_code = get_errno();

    (void)ksignal_sendsig(&curproc->inh.parent->sigs, SIGCHLD, &sigparm);
    thread_flags_set(current_thread, SCHED_DETACH_FLAG);
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
static int sys_proc_getsetcred(__user void * user_args)
{
    struct _proc_credctl_args pcred;
    uid_t ruid = curproc->cred.uid;
    /*uid_t euid = curproc->cred.euid;*/
    uid_t suid = curproc->cred.suid;
    gid_t rgid = curproc->cred.gid;
    gid_t sgid = curproc->cred.sgid;
    int retval = 0;

    if (!useracc(user_args, sizeof(pcred), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }
    copyin(user_args, &pcred, sizeof(pcred));

    /* Set uid */
    if (pcred.ruid >= 0) {
        if (priv_check(&curproc->cred, PRIV_CRED_SETUID) == 0)
            curproc->cred.uid = pcred.ruid;
        else
            retval = -1;
    }

    /* Set euid */
    if (pcred.euid >= 0) {
        uid_t new_euid = pcred.euid;

        if ((priv_check(&curproc->cred, PRIV_CRED_SETEUID) == 0) ||
                new_euid == ruid || new_euid == suid)
            curproc->cred.euid = new_euid;
        else
            retval = -1;
    }

    /* Set suid */
    if (pcred.suid >= 0) {
        if (priv_check(&curproc->cred, PRIV_CRED_SETSUID) == 0)
            curproc->cred.suid = pcred.suid;
        else
            retval = -1;
    }

    /* Set gid */
    if (pcred.rgid >= 0) {
        if (priv_check(&curproc->cred, PRIV_CRED_SETGID) == 0)
            curproc->cred.gid = pcred.rgid;
        else
            retval = -1;
    }

    /* Set egid */
    if (pcred.egid >= 0) {
        gid_t new_egid = pcred.egid;

        if ((priv_check(&curproc->cred, PRIV_CRED_SETEGID) == 0) ||
                new_egid == rgid || new_egid == sgid)
            curproc->cred.egid = pcred.egid;
        else
            retval = -1;
    }

    /* Set sgid */
    if (pcred.sgid >= 0) {
        if (priv_check(&curproc->cred, PRIV_CRED_SETSGID) == 0)
            curproc->cred.sgid = pcred.sgid;
        else
            retval = -1;
    }

    if (retval)
        set_errno(EPERM);

    /* Get current values */
    pcred.ruid = curproc->cred.uid;
    pcred.euid = curproc->cred.euid;
    pcred.suid = curproc->cred.suid;
    pcred.rgid = curproc->cred.gid;
    pcred.egid = curproc->cred.egid;
    pcred.sgid = curproc->cred.sgid;

    copyout(&pcred, user_args, sizeof(pcred));

    return retval;
}

static int sys_proc_getgroups(__user void * user_args)
{
    struct _proc_getgroups_args args;
    const size_t max = NGROUPS_MAX * sizeof(gid_t);

    if (copyin(user_args, &args, sizeof(args))) {
        set_errno(EFAULT);
        return -1;
    }

    if (copyout(&curproc->cred.sup_gid, (__user gid_t *)args.grouplist,
                (args.size < max) ? args.size : max)) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_setgroups(__user void * user_args)
{
    struct _proc_setgroups_args args;
    const size_t max = NGROUPS_MAX * sizeof(gid_t);
    int err;

    err = priv_check(&curproc->cred, PRIV_CRED_SETGROUPS);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    if (copyin(user_args, &args, sizeof(args))) {
        set_errno(EFAULT);
        return -1;
    }

    if (copyin((__user gid_t *)args.grouplist, &curproc->cred.sup_gid,
               (args.size < max) ? args.size : max)) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getsid(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    pid_t sid = -1;

    if (pid == 0) {
        sid = curproc->pgrp->pg_session->s_leader;
    } else {
        struct proc_info * proc;

        PROC_LOCK();
        proc = proc_get_struct(pid);
        if (proc) {
            sid = proc->pgrp->pg_session->s_leader;
        }
        PROC_UNLOCK();
    }

    if (sid == -1)
        set_errno(ESRCH);
    return sid;
}

static int sys_proc_setsid(__user void * user_args)
{
    pid_t pid = curproc->pid;
    char logname[MAXLOGNAME];
    struct session * s = NULL;
    struct pgrp * pg = NULL;

    /*
     * RFE Technically not any process group id should match with this PID but
     *     we can't check it very efficiently right now.
     */
    if (pid == curproc->pgrp->pg_id ||
        pid == curproc->pgrp->pg_session->s_leader) {
        set_errno(EPERM);
        return -1;
    }

    strlcpy(logname, curproc->pgrp->pg_session->s_login, sizeof(logname));
    s = proc_session_create(curproc, logname);
    if (!s) {
        set_errno(ENOMEM);
        return -1;
    }

    PROC_LOCK();
    pg = proc_pgrp_create(s, curproc);
    PROC_UNLOCK();
    if (!pg) {
        proc_session_remove(s);
        set_errno(ENOMEM);
        return -1;
    }

    return pid;
}

static int sys_proc_getpgrp(__user void * user_args)
{
    return curproc->pgrp->pg_id;
}

static int sys_prog_setpgid(__user void * user_args)
{
    struct _proc_setpgid_args args;
    struct proc_info * proc;
    pid_t pg_id;
    int retval = -1;

    if (copyin(user_args, &args, sizeof(args))) {
            set_errno(EFAULT);
            return -1;
    }

    if (args.pg_id < 0) {
        set_errno(EINVAL);
        return -1;
    }

    PROC_LOCK();

    if (args.pid == 0 || args.pid == curproc->pid) {
        proc = curproc;
    } else {
        proc = proc_get_struct(args.pid);
        if (proc) {
            if (proc->inh.parent != curproc) {
                /*
                 * proc must be the current process or a child of the curproc
                 * as per POSIX.
                 */
                proc = NULL;
            } else if (proc->pgrp->pg_session != curproc->pgrp->pg_session) {
                set_errno(EPERM);
                goto fail;
            } /* TODO else if already called exec */
        }
    }
    if (!proc) {
        set_errno(ESRCH);
        goto fail;
    }
    if (proc->pid == proc->pgrp->pg_session->s_leader) {
        set_errno(EPERM);
        goto fail;
    }

    if (args.pg_id == 0)
        pg_id = curproc->pid;
    else
        pg_id = args.pg_id;

    /*
     * Either insert to an existing group or create a new group.
     */
    if (pg_id != proc->pid) {
        struct pgrp * pg;

        pg = proc_session_search_pg(proc->pgrp->pg_session, pg_id);
        if (!pg) {
            set_errno(EPERM);
            goto fail;
        }

        proc_pgrp_insert(pg, proc);
    } else if (!proc_pgrp_create(curproc->pgrp->pg_session, proc)) {
        set_errno(ENOMEM);
        goto fail;
    }

    retval = 0;
fail:
    PROC_UNLOCK();
    return retval;
}

static int sys_proc_getlogin(__user void * user_args)
{
    KASSERT(curproc->pgrp && curproc->pgrp->pg_session,
            "Session is valid");

    if (copyout(curproc->pgrp->pg_session->s_login, user_args, MAXLOGNAME)) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_setlogin(__user void * user_args)
{
    int err;

    KASSERT(curproc->pgrp && curproc->pgrp->pg_session,
            "Session is valid");

    err = priv_check(&curproc->cred, PRIV_PROC_SETLOGIN);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    if (copyin(user_args, curproc->pgrp->pg_session->s_login, MAXLOGNAME)) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getpid(__user void * user_args)
{
    if (copyout(&curproc->pid, user_args, sizeof(pid_t))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getppid(__user void * user_args)
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

static int sys_proc_chdir(__user void * user_args)
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

static int sys_chroot(__user void * user_args)
{
    int err;

    err = priv_check(&curproc->cred, PRIV_VFS_CHROOT);
    if (err) {
        set_errno(-err);
        return -1;
    }

    /* TODO Should we free or take some new refs? */
    curproc->croot = curproc->cwd;

    return 0;
}

static int sys_proc_setpolicy(__user void * user_args)
{
    struct _setpolicy_args args;
    struct proc_info * p;
    uid_t p_euid;
    pthread_t tid;
    int err;

    if ((err = copyin(user_args, &args, sizeof(args)))) {
        set_errno(-err);
        return -1;
    }

    if (args.id == 0) {
        set_errno(ESRCH);
        return -1;
    }

    PROC_LOCK();
    p = proc_get_struct(args.id);
    if (!p || !p->main_thread) {
        set_errno(ESRCH);
        PROC_UNLOCK();
        return -1;
    }
    p_euid = p->cred.euid;
    tid = p->main_thread->id;
    PROC_UNLOCK();

    if (((args.policy != SCHED_OTHER || curproc->cred.euid != p_euid) &&
         (err = priv_check(&curproc->cred, PRIV_SCHED_SETPOLICY))) ||
        (err = thread_set_policy(tid, args.policy))) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_proc_getpolicy(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    struct proc_info * p;
    int policy;

    if (pid == 0) {
        set_errno(ESRCH);
        return -1;
    }

    PROC_LOCK();
    p = proc_get_struct(pid);
    if (!p || !p->main_thread ||
        (policy = thread_get_policy(p->main_thread->id)) < 0) {
        set_errno(ESRCH);
        policy = -1;
    }
    PROC_UNLOCK();

    return policy;
}

static int sys_proc_setpriority(__user void * user_args)
{
    struct _set_priority_args args;
    struct proc_info * p;
    uid_t p_euid;
    pthread_t tid;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(ESRCH);
        return -1;
    }


    if (args.id == 0) {
        set_errno(ESRCH);
        return -1;
    }

    PROC_LOCK();
    p = proc_get_struct(args.id);
    if (!p || !p->main_thread) {
        set_errno(ESRCH);
        PROC_UNLOCK();
        return -1;
    }
    p_euid = p->cred.euid;
    tid = p->main_thread->id;
    PROC_UNLOCK();

    if ((args.priority < 0 || curproc->cred.euid != p_euid) &&
        priv_check(&curproc->cred, PRIV_SCHED_SETPRIORITY)) {
        set_errno(ESRCH);
        return -1;
    }

    err = thread_set_priority(tid, args.priority);
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_proc_getpriority(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    struct proc_info * p;
    int prio;

    if (pid == 0) {
        set_errno(ESRCH);
        return -1;
    }

    PROC_LOCK();
    p = proc_get_struct(pid);
    if (!p || !p->main_thread ||
        (prio = thread_get_priority(p->main_thread->id)) == NICE_ERR) {
        set_errno(ESRCH);
        prio = -1;
    }
    PROC_UNLOCK();

    return prio;
}

static int sys_proc_getrlim(__user void * user_args)
{
    struct _proc_rlim_args args;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE) ||
            copyin(user_args, &args, sizeof(args))) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.resource < 0 || args.resource >= _RLIMIT_ARR_COUNT) {
        set_errno(EINVAL);
        return -1;
    }

    memcpy(&args.rlimit, &curproc->rlim[args.resource], sizeof(struct rlimit));
    copyout(&args, user_args, sizeof(args));

    return 0;
}

static int sys_proc_setrlim(__user void * user_args)
{
    struct _proc_rlim_args args;
    rlim_t current_rlim_max;

    if (copyin(user_args, &args, sizeof(args))) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.resource < 0 || args.resource >= _RLIMIT_ARR_COUNT) {
        set_errno(EINVAL);
        return -1;
    }

    /*
     * rlim_max is the ceiling value for rlim_cur if the current process is
     * unprivileged. A privileged process may make arbitrary changes to either
     * limit value.
     */
    current_rlim_max = curproc->rlim[args.resource].rlim_max;
    if ((args.rlimit.rlim_cur > current_rlim_max ||
         args.rlimit.rlim_max > current_rlim_max) &&
        priv_check(&curproc->cred, PRIV_PROC_SETRLIMIT)) {
        set_errno(EPERM);
        return -1;
    }

    /*
     * Validate limit values.
     */
    switch (args.resource) {
    case RLIMIT_CORE ... RLIMIT_AS:
        if ((args.rlimit.rlim_cur < RLIM_INFINITY) ||
            (args.rlimit.rlim_max < RLIM_INFINITY)) {
            set_errno(EINVAL);
            return -1;
        }
    }

    memcpy(&curproc->rlim[args.resource], &args.rlimit, sizeof(struct rlimit));

    return -1;
}

static int sys_proc_times(__user void * user_args)
{
    if (copyout(&curproc->tms, user_args, sizeof(struct tms))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_proc_getbreak(__user void * user_args)
{
    struct _proc_getbreak_args args;
    int err;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        set_errno(EFAULT);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    args.start = curproc->brk_start;
    args.stop = curproc->brk_stop;
    err |= copyout(&args, user_args, sizeof(args));
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
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETGROUPS, sys_proc_getgroups),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETGROUPS, sys_proc_setgroups),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETSID, sys_proc_getsid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETSID, sys_proc_setsid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPGRP, sys_proc_getpgrp),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETPGID, sys_prog_setpgid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETLOGIN, sys_proc_getlogin),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETLOGIN, sys_proc_setlogin),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPID, sys_proc_getpid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPPID, sys_proc_getppid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_CHDIR, sys_proc_chdir),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_CHROOT, sys_chroot),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETPOLICY, sys_proc_setpolicy),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPOLICY, sys_proc_getpolicy),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETPRIORITY, sys_proc_setpriority),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETPRIORITY, sys_proc_getpriority),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETRLIM, sys_proc_getrlim),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_SETRLIM, sys_proc_setrlim),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_TIMES, sys_proc_times),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PROC_GETBREAK, sys_proc_getbreak),
};
SYSCALL_HANDLERDEF(proc_syscall, proc_sysfnmap)
