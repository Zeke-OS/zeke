/**
 *******************************************************************************
 * @file    thread.c
 * @author  Olli Vanhoja
 * @brief   Generic thread management and scheduling functions.
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

#define KERNEL_INTERNAL
#include <libkern.h>
#include <kstring.h>
#include <sys/linker_set.h>
#include <hal/core.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <buf.h>
#include <syscall.h>
#include <kerror.h>
#include <errno.h>
#include <timers.h>
#include <proc.h>
#include <tsched.h>
#include <thread.h>

#define KSTACK_SIZE ((MMU_VADDR_TKSTACK_END - MMU_VADDR_TKSTACK_START) + 1)

/* Linker sets for thread constructors and destructors. */
SET_DECLARE(thread_ctors, void);
SET_DECLARE(thread_dtors, void);
SET_DECLARE(thread_fork_handlers, void);

/* Linker sets for pre- and post-scheduling tasks */
SET_DECLARE(pre_sched_tasks, void);
SET_DECLARE(post_sched_tasks, void);
SET_DECLARE(sched_idle_tasks, void);


static void thread_set_inheritance(threadInfo_t * new_child,
                                   threadInfo_t * parent);
static void thread_init_kstack(threadInfo_t * tp);
static void thread_free_kstack(threadInfo_t * tp);


void sched_handler(void)
{
    threadInfo_t * const prev_thread = current_thread;
    void ** task_p;

    if (!current_thread) {
        current_thread = sched_get_thread_info(0);
        if (!current_thread)
            panic("No thread 0\n");
    }

    proc_update_times();

    /* Pre-scheduling tasks */
    SET_FOREACH(task_p, pre_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }

    /*
     * Call the actual context switcher function that schedules the next thread.
     */
    sched_schedule();
    if (current_thread != prev_thread) {
#ifdef configSCHED_DEBUG
        char buf[80];
        ksprintf(buf, sizeof(buf), "%x\n", current_thread->kstack_region);
        KERROR(KERROR_DEBUG, buf);
#endif
        mmu_map_region(&(current_thread->kstack_region->b_mmu));
    }

    /* Post-scheduling tasks */
    SET_FOREACH(task_p, post_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }
}

/**
 * Enter kernel mode.
 */
void thread_enter_kernel(void)
{
    current_thread->curr_mpt = &mmu_pagetable_master;
}

/**
 * Exit from kernel mode.
 */
mmu_pagetable_t * thread_exit_kernel(void)
{
    KASSERT(current_thread->curr_mpt != NULL, "curr_mpt must be set");

    current_thread->curr_mpt = &curproc->mm.mpt;
    return current_thread->curr_mpt;
}

/**
 * Suspend thread, enter scheduler.
 */
void thread_suspend(void)
{
}

/**
 * Resume threa from scheduler.
 */
mmu_pagetable_t * thread_resume(void)
{
    KASSERT(current_thread->curr_mpt != NULL, "curr_mpt must be set");

    return current_thread->curr_mpt;
}

/**
 * Kernel idle thread
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 */
void * idle_thread(/*@unused@*/ void * arg)
{
    void ** task_p;

    while(1) {
        /* Execute idle tasks */
        SET_FOREACH(task_p, sched_idle_tasks) {
            sched_task_t task = *(sched_task_t *)task_p;
            task();
        }

        idle_sleep();
    }
}

pthread_t thread_create(struct _ds_pthread_create * thread_def, int priv)
{
    const pthread_t tid = sched_new_tid();
    struct thread_info * tp = sched_get_thread_info(tid);

    if (tid < 0 || !tp)
        return -1;

    thread_init(tp,
                tid,                    /* Index of the thread created */
                thread_def,             /* Thread definition. */
                current_thread,         /* Pointer to the parent thread. */
                priv);                  /* kworker flag. */

    return tid;
}

void thread_init(struct thread_info * tp, pthread_t thread_id,
                 struct _ds_pthread_create * thread_def, threadInfo_t * parent,
                 int priv)
{
    /* This function should not be called for an already initialized thread. */
    if (tp->flags & SCHED_IN_USE_FLAG)
        panic("Can't init thread that is already in use.\n");

#ifdef configSCHED_TINY
    memset(tp, 0, sizeof(struct thread_info));
#endif

    /* Return thread id */
    if (thread_def->thread)
        *(thread_def->thread) = thread_id;

    /* Init core specific stack frame for user space */
    init_stack_frame(thread_def, &(tp->sframe[SCHED_SFRAME_SYS]),
            priv);

    /* Mark this thread index as used.
     * EXEC flag is set later in sched_thread_set_exec */
    tp->flags   = (uint32_t)SCHED_IN_USE_FLAG;
    tp->id      = thread_id;
    tp->niceval = thread_def->def->tpriority;

    if (priv) {
        /* Just that user can see that this is a kworker, no functional
         * difference other than privileged mode. */
         tp->flags |= SCHED_KWORKER_FLAG;
    }

    /* Clear signal flags & wait states */
    tp->a_wait_count    = ATOMIC_INIT(0);
    tp->wait_tim        = -1;

    /* Update parent and child pointers */
    thread_set_inheritance(tp, parent);

    /* So errno is at the last address of stack area.
     * Note that this should also agree with core specific
     * init_stack_frame() function. */
    tp->errno_uaddr = (void *)((uintptr_t)(thread_def->def->stackAddr)
            + thread_def->def->stackSize
            - sizeof(errno_t));

    /* Create kstack */
    thread_init_kstack(tp);

    /* Select master page table used on startup */
    if (!parent) {
        tp->curr_mpt = &mmu_pagetable_master;
    } else {
        proc_info_t * proc;

        proc = proc_get_struct_l(parent->pid_owner);
        if (!proc)
            panic("Owner must exist");

        tp->curr_mpt = &proc->mm.mpt;
    }

    /* Call thread constructors */
    void ** thread_ctor_p;
    SET_FOREACH(thread_ctor_p, thread_ctors) {
        thread_cdtor_t ctor = *(thread_cdtor_t *)thread_ctor_p;
        ctor(tp);
    }

    /* Put thread into execution */
    sched_thread_set_exec(tp->id);
}

/**
 * Set thread inheritance
 * Sets linking from the parent thread to the thread id.
 */
static void thread_set_inheritance(threadInfo_t * new_child,
                                   threadInfo_t * parent)
{
    threadInfo_t * last_node;
    threadInfo_t * tmp;

    /* Initial values for all threads */
    new_child->inh.parent = parent;
    new_child->inh.first_child = NULL;
    new_child->inh.next_child = NULL;

    if (parent == NULL) {
        new_child->pid_owner = 0;
        return;
    }
    new_child->pid_owner = parent->pid_owner;

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = new_child;
        new_child->inh.next_child = NULL;

        return; /* All done */
    }

    /* Find the last child thread
     * Assuming first_child is a valid thread pointer
     */
    tmp = (threadInfo_t *)(parent->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as the last child in chain. */
    last_node->inh.next_child = new_child;
}

void thread_set_current_insys(int s)
{
    if (s)
        current_thread->flags |= SCHED_INSYS_FLAG;
    else
        current_thread->flags &= ~SCHED_INSYS_FLAG;
}

pthread_t thread_fork(void)
{
    struct thread_info * const old_thread = current_thread;
    struct thread_info * new_thread;
    threadInfo_t tmp;
    pthread_t new_id;
    void ** fork_handler_p;

#ifdef configSCHED_DEBUG
    KASSERT(old_thread, "current_thread not set\n");
#endif

    /* Get next free thread_id */
    new_id = sched_new_tid();
    if (new_id < 0) {
        return -ENOMEM;
    }

    /* New thread is kept in tmp until it's ready for execution. */
    memcpy(&tmp, old_thread, sizeof(threadInfo_t));
    tmp.flags &= ~SCHED_EXEC_FLAG; /* Disable exec for now. */
    tmp.flags &= ~SCHED_INSYS_FLAG;
    tmp.id = new_id;

    thread_set_inheritance(&tmp, old_thread);

    memcpy(&tmp.sframe[SCHED_SFRAME_SYS], &old_thread->sframe[SCHED_SFRAME_SVC],
            sizeof(sw_stack_frame_t));

    SET_FOREACH(fork_handler_p, thread_fork_handlers) {
        sched_task_t task = *(thread_cdtor_t *)fork_handler_p;
        task(&tmp);
    }

    new_thread = sched_get_thread_info(new_id);
    if (!new_thread)
        panic("Failed to get newly created thread struct\n");

    memcpy(new_thread, &tmp, sizeof(struct thread_info));
    thread_init_kstack(new_thread);

    /* TODO Increment resource refcounters(?) */

    return new_id;
}

void thread_wait(void)
{
    atomic_inc(&current_thread->a_wait_count);
    sched_sleep_current_thread(0);
}

void thread_release(threadInfo_t * thread)
{
    int old_val;

    old_val = atomic_dec(&thread->a_wait_count);

    if (old_val == 0) {
        atomic_inc(&thread->a_wait_count);
        old_val = 1;
    }

    if (old_val == 1) {
        thread->flags &= ~SCHED_WAIT_FLAG;
        sched_thread_set_exec(thread->id);
    }
}

static void thread_event_timer(void * event_arg)
{
    threadInfo_t * thread = (threadInfo_t *)event_arg;

    timers_release(thread->wait_tim);
    thread->wait_tim = -1;

    thread_release(thread);
}

void thread_sleep(long millisec)
{
    int timer_id;

    do {
        timer_id = timers_add(thread_event_timer, current_thread,
            TIMERS_FLAG_ONESHOT, millisec * 1000);
    } while (timer_id < 0);
    current_thread->wait_tim = timer_id;

    /* This should prevent anyone from waking up this thread for a while. */
    timers_start(timer_id);
    thread_wait();
}

/**
 * Initialize thread kernel mode stack.
 * @param tp is a pointer to the thread.
 */
static void thread_init_kstack(threadInfo_t * tp)
{
    struct buf * kstack;

#ifdef configSCHED_DEBUG
    KASSERT(tp, "tp not set\n");
#endif

    /* Create a kstack */
    kstack = geteblk(KSTACK_SIZE);
    if (!kstack) {
        panic("OOM during thread creation\n");
    }

    kstack->b_uflags    = 0;
    kstack->b_mmu.vaddr = MMU_VADDR_TKSTACK_START;
    kstack->b_mmu.pt    = &mmu_pagetable_system;

    tp->kstack_region = kstack;
}

static void thread_free_kstack(threadInfo_t * tp)
{
    tp->kstack_region->vm_ops->rfree(tp->kstack_region);
}

pthread_t get_current_tid(void)
{
    if (current_thread)
        return (pthread_t)(current_thread->id);
    return 0;
}

void * thread_get_curr_stackframe(size_t ind)
{
    if (current_thread && (ind < SCHED_SFRAME_ARR_SIZE))
        return &(current_thread->sframe[ind]);
    return NULL;
}

int thread_set_priority(pthread_t thread_id, int priority)
{
    struct thread_info * tp = sched_get_thread_info(thread_id);

    if (!tp || (tp->flags & SCHED_IN_USE_FLAG) == 0) {
        return -ESRCH;
    }

    tp->niceval = priority;

    return 0;
}

int thread_get_priority(pthread_t thread_id)
{
    struct thread_info * tp = sched_get_thread_info(thread_id);

    if (!tp || (tp->flags & SCHED_IN_USE_FLAG) == 0) {
        return NICE_ERR;
    }

    return tp->niceval;
}

void thread_die(intptr_t retval)
{
    istate_t s;

    /* TODO This part is not quite clever nor MP safe. */
    s = get_interrupt_state();
    disable_interrupt();

    current_thread->retval = retval;
    current_thread->flags |= SCHED_ZOMBIE_FLAG;

    set_interrupt_state(s);

    sched_sleep_current_thread(SCHED_PERMASLEEP);
}

/* TODO Might be unsafe to call from multiple threads for the same tree! */
int thread_terminate(pthread_t thread_id)
{
    threadInfo_t * thread = sched_get_thread_info(thread_id);
    threadInfo_t * child;
    threadInfo_t * prev_child;

    if (!thread || !SCHED_TEST_TERMINATE_OK(thread->flags)) {
        return -EPERM;
    }

    /* Remove all child threads from execution */
    child = thread->inh.first_child;
    prev_child = NULL;
    if (child != NULL) {
        do {
            if (thread_terminate(child->id) != 0) {
                child->inh.parent = 0; /* Child is now orphan, it was
                                        * probably a kworker that couldn't be
                                        * killed. */
            }

            /* Fix child list */
            if (child->flags &&
                    (((threadInfo_t *)(thread->inh.first_child))->flags == 0)) {
                thread->inh.first_child = child;
                prev_child = child;
            } else if (child->flags && prev_child) {
                prev_child->inh.next_child = child;
                prev_child = child;
            } else if (child->flags) {
                prev_child = child;
            }
        } while ((child = child->inh.next_child) != NULL);
    }

    /*
     * We set this thread as a zombie. If detach is also set then
     * sched_thread_remove() will remove this thread immediately but usually
     * it's not, then it will release some resources but left the thread
     * struct mostly intact.
     */
    thread->flags |= SCHED_ZOMBIE_FLAG;
    thread->flags &= ~SCHED_EXEC_FLAG;

    /* Remove thread completely if it is a detached zombie, its parent is a
     * detached zombie thread or the thread is parentles for any reason.
     * Otherwise we left the thread struct intact for now.
     */
    if (SCHED_TEST_DETACHED_ZOMBIE(thread->flags) ||
            (thread->inh.parent == 0)             ||
            ((thread->inh.parent != 0)            &&
            SCHED_TEST_DETACHED_ZOMBIE(((threadInfo_t *)(thread->inh.parent))->flags))) {

        /* Release wait timeout timer */
        if (thread->wait_tim >= 0) {
            timers_release(thread->wait_tim);
        }

        /* Notify the owner process about removal of a thread. */
        if (thread->pid_owner != 0) {
            proc_thread_removed(thread->pid_owner, thread_id);
        }

        /* Call thread destructors */
        void ** thread_dtor_p;
        SET_FOREACH(thread_dtor_p, thread_dtors) {
            thread_cdtor_t dtor = *(thread_cdtor_t *)thread_dtor_p;
            if (dtor)
                dtor(thread);
        }

        thread_free_kstack(thread);
        sched_thread_remove(thread_id);
    }

    return 0;
}

static void dummycd(struct thread_info * th)
{
}
DATA_SET(thread_ctors, dummycd);
DATA_SET(thread_dtors, dummycd);

/* Syscalls */

static int sys_thread_create(void * user_args)
{
    struct _ds_pthread_create args;
    pthread_attr_t thdef;
    pthread_t thread_id;
    pthread_t * usr_thread_id;
    int err;

    if (!useracc(user_args, sizeof(args), VM_PROT_WRITE)) {
        /* No permission to read/write */
        set_errno(EFAULT);
        return -1;
    }

    copyin(user_args, &args, sizeof(args));
    err = copyin(args.def, (void *)(&thdef), sizeof(thdef));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    usr_thread_id = args.thread;
    if (usr_thread_id) {
        if (!useracc(usr_thread_id, sizeof(pthread_t), VM_PROT_WRITE)) {
            set_errno(EFAULT);
            return -1;
        }
    }
    args.thread = &thread_id;

    thread_create(&args, 0);

    copyout(&args, user_args, sizeof(args));
    if (usr_thread_id)
        copyout(&thread_id, usr_thread_id, sizeof(thread_id));
    return 0;
}

static int sys_thread_terminate(void * user_args)
{
    pthread_t thread_id;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return thread_terminate(thread_id);
}

static int sys_thread_sleep_ms(void * user_args)
{
    uint32_t val;
    int err;

    err = copyin(user_args, &val, sizeof(uint32_t));
    if (err) {
        set_errno(EFAULT);
        return -EFAULT;
    }

    thread_sleep(val);

    return 0; /* TODO Return value might be incorrect */
}

static int sys_get_current_tid(void * user_args)
{
    return (int)get_current_tid();
}

/**
 * Get address of thread errno.
 */
static intptr_t sys_geterrno(void * user_args)
{
    return (intptr_t)current_thread->errno_uaddr;
}

static int sys_thread_die(void * user_args)
{
    thread_die((intptr_t)user_args);

    /* Does not return */
    return 0;
}

static int sys_thread_detach(void * user_args)
{
    pthread_t thread_id;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if ((uintptr_t)sched_thread_detach(thread_id)) {
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static int sys_thread_setpriority(void * user_args)
{
    int err;
    struct _ds_set_priority args;

    err = priv_check(curproc, PRIV_SCHED_SETPRIORITY);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(ESRCH);
        return -1;
    }

    err = (uintptr_t)thread_set_priority(args.thread_id, args.priority);
    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static int sys_thread_getpriority(void * user_args)
{
    int pri;
    pthread_t thread_id;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(ESRCH);
        return -1;
    }

    pri = (uintptr_t)thread_get_priority(thread_id);
    if (pri == NICE_ERR) {
        set_errno(ESRCH);
        pri = -1; /* Note: -1 might be also legitimate prio value. */
    }

    return pri;
}

static const syscall_handler_t thread_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_CREATE, sys_thread_create),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_TERMINATE, sys_thread_terminate),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_SLEEP_MS, sys_thread_sleep_ms),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_GETTID, sys_get_current_tid),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_GETERRNO, sys_geterrno),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_DIE, sys_thread_die),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_DETACH, sys_thread_detach),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_SETPRIORITY, sys_thread_setpriority),
    ARRDECL_SYSCALL_HNDL(SYSCALL_THREAD_GETPRIORITY, sys_thread_getpriority)
};
SYSCALL_HANDLERDEF(thread_syscall, thread_sysfnmap)
