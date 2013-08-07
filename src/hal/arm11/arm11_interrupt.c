/**
*******************************************************************************
* @file arm1176jzf_s_interrupt.c
* @author Olli Vanhoja
* @brief Interrupt service routines.
*******************************************************************************
*/

/** @addtogroup HAL
* @{
*/

/** @addtogroup ARM1176JZF_S
* @{
*/

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <sched.h>
#include <syscall.h>
#include "../hal_core.h"
#include "arm11_interrupt.h"

void interrupt_init_module(void) __attribute__((constructor));
static inline void run_scheduler(void);

void interrupt_init_module(void)
{
    /* TODO */
    return 0; /* OK */
}

static inline void run_scheduler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        sched_handler();
        load_context(); /* Since PSP has been updated, this loads the last state
                         * of the resumed task */
        __asm__ volatile (
            "ADD sp, sp, %0\n"
            :
            : "r" (THREAD_RETURN));
    }
}

/**
* This function handles NMI exception.
*/
void NMI_Handler(void)
{
    /* @todo fixme */
}

/**
* This function handles SVCall exception.
*/
#pragma optimize = no_cse
void SVC_Handler(void)
{
    uint32_t type;
    void * p;

    __asm__ volatile(
        "MOV %[typ], r2\n" /* Read parameters from r2 & r3 */
        "MOV %[arg], r3\n"
        : [typ]"=r" (type), [arg]"=r" (p)
        :
        : "r2", "r3"
    );

    /* Call kernel's internal syscall handler */
    uint32_t result = _intSyscall_handler(type, p);

    /* Return value is stored to r4 */
    __asm__ volatile(
        "MOV r4, %0\n"
        :
        : "r" (result)
        : "r4"
    );
}

/**
* This function handles PendSVC exception.
*/
void PendSV_Handler(void)
{
    run_scheduler();
}

/**
* This function handles SysTick Handler.
*/
void SysTick_Handler(void)
{
    run_scheduler();
}

/**
* @}
*/

/**
* @}
*/
