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

void thread_test(void * arg);
static char stack_1[200];
static char stack_2[200];

int main()
{
    kernel_init();
    kernel_ThreadCreate(&thread_test, NULL, stack_1, sizeof(stack_1)/sizeof(char));
    kernel_ThreadCreate(&thread_test, NULL, stack_2, sizeof(stack_2)/sizeof(char));
    kernel_start();
    while(1) {}
}

void thread_test(void * arg)
{
    volatile int i = 0;
    while(1) {
        i++;
        volatile int n = 500000;
        for (; n > 0; n--) {
        }
        kernel_ThreadSleep();
    }
}