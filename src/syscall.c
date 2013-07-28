/**
 *******************************************************************************
 * @file    syscall.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel's internal Syscall handler that is called from kernel scope.
 *
 *******************************************************************************
 */

#include "sched.h"
#define KERNEL_INTERNAL 1
#ifndef PU_TEST_BUILD
#include "hal/hal_core.h"
#endif
#include "ksignal.h"
#if configDEVSUBSYS != 0
#include "dev/dev.h"
#endif
#include "locks.h"
#include <syscall.h>

/* For all Syscall groups */
#if configDEVSUBSYS != 0
#define FOR_ALL_SYSCALL_GROUPS(apply)                       \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)               \
    apply(SYSCALL_GROUP_SCHED_THREAD, sched_syscall_thread) \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)            \
    apply(SYSCALL_GROUP_DEV, dev_syscall)                   \
    apply(SYSCALL_GROUP_LOCKS, locks_syscall)
#elif PU_TEST_BUILD != 0
#define FOR_ALL_SYSCALL_GROUPS(apply)                       \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)               \
    apply(SYSCALL_GROUP_SCHED_THREAD, sched_syscall_thread) \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)
#else
#define FOR_ALL_SYSCALL_GROUPS(apply)                       \
    apply(SYSCALL_GROUP_SCHED, sched_syscall)               \
    apply(SYSCALL_GROUP_SCHED_THREAD, sched_syscall_thread) \
    apply(SYSCALL_GROUP_SIGNAL, ksignal_syscall)            \
    apply(SYSCALL_GROUP_LOCKS, locks_syscall)
#endif

typedef uint32_t (*kernel_syscall_handler_t)(uint32_t type,  void * p);

static kernel_syscall_handler_t syscall_callmap[] = {
    #define SYSCALL_MAP_X(major, function) [major] = function,
    FOR_ALL_SYSCALL_GROUPS(SYSCALL_MAP_X)
    #undef SYSCALL_MAP_X
};

/**
 * Kernel's internal Syscall handler/translator
 *
 * This function is called from interrupt handler. This function calls the
 * actual kernel function and returns a result pointer/data to the interrupt
 * handler which returns it to the original caller, which is usually a library
 * function in kernel.c
 * @param type  syscall type.
 * @param p     pointer to the parameter or parameter structure.
 * @return      result value or pointer to the result from the called kernel
 *              function.
 */
//#pragma optimize=no_code_motion
uint32_t _intSyscall_handler(uint32_t type, void * p)
{
    kernel_syscall_handler_t fpt;
    uint32_t major = SYSCALL_MAJOR(type);

    if (major >= (sizeof(syscall_callmap) / sizeof(void *))) {
        /* 0/NULL means usually ERROR, however there is some cases where NULL as
         * a return value doesn't necessarily mean error. */
        return 0; /* NULL */
    }

    fpt = syscall_callmap[major];
    return fpt(type, p);
}
