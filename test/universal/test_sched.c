/**
 * @file test_sched.c
 * @brief Test scheduler thread creation functions.
 */

#include <stdio.h>
#include "minunit.h"

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

/* NOTE: Included sched.c */
#include "sched.c"

/* Override sched_thread_set_exec with our own implementation */
/*
void my_sched_thread_set_exec(int thread_id, osPriority pri);
void my_sched_thread_set_exec(int thread_id, osPriority pri)
{
}
#define sched_thread_set_exec my_sched_thread_set_exec
*/

/* Override heap_insert with dummy function */
void my_heap_insert(void * h, threadInfo_t * th);
void my_heap_insert(void * h, threadInfo_t * th) { }
#define heap_insert my_heap_insert

void th1(void * arg);
void th2(void * arg);
void th1(void * arg) { }
void th2(void * arg) { }

static void clear_task_table(void)
{
    memset(&task_table, 0, sizeof(threadInfo_t)*configSCHED_MAX_THREADS);
}

static char * test_sched_ThreadCreate(void)
{
    static char stack_1[20];
    static char stack_2[20];

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

    mu_begin_test();

    clear_task_table();

    sched_ThreadCreate(&thread_def1, NULL);
    sched_ThreadCreate(&thread_def2, NULL);

    mu_assert("error, incorrect flags set for thread1",
              task_table[1].flags == (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG));
    mu_assert("error, incorrect flags set for thread2",
              task_table[2].flags == (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG));

    mu_assert("error, incorrect uCounter value for thread1",
              task_table[1].uCounter == 0);

    mu_assert("error, incorrect uCounter value for thread2",
              task_table[2].uCounter == 0);

    mu_assert("error, incorrect priority for thread1",
              task_table[1].def_priority == thread_def1.tpriority);

    mu_assert("error, incorrect priority for thread2",
              task_table[2].def_priority == thread_def2.tpriority);

    mu_assert("error, priority and def_priority should be equal for thread1",
              task_table[1].priority == task_table[0].def_priority);

    mu_assert("error, priority and def_priority should be equal for thread2",
              task_table[2].priority == task_table[2].def_priority);

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

    mu_begin_test();

    clear_task_table();

    sched_thread_set(1, &thread_def1, NULL, NULL);
    sched_thread_set(2, &thread_def2, NULL, &(task_table[1]));
    sched_thread_set(3, &thread_def3, NULL, &(task_table[1]));
    sched_thread_set(4, &thread_def4, NULL, &(task_table[2]));

    /* Test parent attributes */
    mu_assert("error, thread1 should not have parent",
              task_table[1].inh.parent == NULL);
    mu_assert("error, thread2's parent should be thread1",
              task_table[2].inh.parent == &(task_table[1]));
    mu_assert("error, thread3's parent should be thread1",
              task_table[3].inh.parent == &(task_table[1]));
    mu_assert("error, thread4's parent should be thread2",
              task_table[4].inh.parent == &(task_table[2]));

    /* Test first_child attributes */
    mu_assert("error, thread1's first child should be thread2",
              task_table[1].inh.first_child == &(task_table[2]));
    mu_assert("error, thread2's first child should be thread4",
              task_table[2].inh.first_child == &(task_table[4]));

    /* Test next_child attributes */
    mu_assert("error, thread2 should have thread3 as a next_child",
              task_table[2].inh.next_child == &(task_table[3]));
    mu_assert("error, thread4 should have NULL as a next_child",
              task_table[4].inh.next_child == NULL);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_sched_ThreadCreate);
    mu_run_test(test_sched_thread_set_inheritance);
    return 0;
}

int main(int argc, char **argv)
{
    return mu_run_tests(&all_tests);
}
