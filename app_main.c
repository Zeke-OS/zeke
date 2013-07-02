/**
 *******************************************************************************
 * @file    app_main.c
 * @author  Olli Vanhoja
 *
 * @brief   Main entry point to the application
 *
 *******************************************************************************
 */
#include <stdio.h>
#include "kernel.h"
#include "app_main.h"
#include "stm32f0_discovery.h"

static char stack_1[300];
static char stack_2[300];
static char stack_3[200];

void createThreads(void);
void thread_input(void const * arg);
void thread_led(void const * arg);
void thread_load_test(void const * arg);
static void print_loadAvg(void);

static int x = 5;
static int y = 8;
volatile int z;

osThreadId th1_id;
osThreadId th2_id;

osDev_t dev_lcd = DEV_MMTODEV(1, 0);

/**
  * main thread
  */
void app_main(void)
{
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

    createThreads();
    osDelay(osWaitForever);
}

void createThreads(void)
{
    osThreadDef_t th_1 = {  (os_pthread)(&thread_input),
                            osPriorityBelowNormal,
                            stack_1,
                            sizeof(stack_1) / sizeof(char)
                         };
    osThreadDef_t th_2 = {  (os_pthread)(&thread_led),
                            osPriorityHigh,
                            stack_2,
                            sizeof(stack_2) / sizeof(char)
                         };
    osThreadDef_t th_3 = {  (os_pthread)(&thread_load_test),
                            osPriorityBelowNormal,
                            stack_3,
                            sizeof(stack_3) / sizeof(char)
                         };

    th1_id = osThreadCreate(&th_1, &x);
    th2_id = osThreadCreate(&th_2, &y);
    osThreadCreate(&th_3, NULL);
}

void thread_input(void const * arg)
{
    while (1) {
        osDelay(5);

        if(STM_EVAL_PBGetState(BUTTON_USER) == SET)
        {
            osSignalSet(th2_id, 1);
            /* GENERATE HARDFAULT */
            /*asm volatile ("MOVS R0, #0\n"
                          "BX R0\n");*/
            osDelay(1000);
        }
    }

    //osDevClose(dev_lcd);
}

void thread_led(void const * arg)
{
    volatile osEvent evnt;

    if (osDevOpen(dev_lcd)) {
        while (1);
    }

    while(1) {
        STM_EVAL_LEDToggle(LED3);
        print_loadAvg();
        evnt = osWait(osWaitForever);
        z = (uint32_t)evnt.status;
    }
}

void thread_load_test(void const * arg)
{
    volatile unsigned int i = 0;
    while (1) {
        i++;
        if (i % 102400)
            osDelay(100);
    }
}

static void print_loadAvg(void)
{
    uint32_t lavg[3];
    char buff[80];
    int i;

    osGetLoadAvg(lavg);
    sprintf(buff, "\x0dLoad avg:\n%d %d %d", lavg[0], lavg[1], lavg[2]);
    /* \x0d = CR := return to home position */

    i = 0;
    while (buff[i] != '\0') {
        osDevCwrite(buff[i++], dev_lcd);
    }
}
