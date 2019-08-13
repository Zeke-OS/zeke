/**
 *******************************************************************************
 * @file    proc.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>
#include <buf.h>
#include <exec.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kmem.h>
#include <ksched.h>
#include <kstring.h>
#include <libkern.h>
#include <mempool.h>
#include <proc.h>
#include <vm/vm_copyinstruct.h>

#define SIZEOF_PROCARR ((configMAXPROC + 1) * sizeof(struct proc_info *))

/**
 * Processes indexed by pid.
 */
static struct proc_info *procarr[SIZEOF_PROCARR];
int nprocs = 1;             /*!< Current # of procs. */
struct proc_info * curproc; /*!< PCB of the current process. */

static const struct vm_ops sys_vm_ops; /* NOOP struct for system regions. */

extern vnode_t kerror_vnode;
extern void * __bss_break __attribute__((weak));

/**
 * proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
mtx_t proclock = MTX_INITIALIZER(MTX_TYPE_SPIN, MTX_OPT_DINT);

static const char * const proc_state_names[] = {
    "PROC_STATE_INITIAL",
    "PROC_STATE_RUNNING", /* not used atm */
    "PROC_STATE_READY",
    "PROC_STATE_WAITING", /* not used atm */
    "PROC_STATE_STOPPED",
    "PROC_STATE_ZOMBIE",
    "PROC_STATE_DEFUNCT",
};

/*
 * PID buffers.
 */
#define NR_PIDS_BUFS 4
static pid_t pids_buf[NR_PIDS_BUFS][configMAXPROC + 1];
static isema_t pids_buf_isema[NR_PIDS_BUFS] = ISEMA_INITIALIZER(NR_PIDS_BUFS);

static void init_kernel_proc(void);
static void procarr_remove(pid_t pid);
static void proc_remove(struct proc_info * proc);
pid_t proc_update(void); /* Used in HAL, so not static but not in headeaders. */

int __kinit__ proc_init(void)
{
    int _proc_init_fork(void);
    SUBSYS_INIT("proc");

    init_kernel_proc();
    /* Do here the same as proc_update() would do when running. */
    curproc = procarr[0];

    return _proc_init_fork();
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
    int err;

    procarr[0] = kzalloc_crit(sizeof(struct proc_info));
    kernel_proc = procarr[0];

    kernel_proc->pid = 0;
    kernel_proc->state = PROC_STATE_READY;
    kernel_proc->nice = NZERO;
    strcpy(kernel_proc->name, "kernel");

    /*
     * Initialize a session.
     */
    PROC_LOCK();
    kernel_proc->pgrp = proc_pgrp_create(NULL, kernel_proc);
    PROC_UNLOCK();
    if (!kernel_proc->pgrp) {
        panic(panic_msg);
    }
    proc_session_setlogin(kernel_proc->pgrp->pg_session, "root");

    priv_cred_init(&kernel_proc->cred);

    RB_INIT(&(kernel_proc->mm.ptlist_head));

    /* Copy master page table descriptor */
    memcpy(&kernel_proc->mm.mpt, &mmu_pagetable_master,
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
    struct buf * kprocvm_code = kzalloc_crit(sizeof(struct buf));
    struct buf * kprocvm_heap = kzalloc_crit(sizeof(struct buf));

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
    kernel_proc->files = fs_alloc_files(8, CMASK);
    if (!kernel_proc->files) {
        panic(panic_msg);
    }

    kernel_proc->files->fd[STDIN_FILENO] = NULL;
    /* stderr */
#ifdef configKLOGGER
    kernel_proc->files->fd[STDERR_FILENO] = kzalloc_crit(sizeof(file_t));
    if (fs_fildes_set(kernel_proc->files->fd[STDERR_FILENO],
                      &kerror_vnode, O_WRONLY)) {
        panic(panic_msg);
    }
    kernel_proc->files->fd[STDOUT_FILENO] = fs_fildes_ref(kernel_proc->files,
                                                          STDERR_FILENO, 1);
#else
    kernel_proc->files->fd[STDOUT_FILENO] = NULL;
    kernel_proc->files->fd[STDERR_FILENO] = NULL;
#endif

    init_rlims(&kernel_proc->rlim);

    mtx_init(&kernel_proc->inh.lock, PROC_INH_LOCK_TYPE, PROC_INH_LOCK_OPT);
}

void procarr_insert(struct proc_info * new_proc)
{
    KASSERT(new_proc, "new_proc can't be NULL");

    KERROR_DBG("%s(%d)\n", __func__, new_proc->pid);

    if (new_proc->pid > configMAXPROC || new_proc->pid < 0) {
        KERROR(KERROR_ERR, "Inserted new_proc out of bounds (%d)\n",
               new_proc->pid);
        return;
    }

    PROC_LOCK();
    procarr[new_proc->pid] = new_proc;
    nprocs++;
    PROC_UNLOCK();
}

static void procarr_remove(pid_t pid)
{
    if (pid > configMAXPROC || pid < 0) {
        KERROR(KERROR_ERR, "Attempt to remove a nonexistent process\n");
        return;
    }

    PROC_LOCK();
    procarr[pid] = NULL;
    nprocs--;
    PROC_UNLOCK();
}

pid_t * proc_get_pids_buffer(void)
{
    pid_t * buf;

    buf = pids_buf[isema_acquire(pids_buf_isema, num_elem(pids_buf_isema))];
    memset(buf, 0, configMAXPROC + 1);

    return buf;
}

void proc_release_pids_buffer(pid_t * buf)
{
    uintptr_t i = ((uintptr_t)buf - (uintptr_t)pids_buf) / sizeof(pids_buf[0]);
    isema_release(pids_buf_isema, i);
}

void proc_get_pids(pid_t * pids)
{
    size_t i, j = 0;

    PROC_KASSERT_LOCK();

    for (i = 1; i < configMAXPROC + 1; i++) {
        struct proc_info * proc = procarr[i];

        if (proc) {
            pids[j++] = proc->pid;
        }
    }
}

/**
 * Remove zombie process from the system.
 */
static void proc_remove(struct proc_info * proc)
{
    struct proc_info * parent;

    KASSERT(proc, "Attempt to remove NULL proc");
    KERROR_DBG("%s(%d)\n", __func__, proc->pid);

    proc->state = PROC_STATE_DEFUNCT;

    /*
     * Remove from parent's child list.
     */
    parent = proc->inh.parent;
    if (parent) {
        mtx_lock(&parent->inh.lock);
        PROC_INH_REMOVE(parent, proc);
        mtx_unlock(&parent->inh.lock);
    }

    /*
     * Adopt children to PID 1 if any.
     */
    if (!PROC_INH_IS_EMPTY(proc)) {
        struct proc_info * init;
        struct proc_info * child;
        struct proc_info * child_tmp;

        init = proc_ref(1);
        if (!init)
            panic("init not found\n");

        mtx_lock(&proc->inh.lock);
        PROC_INH_FOREACH_SAFE(child, child_tmp, proc) {
            PROC_INH_REMOVE(proc, child);

            child->inh.parent = init; /* re-parent */
            mtx_lock(&init->inh.lock);
            PROC_INH_INSERT_HEAD(init, child);
            mtx_unlock(&init->inh.lock);
        }
        mtx_unlock(&proc->inh.lock);
        proc_unref(init);
    }


    vrele(proc->cwd);
    proc_free(proc);
    procarr_remove(proc->pid);
}

void proc_free(struct proc_info * p)
{
    if (!p) {
        KERROR(KERROR_WARN, "Got NULL as a proc_info struct, double free?\n");

        return;
    }

    /* exit_ksiginfo isn't needed anymore */
    kfree(p->exit_ksiginfo);

    /* Close all file descriptors and free files struct. */
    fs_fildes_close_all(p, 0);
    kfree(p->files);

    vm_mm_destroy(&p->mm);

    PROC_LOCK();
    proc_pgrp_remove(p);
    PROC_UNLOCK();
    mempool_return(proc_pool, p);
}

/**
 * Get pointer to a internal proc_info structure.
 * This function must remain static, external users should call proc_ref().
 * @note Requires PROC_LOCK.
 */
static struct proc_info * proc_get_struct(pid_t pid)
{
    if (0 > pid || pid > configMAXPROC) {
        KERROR(KERROR_ERR, "Invalid PID (%d, max: %d)\n", pid, configMAXPROC);

        return NULL;
    }
    return procarr[pid];
}

int proc_exists_locked(pid_t pid)
{
    struct proc_info * proc = proc_ref_locked(pid);

    if (proc) {
        proc_unref(proc);
        return (1 == 1);
    }
    return 0;
}

int proc_exists(pid_t pid)
{
    struct proc_info * proc = proc_ref(pid);

    if (proc) {
        proc_unref(proc);
        return (1 == 1);
    }
    return 0;
}

struct proc_info * proc_ref_locked(pid_t pid)
{
    PROC_KASSERT_LOCK();

    return kpalloc(proc_get_struct(pid));
}

struct proc_info * proc_ref(pid_t pid)
{
    struct proc_info * proc;

    /*
     * RFE mm protection is not perfect
     * Something bad might happen if there is an on going mem transfer op and
     * the process is removed.
     */

    PROC_LOCK();
    proc = kpalloc(proc_get_struct(pid));
    PROC_UNLOCK();

    return proc;
}

void proc_unref(struct proc_info * proc)
{
    kfree(proc);
}

const char * proc_state2str(enum proc_state state)
{
    return ((unsigned)state > sizeof(proc_state_names)) ?
        "PROC_STATE_INVALID" : proc_state_names[state];
}

dev_t get_ctty(struct proc_info * proc)
{
    int err, fd = proc->pgrp->pg_session->s_ctty_fd;
    struct stat stat_buf;
    file_t * file;
    vnode_t * vnode;
    dev_t ctty = 0;

    file = fs_fildes_ref(proc->files, fd, 1);
    if (!file)
        return 0;

    vnode = file->vnode;
    err = vnode->vnode_ops->stat(vnode, &stat_buf);
    if (err)
        goto out;

    ctty = stat_buf.st_rdev;

out:
    fs_fildes_ref(proc->files, fd, -1);
    return ctty;
}

struct thread_info * proc_iterate_threads(const struct proc_info * proc,
                                          struct thread_info ** thread_it)
{
    struct thread_info * tmp = *thread_it;

    if (!tmp)
        *thread_it = proc->main_thread;
    else if (tmp == proc->main_thread)
        *thread_it = (*thread_it)->inh.first_child;
    else
        *thread_it = (*thread_it)->inh.next_child;

    return *thread_it;
}

/* Called when thread is completely removed from the scheduler */
void proc_thread_removed(pid_t pid, pthread_t thread_id)
{
    struct proc_info * p;

    if (!(p = proc_ref(pid)))
        return;

    /* Go zombie if removed thread was main() */
    if (p->main_thread && (p->main_thread->id == thread_id)) {
        /* Propagate exit signal */
        if (p->main_thread->exit_ksiginfo) {
            kpalloc(p->main_thread->exit_ksiginfo);
            p->exit_ksiginfo = p->main_thread->exit_ksiginfo;
        }

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

    proc_unref(p);
}

void proc_update_times(void)
{
    if (thread_flags_is_set(current_thread, SCHED_INSYS_FLAG) &&
        thread_state_get(current_thread) != THREAD_STATE_BLOCKED) {
        curproc->tms.tms_stime++;
    } else {
        curproc->tms.tms_utime++;
    }
}
SCHED_PRE_SCHED_TASK(proc_update_times);

int proc_abo_handler(const struct mmu_abo_param * restrict abo)
{
    const uintptr_t vaddr = abo->far;
    struct vm_mm_struct * mm;
    const char * abo_str = mmu_abo_strerror(abo);
    int err;

    if (!abo->proc) {
        return -ESRCH;
    }

    KERROR_DBG("%s: MOO, (%s) %x @ %x by %d:%d\n", __func__,
               abo_str, (unsigned)vaddr, (unsigned)abo->lr,
               abo->proc->pid, abo->thread->id);

    mm = &abo->proc->mm;

    mtx_lock(&mm->regions_lock);
    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];
        uintptr_t reg_start, reg_end;
        char uap[5];

        if (!region)
            continue;

        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + region->b_bufsize - 1;

        vm_get_uapstring(uap, region);
        KERROR_DBG("sect %d: vaddr: %x - %x paddr: %x uap: %s\n",
                   i, (unsigned)reg_start, (unsigned)reg_end,
                   (unsigned)region->b_mmu.paddr, uap);

        if (!VM_ADDR_IS_IN_RANGE(vaddr, reg_start, reg_end))
            continue; /* if not in range then try next region. */

        /*
         * This is the correct region.
         */

        if (MMU_ABORT_IS_TRANSLATION_FAULT(abo->fsr)) { /* Translation fault */
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
            vm_mapproc_region(abo->proc, region);

            KERROR_DBG("%s \"%s\" of a valid memory region (%d) fixed by remapping the region\n",
                       mmu_abo_strtype(abo), abo_str, i);

            return 0;
        }

        /* Test for COW flag. */
        if ((region->b_uflags & VM_PROT_COW) != VM_PROT_COW) {
            KERROR_DBG("Memory protection error\n");
            err = -EACCES; /* Memory protection error. */
            goto fail;
        }

        if (!region->vm_ops->rclone) {
            KERROR_DBG("rclone() not supported\n");
            err = -ENOTSUP; /* rclone() not supported. */
            goto fail;
        }

        region = region->vm_ops->rclone(region);
        if (!region) {
            KERROR_DBG("Can't clone region; COW failed\n");
            err = -ENOMEM; /* Can't clone region; COW failed. */
            goto fail;
        }
        /*
         * The old region remains marked as COW as it would be racy to change
         * its state.
         */

        mtx_unlock(&mm->regions_lock);
        err = vm_replace_region(abo->proc, region, i, VM_INSOP_MAP_REG);

        KERROR_DBG("COW done (%d)\n", err);
        return err; /* COW done. */
    }

    KERROR_DBG("No mapping found\n");
    err = -EFAULT;
fail:
    mtx_unlock(&mm->regions_lock);

    return err; /* Not found or error occured. */
}

pid_t proc_update(void)
{
    pid_t current_pid;

    current_pid = current_thread->pid_owner;
    PROC_LOCK();
    curproc = proc_get_struct(current_pid);
    PROC_UNLOCK();

    KASSERT(curproc, "curproc should be valid");

    return current_pid;
}

/* Syscall handlers ***********************************************************/

static intptr_t sys_proc_fork(__user void * user_args)
{
    int err;

    err = priv_check(&curproc->cred, PRIV_PROC_FORK);
    if (err) {
        set_errno(-err);
        return -1;
    }

    pid_t pid = proc_fork();
    if (pid < 0) {
        set_errno(-pid);
        return -1;
    } else {
        return pid;
    }
}

static intptr_t sys_proc_wait(__user void * user_args)
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
        child = PROC_INH_FIRST(curproc);
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

        p = proc_ref(args.pid);
        tmp = PROC_INH_FIRST(curproc);
        if (!p || !tmp) {
            set_errno(ECHILD);
            return -1;
        }

        /*
         * Check that p is a child of curproc.
         */
        mtx_lock(&curproc->inh.lock);
        PROC_INH_FOREACH(tmp, curproc) {
            if (tmp->pid == p->pid) {
                child = p;
                break;
            }
        }
        proc_unref(p); /* A true child wont be freed before we are ready. */
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
    args.status = (child->exit_code & 0xff) << 8;
    if (child->exit_ksiginfo) { /* if signaled. */
        struct ksiginfo * ksig = child->exit_ksiginfo;

        args.status |= ksig->siginfo.si_signo & 0177;
        if (ksig->siginfo.si_code == CLD_DUMPED) {
            args.status |= 0200; /* for WCOREDUMP. */
        }
    }

    if (args.options & WNOWAIT) {
        /*
         * Leave the proc around, available for later waits.
         */
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

    /* Remove the wait'd process */
    proc_remove(child);

    return (uintptr_t)pid_child;
}

static intptr_t sys_proc_exit(__user void * user_args)
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
static intptr_t sys_proc_getsetcred(__user void * user_args)
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

static intptr_t sys_proc_getgroups(__user void * user_args)
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

static intptr_t sys_proc_setgroups(__user void * user_args)
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

static intptr_t sys_proc_getsid(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    pid_t sid = -1;

    if (pid == 0) {
        sid = curproc->pgrp->pg_session->s_leader;
    } else {
        struct proc_info * proc;

        proc = proc_ref(pid);
        if (proc) {
            sid = proc->pgrp->pg_session->s_leader;
            proc_unref(proc);
        }
    }

    if (sid == -1)
        set_errno(ESRCH);
    return sid;
}

static intptr_t sys_proc_setsid(__user void * user_args)
{
    pid_t pid = curproc->pid;
    struct pgrp * pg;

    /*
     * RFE Technically not any process group id should match with this PID but
     *     we can't check it very efficiently right now.
     */
    if (pid == curproc->pgrp->pg_id ||
        pid == curproc->pgrp->pg_session->s_leader) {
        set_errno(EPERM);
        return -1;
    }

    PROC_LOCK();
    pg = proc_pgrp_create(NULL, curproc);
    PROC_UNLOCK();
    if (!pg) {
        set_errno(ENOMEM);
        return -1;
    }
    proc_session_setlogin(pg->pg_session, curproc->pgrp->pg_session->s_login);

    return pid;
}

static intptr_t sys_proc_getpgrp(__user void * user_args)
{
    return curproc->pgrp->pg_id;
}

static intptr_t sys_prog_setpgid(__user void * user_args)
{
    /* RFE May not need proclock */
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
        proc = proc_ref_locked(curproc->pid);
    } else {
        proc = proc_ref_locked(args.pid);
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
    proc_unref(proc);
    return retval;
}

static intptr_t sys_proc_getlogin(__user void * user_args)
{
    KASSERT(curproc->pgrp && curproc->pgrp->pg_session,
            "Session is valid");

    if (copyout(curproc->pgrp->pg_session->s_login, user_args, MAXLOGNAME)) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static intptr_t sys_proc_setlogin(__user void * user_args)
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

static intptr_t sys_proc_getpid(__user void * user_args)
{
    if (copyout(&curproc->pid, user_args, sizeof(pid_t))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static intptr_t sys_proc_getppid(__user void * user_args)
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

static intptr_t sys_proc_chdir(__user void * user_args)
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

static intptr_t sys_chroot(__user void * user_args)
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

static intptr_t sys_proc_setpolicy(__user void * user_args)
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

    p = proc_ref(args.id);
    if (!p || !p->main_thread) {
        set_errno(ESRCH);
        return -1;
    }
    p_euid = p->cred.euid;
    tid = p->main_thread->id;
    proc_unref(p);

    if (((args.policy != SCHED_OTHER || curproc->cred.euid != p_euid) &&
         (err = priv_check(&curproc->cred, PRIV_SCHED_SETPOLICY))) ||
        (err = thread_set_policy(tid, args.policy))) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static intptr_t sys_proc_getpolicy(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    struct proc_info * p;
    int policy;

    if (pid == 0) {
        set_errno(ESRCH);
        return -1;
    }

    p = proc_ref(pid);
    if (!p || !p->main_thread ||
        (policy = thread_get_policy(p->main_thread->id)) < 0) {
        set_errno(ESRCH);
        policy = -1;
    }
    proc_unref(p);

    return policy;
}

static intptr_t sys_proc_setpriority(__user void * user_args)
{
    struct _set_priority_args args;
    struct proc_info * p;

    if (copyin(user_args, &args, sizeof(args))) {
        set_errno(ESRCH);
        return -1;
    }

    if (args.id == 0) {
        set_errno(ESRCH);
        return -1;
    }

    p = proc_ref(args.id);
    if (!p) {
        set_errno(ESRCH);
        return -1;
    }

    if ((args.priority < 0 || curproc->cred.euid != p->cred.euid) &&
        priv_check(&curproc->cred, PRIV_SCHED_SETPRIORITY)) {
        proc_unref(p);
        set_errno(ESRCH);
        return -1;
    }

    if (!(NICE_MIN <= args.priority && args.priority <= NICE_MAX)) {
        return -EACCES;
    }

    p->nice = args.priority;
    proc_unref(p);

    return 0;
}

static intptr_t sys_proc_getpriority(__user void * user_args)
{
    pid_t pid = (pid_t)user_args;
    struct proc_info * p;
    int prio;

    if (pid == 0) {
        set_errno(ESRCH);
        return -1;
    }

    p = proc_ref(pid);
    if (!p) {
        set_errno(ESRCH);
        return -1;
    }
    prio = p->nice;
    proc_unref(p);

    return prio;
}

static intptr_t sys_proc_getrlim(__user void * user_args)
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

static intptr_t sys_proc_setrlim(__user void * user_args)
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

static intptr_t sys_proc_times(__user void * user_args)
{
    if (copyout(&curproc->tms, user_args, sizeof(struct tms))) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static intptr_t sys_proc_getbreak(__user void * user_args)
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
