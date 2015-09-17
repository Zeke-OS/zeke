/* file test_doubles.c */

#include <stdio.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_ok(void)
{
    double value = 4.0f;

    pu_assert_double_equal("Values are approximately equal", value, 4.2f, 0.3f);

    return NULL;
}

static char * test_fail(void)
{
    double value = 3.0f;

    pu_assert_double_equal("Values are approximately equal", value, 5.0f, 0.5f);

    return NULL;
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
