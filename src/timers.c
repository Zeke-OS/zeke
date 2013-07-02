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

/* Internal Flag Bits */
/* NONE */

/**
 * Indicates a free timer position on thread_id of allocation entry.
 */
#define TIMERS_POS_FREE -1

/** Timer allocation struct */
typedef struct {
    timers_flags_t flags;   /*!< Timer flags
                             * + 0 = Timer state
                             *     + 0 = disabled
                             *     + 1 = enabled
                             * + 1 = Timer type
                             *     + 0 = one-shot
                             *     + 1 = periodic
                             */
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
        timers_array[i].flags = 0;
        timers_array[i].thread_id = TIMERS_POS_FREE;
    }
}

void timers_run(void)
{
    int i;
    uint32_t value;

    timers_value++;
    value = timers_value;
    i = 0;
    do {
        uint32_t exp = timers_array[i].expires;
        if (timers_array[i].flags & TIMERS_FLAG_ENABLED) {
            if (exp == value) {
                sched_thread_set_exec(timers_array[i].thread_id);

                if (!(timers_array[i].flags & TIMERS_FLAG_PERIODIC)) {
                    /* Release the timer */
                    timers_array[i].flags &= ~TIMERS_FLAG_ENABLED;
                    timers_array[i].thread_id = TIMERS_POS_FREE;
                } else {
                    /* Repeating timer */
                    timers_array[i].expires = timers_calc_exp(timers_array[i].reset_val);
                }
            }
        }
    } while (++i < configTIMERS_MAX);
}

/**
 * Allocate a new timer
 * @param thread_id thread id to add this timer for.
 * @param flags User modifiable flags (see: TIMERS_USER_FLAGS)-
 * @param millisec delay to trigger from the time when enabled.
 * @param return -1 if allocation failed.
 */
int timers_add(osThreadId thread_id, timers_flags_t flags, uint32_t millisec)
{
    int i = 0;
    flags &= TIMERS_USER_FLAGS; /* Allow only user flags to be set */

    do { /* Locate first free timer */
        if (timers_array[i].thread_id == TIMERS_POS_FREE) {
            timers_array[i].thread_id = thread_id;
            timers_array[i].flags = flags;

            if (flags & TIMERS_FLAG_PERIODIC) { /* Periodic timer */
                timers_array[i].reset_val = millisec;
            }

            if (millisec > 0) {
                timers_array[i].expires = timers_calc_exp(millisec);
            }
            return i;
        }
    } while (++i < configTIMERS_MAX);

    return -1;
}

void timers_start(int tim)
{
    if (tim < configTIMERS_MAX && tim >= 0)
        return;

    timers_array[tim].flags |= TIMERS_FLAG_ENABLED;
}

/* TODO should check that owner matches before relasing the timer
 */
void timers_release(int tim)
{
    if (tim < configTIMERS_MAX && tim >= 0)
        return;

    /* Release the timer */
    timers_array[tim].flags = 0;
    timers_array[tim].thread_id = TIMERS_POS_FREE;
}

osThreadId timers_get_owner(int tim) {
    if (tim < configTIMERS_MAX && tim >= 0)
        return -1;

    return timers_array[tim].thread_id;
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
