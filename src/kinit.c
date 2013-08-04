/**
 *******************************************************************************
 * @file    kernel_init.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 *******************************************************************************
 */

#include <kernel.h>
#include <app_main.h>
#include "sched.h"

extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));
extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

static char main_Stack[configAPP_MAIN_SSIZE];

/**
 * Run all kernel module initializers.
 */
void exec_init_array(void)
{
    int n, i;

    n = __init_array_end - __init_array_start;
    for (i = 0; i < n; i++) {
        __init_array_start[i] ();
    }
}

/**
 * Initialize main thread.
 */
int kinit(void)
{
    /* Create app_main thread */
    osThreadDef_t main_thread = {
        .pthread   = (os_pthread)(&main),
        .tpriority = configAPP_MAIN_PRI,
        .stackAddr = main_Stack,
        .stackSize = sizeof(main_Stack)/sizeof(char)
    };
    osThreadCreate(&main_thread, NULL);
}


/**
 * Run all kernel module finalizers.
 */
void exec_fini_array(void)
{
    int n, i;

    n = __fini_array_end - __fini_array_start;
    for (i = 0; i < n; i++) {
        __fini_array_start[i] ();
    }
}

