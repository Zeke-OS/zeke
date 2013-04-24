/**
 * @file test_queue.c
 * @brief Test generic thread-safe queue implementation.
 */

#include <stdio.h>
#include "punit.h"
#include "queue.h"

int tarr[5];
queue_cb_t queue;

static void setup()
{
    int i;
    for (i = 0; i < sizeof(tarr) / sizeof(int); i++) {
        tarr[i] = 0;
    }

    queue = queue_create(&tarr, sizeof(int), sizeof(tarr));
}

static void teardown()
{
    queue_clearFromPushEnd(&queue);
}

static char * test_queue_single_push(void)
{
    int x = 5;
    int err;

    err = queue_push(&queue, &x);
    pu_assert("error, push failed", err != 0);
    pu_assert_equal("error, value of x was not pushed to the first index", tarr[0], x);

    return 0;
}

static char * test_queue_single_pop(void)
{
    int x = 5;
    int y;
    int err;

    err = queue_push(&queue, &x);
    pu_assert("error, push failed", err != 0);

    err = queue_pop(&queue, &y);
    pu_assert("error, pop failed", err != 0);
    pu_assert_equal("error, different value was returned with pop that pushed with push", x, y);

    return 0;
}

static void all_tests() {
    pu_def_test(test_queue_single_push, PU_RUN);
    pu_def_test(test_queue_single_pop, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

