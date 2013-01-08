/**
 *******************************************************************************
 * @file    timers.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel timers
 *
 *******************************************************************************
 */

#include "kernel_config.h"
#include "sched.h"
#include "timers.h"

/** Timer allocation struct */
typedef struct {
    uint32_t enabled;       /*!< Enable/Disable timer */
    os_timer_type type;     /*!< One-shot or repeating timer */
    osThreadId thread_id;   /*!< Thread id */
    uint32_t reset_val;     /*!< Reset value for repeating timer */
    uint32_t expires;       /*!< Timer expiration time */
} timer_alloc_data_t;

uint32_t timers_value; /*!< Current tick value */
static volatile timer_alloc_data_t timers_array[configTIMERS_MAX];

static int timers_calc_exp(int millisec);

void timers_init(void)
{
    int i;

    timers_value = 0;
    for (i = 0; i < configTIMERS_MAX; i++) {
        timers_array[i].enabled = 0;
        timers_array[i].thread_id = -1;
    }
}

void timers_run(void)
{
    int i;
    uint32_t value;

    timers_value++;;
    value = timers_value;
    i = 0;
    do {
        uint32_t exp = timers_array[i].expires;
        if (timers_array[i].enabled) {
            if (exp == value) {
                sched_thread_set_exec(timers_array[i].thread_id);

                if (timers_array[i].type == osTimerOnce) {
                    /* Release the timer */
                    timers_array[i].enabled = 0;
                    timers_array[i].thread_id = -1;
                } else {
                    /* Repeating timer */
                    timers_array[i].expires = timers_calc_exp(timers_array[i].reset_val);
                }
            }
        }
    } while (++i < configTIMERS_MAX);
}

/**
 * Reserve a new timer
 * @return Reference to a timer or -1 if allocation failed.
 */
int timers_add(osThreadId thread_id, os_timer_type type, uint32_t millisec)
{
    int i = 0;

    do {
        if (timers_array[i].thread_id == -1) {
            timers_array[i].thread_id = thread_id;
            timers_array[i].type = type;
            if (type == osTimerPeriodic) {
                timers_array[i].reset_val = millisec;
            }

            if (millisec > 0) {
                timers_array[i].enabled = 1;
                timers_array[i].expires = timers_calc_exp(millisec);
            } /* else {
                timers_array[i].enabled = 0;
            } */
            return i;
        }
    } while (++i < configTIMERS_MAX);

    return -1;
}

void timers_start(int tim)
{
    timers_array[tim].enabled = 1;
}

void timers_release(int tim)
{
    /* Release the timer */
    timers_array[tim].enabled = 0;
    timers_array[tim].thread_id = -1;
}

static int timers_calc_exp(int millisec)
{
    int exp;
    uint32_t value = timers_value;

    exp = value + ((millisec * configSCHED_FREQ) / 1000);
    if (exp == value)
        exp++;

    return exp;
}
