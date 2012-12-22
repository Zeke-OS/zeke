/**
 *******************************************************************************
 * @file    main.c
 * @author  Olli Vanhoja
 *
 * @brief   Main
 *
 *******************************************************************************
 */

#include "kernel.h"

static char stack_1[200];
static char stack_2[200];

void createThreads(void);
void thread_test(void const * arg);

int main()
{
    kernel_init();
    createThreads();
    kernel_start();
    while(1) {}
}

void createThreads(void)
{
    osThreadDef_t th_1 = { (os_pthread)(&thread_test), osPriorityNormal, stack_1, sizeof(stack_1)/sizeof(char) };
    osThreadDef_t th_2 = { (os_pthread)(&thread_test), osPriorityHigh, stack_2, sizeof(stack_2)/sizeof(char) };
    osThreadCreate(&th_1, NULL);
    osThreadCreate(&th_2, NULL);
}



void thread_test(void const * arg)
{
    volatile int i = 0;
    while(1) {
        i++;
        volatile int n = 250000;
        for (; n > 0; n--) {
        }
        i = osDelay(osWaitForever);
    }
}