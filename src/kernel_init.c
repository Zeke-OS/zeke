/**
 *******************************************************************************
 * @file    kernel_init.c
 * @author  Olli Vanhoja
 * @brief   System init for Zero Kernel.
 *******************************************************************************
 */

#include "kernel.h"
#include "timers.h"
#include "sched.h"
#include "app_main.h"
#include "hal/hal_mcu.h"
#if configDEVSUBSYS != 0
#include "dev/dev.h"
#include "dev/lcd_ctrl.h"
#endif

static char main_Stack[configAPP_MAIN_SSIZE];

int main(void)
{
    if (interrupt_init_module()) {
		while (1);
	}
    timers_init();
    sched_init();

#if configDEVSUBSYS != 0
    /* Initialize device drivers */
    dev_init_all();
#endif

    {
        /* Create app_main thread */
        osThreadDef_t main_thread = {
            .pthread   = (os_pthread)(&app_main),
            .tpriority = configAPP_MAIN_PRI,
            .stackAddr = main_Stack,
            .stackSize = sizeof(main_Stack)/sizeof(char)
        };
        osThreadCreate(&main_thread, NULL);
    }

    sched_start();
    while (1);
}
