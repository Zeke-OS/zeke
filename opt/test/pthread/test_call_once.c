#include <threads.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <zeke.h>
#include "punit.h"

static int count;
static once_flag once = ONCE_FLAG_INIT;

static void setup(void)
{
    count = 0;
}

static void teardown(void)
{
    /* Intentionally unimplemented... */
}

static void do_once(void)
{
    count++;
}

static char * test_call_once(void)
{
    pu_assert_equal("", count, 0);
    call_once(&once, do_once);
    pu_assert_equal("", count, 1);
    call_once(&once, do_once);
    pu_assert_equal("", count, 1);
    do_once();
    pu_assert_equal("", count, 2);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_call_once, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
