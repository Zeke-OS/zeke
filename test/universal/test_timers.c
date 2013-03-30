/**
 * @file test_sched.c
 * @brief Test scheduler thread creation functions.
 */

#include <stdio.h>
#include <stdint.h>
#include "punit.h"

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

/* Override sched_thread_set_exec */
int my_sched_called = 0;
void sched_thread_set_exec(void);
void sched_thread_set_exec(void) { my_sched_called++; }

#include "timers.h"

static void call_timers_run(void);
static void call_timers_run(void)
{
    int i;
    for (i = 0; i < configSCHED_FREQ; i++)
        timers_run();
}

static void setup()
{
    timers_init();
    my_sched_called = 0;
}

static void teardown()
{
}

static char * test_timers_add_run()
{
    int err;

    call_timers_run();
    err = timers_add(0, osTimerOnce, 3);
    call_timers_run();
    call_timers_run();
    call_timers_run();

    pu_assert("timers_add shouldn't have returned error", err >= 0);
    pu_assert("my_sched_thread_set_exec should have been called once by now",
              my_sched_called == 1);

    return 0;
}

static char * test_timers_add_run_multiple()
{
    int err;

    call_timers_run();
    err =  timers_add(2, osTimerOnce, 2);
    err |= timers_add(1, osTimerOnce, 2);
    err |= timers_add(3, osTimerOnce, 20 * configSCHED_FREQ);
    call_timers_run();

    pu_assert("timers_add shouldn't have returned error", err >= 0);
    pu_assert("my_sched_thread_set_exec should have been called twice by now",
              my_sched_called == 2);

    call_timers_run();

    pu_assert("my_sched_thread_set_exec should have been called three times by now",
              my_sched_called == 3);

    return 0;
}

static char * test_timers_add_run_zero_delay()
{
    call_timers_run();
    call_timers_run();
    call_timers_run();
    timers_add(1, osTimerOnce, 0);
    call_timers_run();

    pu_assert("my_sched_thread_set_exec should have been called once by now",
              my_sched_called == 1);

    return 0;
}

/* ALL TESTS *****************************************************************/

static void all_tests() {
    pu_def_test(test_timers_add_run, PU_RUN);
    pu_def_test(test_timers_add_run_multiple, PU_RUN);
    pu_def_test(test_timers_add_run_zero_delay, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
