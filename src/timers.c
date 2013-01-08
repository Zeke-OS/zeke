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
    int thread_id;      /*!< Thread id */
    uint32_t expires;   /*!< Timer expiration time */
} timer_alloc_data_t;

uint32_t timers_value; /*!< Current tick value */
static volatile timer_alloc_data_t timers_array[configTIMERS_MAX];

void timers_init(void)
{
    int i;

    timers_value = 0;
    for (i = 0; i < configTIMERS_MAX; i++) {
        timers_array[i].thread_id = -1;
        timers_array[i].expires = 0;
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
        if (timers_array[i].thread_id >= 0) {
            if (exp == value) {
                sched_thread_set_exec(timers_array[i].thread_id);

                /* Release the timer */
                timers_array[i].thread_id = -1;
                timers_array[i].expires = 0;
            }
        }
    } while (++i < configTIMERS_MAX);
}

/**
 * Reserve a new timer
 * @return 0 if succeed or -1 if there is no free timers available
 */
int timers_add(int thread_id, uint32_t millisec)
{
    int i = 0;
    uint32_t value = timers_value;

    do {
        if (timers_array[i].thread_id == -1) {
            timers_array[i].thread_id = thread_id;
            timers_array[i].expires = value + ((millisec * configSCHED_FREQ) / 1000);
            if (timers_array[i].expires == value)
                timers_array[i].expires++;
            return 0;
        }
    } while (++i < configTIMERS_MAX);

    return -1;
}

/**
 * Remove all timer allocations for a given thread
 */
void timers_release(int thread_id)
{
    int i = 0;
    do {
        if (timers_array[i].thread_id == thread_id) {
            /* Release the timer */
            timers_array[i].thread_id = -1;
            timers_array[i].expires = 0;
        }
    } while (++i < configTIMERS_MAX);
}
