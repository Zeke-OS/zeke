/**
 * @file test_sched.c
 * @brief Test scheduler thread creation functions.
 */

#include <stdio.h>
#include <stdint.h>
#include "punit.h"

#include "kernel.h"

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

/* Override timers_run */
void my_timers_run(void);
void my_timers_run(void) { }
#define timers_run my_timers_run

/* Override timers_add */
int my_timers_add(int thread_id, os_timer_type type, uint32_t millisec);
int my_timers_add(int thread_id, os_timer_type type, uint32_t millisec) { return 0; }
#define timers_add my_timers_add

/* Override timers_release */
void timers_release(int tim);
void timers_release(int tim) { }

/* NOTE: Included sched.c */
#include "sched.c"
#include "ksignal.c"

/* Override del_thread */
void my_del_thread(void);
void my_del_thread(void) { }
#define del_thread my_del_thread

/* Override heap_insert with dummy function */
void my_heap_insert(void * h, threadInfo_t * th);
void my_heap_insert(void * h, threadInfo_t * th) { }
#define heap_insert my_heap_insert


void th1(void * arg);
void th2(void * arg);
void th1(void * arg) { }
void th2(void * arg) { }

static char stack_1[20];
static char stack_2[20];

static void setup()
{
    memset(&task_table, 0, sizeof(threadInfo_t)*configSCHED_MAX_THREADS);
}

static void teardown()
{
}

/* THREAD CREATION ***********************************************************/

static char * test_sched_threadCreate(void)
{
    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };
    osThreadDef_t thread_def2 = { (os_pthread)(&th2),
                                  osPriorityHigh,
                                  stack_2,
                                  sizeof(stack_2)/sizeof(char)
                                };

    sched_threadCreate(&thread_def1, NULL);
    sched_threadCreate(&thread_def2, NULL);

    pu_assert("error, incorrect flags set for thread1",
              task_table[1].flags == (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG));
    pu_assert("error, incorrect flags set for thread2",
              task_table[2].flags == (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG));

    /* Time slices are now pre calculated to this test is deprecated. */
    /*pu_assert_equal("error, incorrect ts_counter value for thread1",
              task_table[1].ts_counter, 0);

    pu_assert_equal("error, incorrect ts_counter value for thread2",
              task_table[2].ts_counter, 0);*/

    pu_assert("error, incorrect priority for thread1",
              task_table[1].def_priority == thread_def1.tpriority);

    pu_assert("error, incorrect priority for thread2",
              task_table[2].def_priority == thread_def2.tpriority);

    pu_assert_equal("error, priority and def_priority should be equal for thread1",
              task_table[1].priority, task_table[0].def_priority);

    pu_assert_equal("error, priority and def_priority should be equal for thread2",
              task_table[2].priority, task_table[2].def_priority);

    return 0;
}

static char * test_sched_thread_set_inheritance(void)
{
    static char stack_1[20];

    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };
    osThreadDef_t thread_def2 = { (os_pthread)(&th1),
                                  osPriorityAboveNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };
    osThreadDef_t thread_def3 = { (os_pthread)(&th1),
                                  osPriorityHigh,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };
    osThreadDef_t thread_def4 = { (os_pthread)(&th1),
                                  osPriorityHigh,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };

    sched_thread_set(1, &thread_def1, NULL, NULL);
    sched_thread_set(2, &thread_def2, NULL, &(task_table[1]));
    sched_thread_set(3, &thread_def3, NULL, &(task_table[1]));
    sched_thread_set(4, &thread_def4, NULL, &(task_table[2]));

    /* Test parent attributes */
    pu_assert_null("error, thread1 should not have parent",
              task_table[1].inh.parent);
    pu_assert_ptr_equal("error, thread2's parent should be thread1",
              task_table[2].inh.parent, &(task_table[1]));
    pu_assert_ptr_equal("error, thread3's parent should be thread1",
              task_table[3].inh.parent, &(task_table[1]));
    pu_assert_ptr_equal("error, thread4's parent should be thread2",
              task_table[4].inh.parent, &(task_table[2]));

    /* Test first_child attributes */
    pu_assert_ptr_equal("error, thread1's first child should be thread2",
              task_table[1].inh.first_child, &(task_table[2]));
    pu_assert_ptr_equal("error, thread2's first child should be thread4",
              task_table[2].inh.first_child, &(task_table[4]));

    /* Test next_child attributes */
    pu_assert_ptr_equal("error, thread2 should have thread3 as a next_child",
              task_table[2].inh.next_child, &(task_table[3]));
    pu_assert_null("error, thread4 should have NULL as a next_child",
              task_table[4].inh.next_child);

    return 0;
}

/* THREAD DELAY **************************************************************/

static char * test_sched_threadDelay_positiveInput()
{
    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };

    sched_thread_set(1, &thread_def1, NULL, NULL);
    current_thread = &(task_table[1]);

    pu_assert("Positive delay value should result osOK",
              sched_threadDelay(15) == osOK);

    pu_assert("Thread execution flag should be disable",
              (current_thread->flags & SCHED_EXEC_FLAG) == 0);

    pu_assert("Thread NO_SIG_FLAG should be set",
              (current_thread->flags & SCHED_NO_SIG_FLAG) == SCHED_NO_SIG_FLAG);

    return 0;
}

static char * test_sched_threadDelay_infiniteInput()
{
    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };

    sched_thread_set(1, &thread_def1, NULL, NULL);
    current_thread = &(task_table[1]);

    pu_assert("osWaitForever delay value should result osOK",
              sched_threadDelay(osWaitForever) == osOK);

    pu_assert("Thread execution flag should be disabled",
              (current_thread->flags & SCHED_EXEC_FLAG) == 0);

    pu_assert("Thread NO_SIG_FLAG should be set",
              (current_thread->flags & SCHED_NO_SIG_FLAG) == SCHED_NO_SIG_FLAG);

    return 0;
}

/* THREAD WAIT ***************************************************************/

static char * test_sched_threadWait_positiveInput()
{
    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };

    sched_thread_set(1, &thread_def1, NULL, NULL);
    current_thread = &(task_table[1]);

    pu_assert_equal("Positive wait timeout value should result osEventTimeout",
              sched_threadWait(15)->status, osEventTimeout);

    pu_assert("Thread execution flag should be disable",
              (current_thread->flags & SCHED_EXEC_FLAG) == 0);

    pu_assert("Thread NO_SIG_FLAG shouldn't be set",
              (current_thread->flags & SCHED_NO_SIG_FLAG) == 0);
    return 0;
}

static char * test_sched_threadWait_infiniteInput()
{
    osThreadDef_t thread_def1 = { (os_pthread)(&th1),
                                  osPriorityNormal,
                                  stack_1,
                                  sizeof(stack_1)/sizeof(char)
                                };

    sched_thread_set(1, &thread_def1, NULL, NULL);
    current_thread = &(task_table[1]);

    pu_assert_equal("osWaitForever timeout value should result osEventTimeout",
              sched_threadWait(osWaitForever)->status, osEventTimeout);

    pu_assert("Thread execution flag should be disabled",
              (current_thread->flags & SCHED_EXEC_FLAG) == 0);

    pu_assert("Thread NO_SIG_FLAG shouldn't be set",
              (current_thread->flags & SCHED_NO_SIG_FLAG) == 0);

    return 0;
}

/* ALL TESTS *****************************************************************/

static void all_tests() {
    pu_def_test(test_sched_threadCreate, PU_RUN);
    pu_def_test(test_sched_thread_set_inheritance, PU_RUN);
    pu_def_test(test_sched_threadDelay_positiveInput, PU_RUN);
    pu_def_test(test_sched_threadDelay_infiniteInput, PU_RUN);
    pu_def_test(test_sched_threadWait_positiveInput, PU_RUN);
    pu_def_test(test_sched_threadWait_infiniteInput, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
