/**
 *******************************************************************************
 * @file    exec.c
 * @author  Olli Vanhoja
 * @brief   Execute a file.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/priv.h>
#include <sys/sysctl.h>
#include <syscall.h>
#include <unistd.h>
#include <buf.h>
#include <exec.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <thread.h>

SET_DECLARE(exec_loader, struct exec_loadfn);

SYSCTL_INT(_kern, KERN_ARGMAX, argmax, CTLFLAG_RD, 0, MMU_PGSIZE_COARSE,
           "Max args to exec");

static int main_stack_dfl = configPROC_STACK_DFL;
SYSCTL_INT(_kern, KERN_DFLSIZ, dflsiz, CTLFLAG_RW,
           &main_stack_dfl, 0, "Default main() stack size");

static int main_stack_max = 2 * configPROC_STACK_DFL;
SYSCTL_INT(_kern, KERN_MAXSIZ, maxsiz, CTLFLAG_RW,
           &main_stack_max, 0, "Max main() stack size");

/**
 * Calculate the size of a new stack allocation for main().
 * @param emin is the required stack size idicated by the executable.
 */
static size_t get_new_main_stack_size(ssize_t emin)
{
    const ssize_t kmin = main_stack_dfl;
    const ssize_t kmax = main_stack_max;
    const ssize_t rlim = curproc->rlim[RLIMIT_STACK].rlim_cur;
    ssize_t dmin, dmax;

    dmin = (emin > 0 && emin > kmin) ? emin : kmin;
    dmax = (rlim > 0 && rlim < kmax) ? rlim : kmax;

    return memalign_size(min(dmin, dmax), MMU_PGSIZE_COARSE);
}

/**
 * Create a new thread for executing main()
 * @param stack_size is the preferred stack size;
 *                   0 if the system default shall be used.
 */
static pthread_t new_main_thread(int uargc, uintptr_t uargv, uintptr_t uenvp,
                                 size_t stack_size)
{
    struct buf * stack_region;
    struct buf * code_region = (*curproc->mm.regions)[MM_CODE_REGION];
    struct _sched_pthread_create_args args;

    stack_size = get_new_main_stack_size(stack_size);
    stack_region = vm_new_userstack_curproc(stack_size);
    if (!stack_region)
        return -ENOMEM;

    args = (struct _sched_pthread_create_args){
        .param.sched_policy = current_thread->param.sched_policy,
        .param.sched_priority = current_thread->param.sched_priority,
        .stack_addr = (void *)(stack_region->b_mmu.vaddr),
        .stack_size = stack_region->b_bufsize,
        .flags      = PTHREAD_CREATE_DETACHED,
        .start      = (void *(*)(void *))(code_region->b_mmu.vaddr),
        .arg1       = uargc,
        .arg2       = uargv,
        .arg3       = uenvp,
        .arg4       = 0, /* Not used */
        .del_thread = NULL /* Not needed for main(). */
    };

    KASSERT(args.stack_size > 0,
            "Size of the main stack must be greater than zero\n");

    return thread_create(&args, THREAD_MODE_USER);
}

int exec_file(struct exec_loadfn * loader, int fildes,
              char name[PROC_NAME_SIZE], struct buf * env_bp,
              int uargc, uintptr_t uargv, uintptr_t uenvp)
{
    file_t * file;
    uintptr_t vaddr = 0; /* RFE Shouldn't matter if elf is not dyn? */
    size_t stack_size;
    pthread_t tid;
    int err;

    KERROR_DBG("exec_file(loader \"%s\", fildes %d, name \"%s\", "
               "env_bp %p, uargc %d, uargv %x,  uenvp %x)\n",
               (loader) ? loader->name : "NULL", fildes, name, env_bp, uargc,
               (uint32_t)uargv, (uint32_t)uenvp);

    file = fs_fildes_ref(curproc->files, fildes, 1);
    if (!file) {
        err = -EBADF;
        goto fail;
    }

    if (!S_ISREG(file->vnode->vn_mode)) {
        fs_fildes_ref(curproc->files, fildes, -1);
        err = -ENOEXEC;
        goto fail;
    }

    /* Unload user regions before loading a new image. */
    (void)vm_unload_regions(curproc, MM_HEAP_REGION, -1);

    /*
     * Do what is necessary on exec here as the loader might need to alter the
     * capabilities and it could be an unexpected result if whatever the loader
     * does would be overriden.
     */
    priv_cred_init_exec(&curproc->cred);

    /* Load the image */
    err = loader->load(curproc, file, &vaddr, &stack_size);
    KERROR_DBG("Proc image loaded (err = %d)\n", err);
    if (err) {
        const struct ksignal_param sigparm = { .si_code = SEGV_MAPERR };

        fs_fildes_ref(curproc->files, fildes, -1);
        ksignal_sendsig_fatal(curproc, SIGSEGV, &sigparm);

        goto fail;
    }

    /*
     * Close the executable file.
     */
    err = fs_fildes_close(curproc, fildes);
    if (err) {
        KERROR_DBG("failed to close the file\n");
        goto fail;
    }

    /* Map new environment */
    err = vm_insert_region(curproc, env_bp, VM_INSOP_MAP_REG);
    if (err < 0) {
        KERROR_DBG("Unable to map a new env\n");
        goto fail;
    }
    vm_fixmemmap_proc(curproc);

    KERROR_DBG("Memory mapping done (pid = %d)\n", curproc->pid);

    /* Close CLOEXEC files */
    fs_fildes_close_exec(curproc);

    /* Change proc name */
    strlcpy(curproc->name, name, sizeof(curproc->name));
    KERROR_DBG("New name \"%s\" set for PID %d\n", curproc->name, curproc->pid);

    /* Create a new main() thread */
    tid = new_main_thread(uargc - 1, uargv, uenvp, stack_size);
    if (tid <= 0) {
        const struct ksignal_param sigparm = {
           .si_code = SI_USER,
        };

        KERROR_DBG("Failed to create a new main() (%d)\n", tid);

        ksignal_sendsig_fatal(curproc, SIGKILL, &sigparm);
    }

    KERROR_DBG("Changing main()\n");

    err = 0;
fail:
    if (err == 0) {
        /* Detach in case the current thread wasn't detached. */
        thread_flags_set(current_thread, SCHED_DETACH_FLAG);

        /*
         * Mark the old main thread for deletion, it's up to user space to kill
         * any children. However, if there are any child threads those may or
         * may not cause a segmentation fault depending on when the scheduler
         * starts removing stuff. This decision was made because we want to
         * keep disable_interrupt() time as short as possible and POSIX seems
         * to be quite silent about this issue anyway.
         */
        disable_interrupt();
        current_thread->inh.first_child = NULL;
        current_thread->inh.parent = NULL;
        curproc->main_thread = thread_lookup(tid);

        /*
         * Don't return but die as the calling user space is wiped and this
         * thread shouldn't exist anymore.
         */
        thread_die(0);
    }

    return err;
}

/**
 * Clone an array of strings and remap to a new section mapped in user space.
 * @note vaddr must be set to the final value by the caller.
 */
static int clone_aa(struct buf * bp, __user char * uarr, size_t n_entries,
                    size_t * doffset)
{
    char ** arg = (char **)(bp->b_data + *doffset);
    char * val  = (char *)(bp->b_data + *doffset);
    size_t bytesleft = bp->b_bcount - *doffset;
    size_t offset;
    int err;

    KERROR_DBG("%s(bp %p, uarr %p, n_entries %u, doffset %p)\n",
               __func__, bp, uarr, n_entries, doffset);

    if (n_entries == 0)
        return 0;

    if (bytesleft <= n_entries * sizeof(char *))
        return -ENOMEM;

    err = copyin(uarr, arg, n_entries * sizeof(char *));
    if (err) {
        KERROR_DBG("%s: Failed to copy the args array\n", __func__);
        return err;
    }

    offset = n_entries * sizeof(char *) + sizeof(char *);
    bytesleft -= offset;

    for (size_t i = 0; i < n_entries - 1; i++) {
        size_t copied;

        if (!arg[i])
            continue;

        err = copyinstr((__user char *)arg[i], val + offset, bytesleft,
                        &copied);
        if (err) {
            KERROR_DBG("%s: Failed to copy arg %i (%p)\n", __func__, i, arg[i]);
            return err;
        }

        KERROR_DBG("%s: arg[%i] = \"%s\"\n",
                   __func__, i, val + offset);

        /* new pointer from arg[i] to the string, valid in user space. */
        arg[i] = (char *)(bp->b_mmu.vaddr + *doffset + offset);

        offset += copied;
        bytesleft -= copied;
    }
    arg[n_entries - 1] = NULL;

    *doffset = offset;

    return 0;
}

/**
 * Get a pointer to the executable loader for a given file.
 */
static int get_loader(int fildes, struct exec_loadfn ** loader)
{
    file_t * file;
    struct exec_loadfn ** ldr;
    int err = -ENOEXEC;

    file = fs_fildes_ref(curproc->files, fildes, 1);
    SET_FOREACH(ldr, exec_loader) {
        err = (*ldr)->test(file);
        if (err == 0)
            *loader = *ldr;
        if (err == 0 || err != -ENOEXEC)
            break;
    }
    fs_fildes_ref(curproc->files, fildes, -1);

    return err;
}

static intptr_t sys_exec(__user void * user_args)
{
    struct _exec_args args;
    char name[PROC_NAME_SIZE];
    struct buf * env_bp = NULL;
    size_t arg_offset = 0;
    uintptr_t envp;
    struct exec_loadfn * loader;
    int err;

    KERROR_DBG("%s: curpid: %d\n", __func__, curproc->pid);

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        err = -EFAULT;
        goto fail;
    }

    if (!args.argv || !args.env) {
        err = -EINVAL;
        goto fail;
    }

    err = get_loader(args.fd, &loader);
    if (err)
        goto fail;

    /*
     * Copy in & out arguments and environ.
     */
    env_bp = geteblk(MMU_PGSIZE_COARSE);
    if (!env_bp) {
        err = -ENOMEM;
        goto fail;
    }

    /* Currently copyin_aa() requires vaddr to be set. */
    env_bp->b_mmu.vaddr = configUENV_BASE_ADDR;
    env_bp->b_uflags = VM_PROT_READ | VM_PROT_WRITE;

    /* Clone argv */
    err = clone_aa(env_bp, (__user char *)args.argv, args.nargv, &arg_offset);
    if (err) {
        KERROR_DBG("Failed to clone args (%d)\n", err);
        goto fail;
    }
    arg_offset = memalign(arg_offset);
    envp = env_bp->b_mmu.vaddr + arg_offset;

    /* Clone env */
    err = clone_aa(env_bp, (__user char *)args.env, args.nenv, &arg_offset);
    if (err) {
        KERROR_DBG("Failed to clone env (%d)\n", err);
        goto fail;
    }

    strlcpy(name, (char *)(env_bp->b_data) + (args.nargv + 1) * sizeof(char *),
            sizeof(name));

    /*
     * Execute.
     */
    err = exec_file(loader, args.fd, name, env_bp, args.nargv,
                    env_bp->b_mmu.vaddr, envp);
    if (err)
        goto fail;

    return 0;
fail:
    if (env_bp && env_bp->vm_ops->rfree) {
        env_bp->vm_ops->rfree(env_bp);
    }
    set_errno(-err);
    return -1;
}

static const syscall_handler_t exec_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_EXEC_EXEC, sys_exec),
};
SYSCALL_HANDLERDEF(exec_syscall, exec_sysfnmap)
