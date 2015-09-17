/* file test_equal.c */

#include <stdio.h>
#include "punit.h"
#include "testable.h"

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_ok(void)
{
    int value = 4;

    value = fn(value);
    pu_assert_equal("Values are equal", value, 4 + 1);
    return 0;
}

static char * test_fail(void)
{
    int value = 4;

    value = fn(value);
    pu_assert_equal("Values are equal", value, 4);
    return 0;
}

static void all_tests(void)
{
    pu_def_test(test_ok, PU_RUN);
    pu_def_test(test_fail, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
