/**
 *******************************************************************************
 * @file    locks.c
 * @author  Olli Vanhoja
 * @brief   Locks and Syscall handlers for locks
 *******************************************************************************
 */

#include "syscall.h"
#include "hal_core.h"
#include "locks.h"

uint32_t locks_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_MUTEX_TEST_AND_SET:
        return (uint32_t)test_and_set((int *)p);

    default:
        return NULL;
    }
}

