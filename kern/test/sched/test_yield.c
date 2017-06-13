#include <buf.h>
#include <kerror.h>
#include <kunit.h>
#include <libkern.h>
#include <thread.h>

static void setup(void)
{
}

static void teardown(void)
{
}

static void * test_thread_yield_thread(void * arg)
{
    KERROR(KERROR_DEBUG, "start\n");
    for (int i = 0; i < 4; i++) {
        KERROR(KERROR_DEBUG, "thread %d\n", (int)arg);
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }

    return NULL;
}

static char * test_thread_yield(void)
{
    struct sched_param param = {
        .sched_policy = SCHED_OTHER,
        .sched_priority = 0,
    };

    (void)kthread_create(&param, 0, test_thread_yield_thread, (void *)1);
    KERROR(KERROR_DEBUG, "thread 1 created\n");
    (void)kthread_create(&param, 0, test_thread_yield_thread, (void *)2);
    KERROR(KERROR_DEBUG, "thread 2 created\n");

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_thread_yield, KU_RUN);
}

TEST_MODULE(sched, yield);
