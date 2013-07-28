/**
 *******************************************************************************
 * @file    kernel_init.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 *******************************************************************************
 */

#include <kernel.h>
#include <app_main.h>
#include "timers.h"
#include "sched.h"
#include "hal/hal_mcu.h"
#if configDEVSUBSYS != 0
#include "dev/dev.h"
#include "dev/lcd_ctrl.h"
#endif

static char main_Stack[configAPP_MAIN_SSIZE];

int kinit(void)
{
    timers_init();
    sched_init();

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

    sched_start();
    while (1);
}
