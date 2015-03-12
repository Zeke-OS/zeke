#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include "punit.h"

char stack[4096];
pthread_t test_tid;

static void setup()
{
    test_tid = 0;
}

static void teardown()
{
}

static void * thread(void * arg)
{
    test_tid = pthread_self();

    return &test_tid;
}

static char * test_create()
{
    pthread_attr_t attr = {
        .tpriority  = 0,
        .stackAddr  = stack,
        .stackSize  = sizeof(stack),
    };
    pthread_t tid, ret;

    pu_assert_equal("Thread created",
            pthread_create(&tid, &attr, thread, 0), 0);

    /* TODO We may like to try join here but it's not supported yet */
#if 0
    pthread_join(tid, &ret);
    pu_assert_equal("Thread IDs are equal", tid, ret);
#endif
    pthread_detach(tid);

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_create, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
