/**
 *******************************************************************************
 * @file    thread.c
 * @author  Olli Vanhoja
 * @brief   Functions that are called in thread context/scope
 *******************************************************************************
 */

#ifdef KERNEL_INTERNAL
    #error KERNEL_INTERNAL must not be defined in thread scope!
#endif

#include "kernel.h"
#include "thread.h"

/**
 * Deletes thread on exit
 * @note This function is called while execution is in thread context.
 */
void del_thread(void)
{
    /* It's considered to be safer to call osThreadTerminate syscall here and
     * terminate the running process while in kernel context even there is
     * no separate privileged mode in Cortex-M0. This atleast improves
     * portability in the future.
     */
    osThreadId thread_id = osThreadGetId();
    osThreadTerminate(thread_id);
    osThreadYield();

    while(1); /* Once the context changes, the program will no longer return to
               * this thread */
}
