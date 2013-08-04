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

static char main_Stack[configAPP_MAIN_SSIZE];

int kinit(void)
{
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
}
