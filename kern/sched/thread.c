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

#include <machine/atomic.h>
#include <sys/linker_set.h>
#include <errno.h>
#include <syscall.h>
#include <libkern.h>
#include <kstring.h>
#include <ptmapper.h>
#include <kmalloc.h>
#include <buf.h>
#include <kerror.h>
#include <timers.h>
#include <proc.h>
#include <ksched.h>

#define KSTACK_SIZE ((MMU_VADDR_TKSTACK_END - MMU_VADDR_TKSTACK_START) + 1)

/* Linker sets for thread constructors and destructors. */
SET_DECLARE(thread_ctors, void);
SET_DECLARE(thread_dtors, void);
SET_DECLARE(thread_fork_handlers, void);

/**
 * Next thread id.
 */
static atomic_t next_thread_id = ATOMIC_INIT(0);

struct cpu_threads {
    RB_HEAD(threadmap, thread_info) threadmap_head;
    STAILQ_HEAD(readyq_head, thread_info) readyq;
    mtx_t lock;
};

static struct cpu_threads cpu[1];
#define CURRENT_CPU (&cpu[0])

static void init_sched_data(struct sched_thread_data * data);
static void thread_set_inheritance(struct thread_info * child,
                                   struct thread_info * parent);
static void thread_init_kstack(struct thread_info * tp);
static void thread_free_kstack(struct thread_info * tp);

RB_PROTOTYPE_STATIC(threadmap, thread_info, sched.ttentry_, thread_id_compare);
RB_GENERATE_STATIC(threadmap, thread_info, sched.ttentry_, thread_id_compare);

/**
 * Initialize thread storage.
 */
void _thread_sys_init(void)
{
    for (size_t i = 0; i < num_elem(cpu); i++) {
        mtx_init(&cpu[0].lock, MTX_TYPE_SPIN, MTX_OPT_DINT);
        RB_INIT(&cpu[0].threadmap_head);
        STAILQ_INIT(&cpu[0].readyq);
    }
}

int thread_id_compare(struct thread_info * a, struct thread_info * b)
{
    KASSERT(a && b, "a & b must be set");

    return a->id - b->id;
}

/**
 * Enter kernel mode.
 * Called by interrupt handler.
 */
void _thread_enter_kernel(void)
{
    current_thread->curr_mpt = &mmu_pagetable_master;
}

/**
 * Exit from kernel mode.
 * Called by interrupt handler.
 */
mmu_pagetable_t * _thread_exit_kernel(void)
{
    KASSERT(current_thread->curr_mpt != NULL, "curr_mpt must be set");

    current_thread->curr_mpt = &curproc->mm.mpt;
    return current_thread->curr_mpt;
}

/**
 * Suspend thread, enter scheduler.
 * Called by interrupt handler.
 */
void _thread_suspend(void)
{
    /* NOP */
}

/**
 * Resume thread from scheduler.
 * Called by interrupt handler
 */
mmu_pagetable_t * _thread_resume(void)
{
    KASSERT(current_thread->curr_mpt != NULL, "curr_mpt must be set");

    return current_thread->curr_mpt;
}

/**
 * Set thread initial configuration.
 * @note This function should not be called for already initialized threads.
 * @param tp           is a pointer to the thread struct.
 * @param thread_id    Thread id
 * @param thread_def   Thread definitions
 * @param parent       Parent thread id, NULL = doesn't have a parent
 * @param priv         If set thread is initialized as a kernel mode thread
 *                     (kworker).
 * @todo what if parent is stopped before this function is called?
 */
static void thread_init(struct thread_info * tp, pthread_t thread_id,
                        struct _sched_pthread_create_args * thread_def,
                        struct thread_info * parent,
                        int priv)
{
    /* Init core specific stack frame for user space */
    init_stack_frame(thread_def, &(tp->sframe[SCHED_SFRAME_SYS]), priv);

    /*
     * Mark this thread as used.
     */
    tp->id      = thread_id;
    tp->flags   = SCHED_IN_USE_FLAG;
    tp->param   = thread_def->param;
    init_sched_data(&tp->sched);

     mtx_lock(&CURRENT_CPU->lock);
     RB_INSERT(threadmap, &CURRENT_CPU->threadmap_head, tp);
     mtx_unlock(&CURRENT_CPU->lock);

    if (thread_def->flags & PTHREAD_CREATE_DETACHED) {
        tp->flags |= SCHED_DETACH_FLAG;
    }

    if (priv) {
        /*
         * Just that user can see that this is a kworker, no functional
         * difference other than privileged mode.
         */
         tp->flags |= SCHED_KWORKER_FLAG;
    }

    tp->wait_tim        = TMNOVAL;

    /* Update parent and child pointers */
    thread_set_inheritance(tp, parent);

    /*
     * So errno is at the last address of stack area.
     * Note that this should also agree with core specific
     * init_stack_frame() function.
     */
    tp->errno_uaddr     = (void *)((uintptr_t)(thread_def->stack_addr)
                          + thread_def->stack_size
                          - sizeof(errno_t));

    /* Create kstack. */
    thread_init_kstack(tp);

    /* Select master page table used on startup. */
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

    /* Put thread into readyq */
    if (thread_ready(tp->id)) {
        panic("Failed to make new_thread ready");
    }
}

pthread_t thread_create(struct _sched_pthread_create_args * thread_def,
                        int priv)
{
    const pthread_t tid = atomic_inc(&next_thread_id);
    struct thread_info * thread;

    thread = kcalloc(1, sizeof(struct thread_info));
    if (!thread)
        return -EAGAIN;

    thread_init(thread,
                tid,                    /* Index of the thread created */
                thread_def,             /* Thread definition. */
                current_thread,         /* Pointer to the parent thread. */
                priv);                  /* kworker flag. */

    return tid;
}

/**
 * Initialize a sched data structure.
 */
static void init_sched_data(struct sched_thread_data * data)
{
    memset(data, '\0', sizeof(struct sched_thread_data));
    mtx_init(&data->tdlock, MTX_TYPE_SPIN, MTX_OPT_DINT);
    data->state = THREAD_STATE_INIT;
}

/**
 * Set thread inheritance.
 * Sets linking from the parent thread to the thread id.
 */
static void thread_set_inheritance(struct thread_info * child,
                                   struct thread_info * parent)
{
    struct thread_info * last_node;
    struct thread_info * tmp;

    /* Initial values for all threads */
    child->inh.parent = parent;
    child->inh.first_child = NULL;
    child->inh.next_child = NULL;

    if (parent == NULL) {
        child->pid_owner = 0;
        return;
    }
    child->pid_owner = parent->pid_owner;

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = child;
        child->inh.next_child = NULL;

        return; /* done */
    }

    /*
     * Find the last child thread.
     * Assuming first_child is a valid thread pointer.
     */
    tmp = parent->inh.first_child;
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as the last child in chain. */
    last_node->inh.next_child = child;
}

/**
 * Initialize thread kernel mode stack.
 * @param tp is a pointer to the thread.
 */
static void thread_init_kstack(struct thread_info * tp)
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
    kstack->b_mmu.control |= MMU_CTRL_XN;

    tp->kstack_region = kstack;
}

/**
 * Free thread kstack.
 */
static void thread_free_kstack(struct thread_info * tp)
{
    tp->kstack_region->vm_ops->rfree(tp->kstack_region);
}

struct thread_info * thread_lookup(pthread_t thread_id)
{
    struct thread_info * thread = NULL;
    struct thread_info find = { .id = thread_id };

    mtx_lock(&CURRENT_CPU->lock);
    if (!RB_EMPTY(&CURRENT_CPU->threadmap_head)) {
        thread = RB_FIND(threadmap, &CURRENT_CPU->threadmap_head, &find);
    }
    mtx_unlock(&CURRENT_CPU->lock);

    return thread;
}

pthread_t thread_fork(void)
{
    struct thread_info * const old_thread = current_thread;
    struct thread_info * new_thread;
    pthread_t new_id;
    void ** fork_handler_p;

#ifdef configSCHED_DEBUG
    KASSERT(old_thread, "current_thread not set\n");
#endif

    /* Get next free thread_id. */
    new_id = atomic_inc(&next_thread_id);
    if (new_id < 0)
        panic("Out of thread IDs");

    new_thread = kcalloc(1, sizeof(struct thread_info));
    if (!new_thread)
        return -EAGAIN;

    memcpy(new_thread, old_thread, sizeof(struct thread_info));
    new_thread->id       = new_id;
    new_thread->flags   &= ~SCHED_INSYS_FLAG;
    init_sched_data(&new_thread->sched);
    thread_set_inheritance(new_thread, old_thread);

    mtx_lock(&CURRENT_CPU->lock);
    RB_INSERT(threadmap, &CURRENT_CPU->threadmap_head, new_thread);
    mtx_unlock(&CURRENT_CPU->lock);

    /*
     * We wan't to return directly to the user space.
     */
    memcpy(&new_thread->sframe[SCHED_SFRAME_SYS],
           &old_thread->sframe[SCHED_SFRAME_SVC],
           sizeof(sw_stack_frame_t));

    SET_FOREACH(fork_handler_p, thread_fork_handlers) {
        sched_task_t task = *(thread_cdtor_t *)fork_handler_p;
        task(new_thread);
    }

    thread_init_kstack(new_thread);

    /* TODO Increment resource refcounters(?) */

    /* The newly created thread shall remain in init state for now. */
    return new_id;
}

int thread_ready(pthread_t thread_id)
{
    struct thread_info * thread = thread_lookup(thread_id);
    enum thread_state prev_state;

    if (!thread || thread_state_get(thread) == THREAD_STATE_DEAD)
        return -ESRCH;

    prev_state = thread_state_set(thread, THREAD_STATE_READY);
    if (prev_state == THREAD_STATE_READY) {
        return 0; /* TODO ? */
    }

    mtx_lock(&CURRENT_CPU->lock);
    STAILQ_INSERT_TAIL(&CURRENT_CPU->readyq, thread, sched.readyq_entry_);
    mtx_unlock(&CURRENT_CPU->lock);

    return 0;
}

struct thread_info * thread_remove_ready(void)
{
    struct thread_info * thread;

    mtx_lock(&CURRENT_CPU->lock);
    if (STAILQ_EMPTY(&CURRENT_CPU->readyq)) {
        mtx_unlock(&CURRENT_CPU->lock);
        return NULL;
    }

    thread = STAILQ_FIRST(&CURRENT_CPU->readyq);
    STAILQ_REMOVE_HEAD(&CURRENT_CPU->readyq, sched.readyq_entry_);

    mtx_unlock(&CURRENT_CPU->lock);
    return thread;
}

void thread_wait(void)
{
    thread_state_set(current_thread, THREAD_STATE_BLOCKED);

    /*
     * Make sure we don't get stuck here.
     * This is mainly here to handle race conditions in exec().
     */
    enable_interrupt();

    while (thread_state_get(current_thread) != THREAD_STATE_EXEC) {
        idle_sleep();
    }
}

void thread_release(pthread_t thread_id)
{
    thread_ready(thread_id);
}

static void thread_event_timer(void * event_arg)
{
    struct thread_info * thread = (struct thread_info *)event_arg;

    timers_release(thread->wait_tim);
    thread->wait_tim = TMNOVAL;

    thread_release(thread->id);
}

void thread_sleep(long millisec)
{
    int timer_id;

    do {
        timer_id = timers_add(thread_event_timer, current_thread,
            TIMERS_FLAG_ONESHOT, millisec * 1000);
    } while (timer_id < 0);
    current_thread->wait_tim = timer_id;

    timers_start(timer_id);
    thread_wait();
}

void thread_yield(enum thread_eyield_strategy strategy)
{
    KASSERT(current_thread, "Current thread must be set");

    thread_ready(current_thread->id);
    if (strategy == THREAD_YIELD_IMMEDIATE)
        idle_sleep();

    /*
     * TODO
     * User may expect this function to yield immediately which doesn't actually
     * happen.
     */
}

void thread_die(intptr_t retval)
{
    current_thread->retval = retval;
    thread_terminate(current_thread->id);
    thread_wait();
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

int thread_set_policy(pthread_t thread_id, unsigned policy)
{
    struct thread_info * thread = thread_lookup(thread_id);

    if (!thread || thread_flags_not_set(thread, SCHED_IN_USE_FLAG))
        return -ESRCH;

    if (policy > SCHED_OTHER)
        return -EINVAL;

    thread->param.sched_policy = policy;

    return 0;
}

unsigned thread_get_policy(pthread_t thread_id)
{
    struct thread_info * thread = thread_lookup(thread_id);

    if (!thread || thread_flags_not_set(thread, SCHED_IN_USE_FLAG))
        return -ESRCH;

    return thread->param.sched_policy;
}

int thread_set_priority(pthread_t thread_id, int priority)
{
    struct thread_info * thread = thread_lookup(thread_id);

    if (!thread || thread_flags_not_set(thread, SCHED_IN_USE_FLAG))
        return -ESRCH;

    thread->param.sched_priority = priority;

    return 0;
}

int thread_get_priority(pthread_t thread_id)
{
    struct thread_info * thread = thread_lookup(thread_id);

    if (!thread || thread_flags_not_set(thread, SCHED_IN_USE_FLAG))
        return NICE_ERR;

    return thread->param.sched_priority;
}

int thread_terminate(pthread_t thread_id)
{
    struct thread_info * thread = thread_lookup(thread_id);
    struct thread_info * child;
    struct thread_info * next_child;

    if (!thread)
        return -EINVAL;

    if (!SCHED_TEST_TERMINATE_OK(thread_flags_get(thread)))
        return -EPERM;

    /* Remove all child threads from execution */
    child = thread->inh.first_child;
    while (child) {
        next_child = child->inh.next_child;

        if (thread_terminate(child->id) == -EPERM) {
            /*
             * The child is now orphan, it was probably a kworker that couldn't
             * be killed.
             */
            child->inh.parent = NULL;
            child->inh.next_child = NULL;
        }

        thread->inh.first_child = next_child;
        child = next_child;
    }

    thread_state_set(thread, THREAD_STATE_DEAD);

    return 0;
}

void thread_remove(pthread_t thread_id)
{
    struct thread_info * thread = thread_lookup(thread_id);
    void ** thread_dtor_p;

    if (thread_flags_not_set(thread, SCHED_IN_USE_FLAG))
        return; /* Already freed */

    thread->flags = 0; /* Clear all flags */
    thread->param.sched_priority = NICE_ERR;

    /* Release wait timeout timer */
    if (thread->wait_tim >= 0) {
        timers_release(thread->wait_tim);
    }

    /* Notify the owner process about removal of a thread. */
    if (thread->pid_owner != 0) {
        proc_thread_removed(thread->pid_owner, thread_id);
    }

    /* Call thread destructors */
    SET_FOREACH(thread_dtor_p, thread_dtors) {
        thread_cdtor_t dtor = *(thread_cdtor_t *)thread_dtor_p;
        if (dtor)
            dtor(thread);
    }

    thread_free_kstack(thread);

    mtx_lock(&CURRENT_CPU->lock);
    RB_REMOVE(threadmap, &CURRENT_CPU->threadmap_head, thread);
    mtx_unlock(&CURRENT_CPU->lock);

    kfree(thread);
}

static void dummycd(struct thread_info * th)
{
}
DATA_SET(thread_ctors, dummycd);
DATA_SET(thread_dtors, dummycd);

/* Syscalls */

static int sys_thread_create(void * user_args)
{
    struct _sched_pthread_create_args args;
    pthread_t tid;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.stack_size < 40) {
        set_errno(EINVAL);
        return -1;
    }

    if (!useracc(args.stack_addr, args.stack_size, VM_PROT_WRITE)) {
        set_errno(EINVAL);
        return -1;
    }

    if (!useracc(args.start, sizeof(void *), VM_PROT_READ | VM_PROT_EXECUTE)) {
        set_errno(EINVAL);
        return -1;
    }

    /* TODO Check policy */

    tid = thread_create(&args, 0);
    if (tid < 0) {
        set_errno(-tid);
        return -1;
    }
    return tid;
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
 * Get the address of thread errno.
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

/**
 * TODO Not completely thread safe.
 */
static int sys_thread_detach(void * user_args)
{
    pthread_t thread_id;
    struct thread_info * thread;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    thread = thread_lookup(thread_id);
    if (!thread) {
        set_errno(EINVAL);
        return -1;
    }

    thread_flags_set(thread, SCHED_DETACH_FLAG);

    return 0;
}

static int sys_thread_setpriority(void * user_args)
{
    int err;
    struct _sched_set_priority_args args;

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

    if (args.priority < 0 && curproc->euid != 0) {
        set_errno(EPERM);
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
    int prio;
    pthread_t thread_id;
    int err;

    err = copyin(user_args, &thread_id, sizeof(pthread_t));
    if (err) {
        set_errno(ESRCH);
        return -1;
    }

    prio = (uintptr_t)thread_get_priority(thread_id);
    if (prio == NICE_ERR) {
        set_errno(ESRCH);
        prio = -1; /* Note: -1 might be also legitimate prio value. */
    }

    return prio;
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
