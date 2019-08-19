#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <zeke.h>
#include "punit.h"

char stack[4096];
pthread_t test_tid;

static void setup(void)
{
    test_tid = 0;
}

static void teardown(void)
{
    /* Intentionally unimplemented... */
}

static void * thread(void * arg)
{
    test_tid = pthread_self();

    return &test_tid;
}

static char * test_create(void)
{
    pthread_attr_t attr;
    pthread_t tid;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, sizeof(stack));
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pu_assert_equal("Thread created",
            pthread_create(&tid, &attr, thread, 0), 0);

    bsleep(2);

    pu_assert_equal("Thread IDs are equal", tid, test_tid);

    return NULL;
}

static char * test_join(void)
{
    pthread_attr_t attr;
    pthread_t tid;
    pthread_t * ret;

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, stack, sizeof(stack));

    pu_assert_equal("Thread created",
            pthread_create(&tid, &attr, thread, 0), 0);

    pthread_join(tid, (void **)(&ret));
    pu_assert_equal("Thread IDs are equal", tid, test_tid);
    pu_assert_ptr_equal("Join returned the correct pointer", &test_tid, ret);
    pu_assert_equal("Thread IDs are equal", tid, *ret);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_create, PU_RUN);
    pu_def_test(test_join, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
