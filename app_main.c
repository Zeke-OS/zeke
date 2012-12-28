/**
 *******************************************************************************
 * @file    main.c
 * @author  Olli Vanhoja
 *
 * @brief   Main entry point to the application
 *
 *******************************************************************************
 */

#include "stm32f0_discovery.h"
#include "kernel.h"
#include "app_main.h"

static char stack_1[200];
static char stack_2[200];

void createThreads(void);
void thread_input(void const * arg);
void thread_led(void const * arg);

static int x = 5;
static int y = 8;
volatile int z;

osThreadId th1_id;
osThreadId th2_id;

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
    osThreadDef_t th_1 = { (os_pthread)(&thread_input), osPriorityBelowNormal, stack_1, sizeof(stack_1)/sizeof(char) };
    osThreadDef_t th_2 = { (os_pthread)(&thread_led), osPriorityHigh, stack_2, sizeof(stack_2)/sizeof(char) };

    th1_id = osThreadCreate(&th_1, &x);
    th2_id = osThreadCreate(&th_2, &y);
}

void thread_input(void const * arg)
{
    while (1) {
        if(STM_EVAL_PBGetState(BUTTON_USER)== SET)
        {
            osSignalSet(th2_id, 1);
        }
    }
}

void thread_led(void const * arg)
{
    volatile osEvent evnt;
    while(1) {
        STM_EVAL_LEDToggle(LED3);
        evnt = osWait(osWaitForever);
        z = (uint32_t)evnt.status;
    }
}
