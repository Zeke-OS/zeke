#include <buf.h>
#include <kerror.h>
#include <kunit.h>
#include <libkern.h>
#include <thread.h>

static char data[] = "0000000000";
static int j;

static void setup(void)
{
    for (size_t i = 0; i < num_elem(data); i++) {
        data[i] = '0';
    }
    data[num_elem(data) - 1] = '\0';
    j = 0;
}

static void teardown(void)
{
}

static void * test_thread_yield_thread(void * arg)
{
    while (j == 0) {
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }

    KERROR(KERROR_DEBUG, "start\n");
    for (int i = 0; i < 4; i++) {
        data[j++] = '0' + (int)arg;
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }
    KERROR(KERROR_DEBUG, "%s\n", data);

    return NULL;
}

static char * test_thread_yield(void)
{
    struct sched_param param = {
        .sched_policy = SCHED_RR,
        .sched_priority = 0,
    };

    (void)kthread_create("yield_test1", &param, 0,
                         test_thread_yield_thread, (void *)1);
    KERROR(KERROR_DEBUG, "thread 1 created\n");
    (void)kthread_create("yield_test2", &param, 0,
                         test_thread_yield_thread, (void *)2);
    KERROR(KERROR_DEBUG, "thread 2 created\n");
    j = 1;

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_thread_yield, KU_RUN);
}

TEST_MODULE(sched, yield);
