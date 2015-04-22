/* file test_equal.c */

#include <stdio.h>
#include "punit.h"
#include "testable.h"

static void setup()
{
}

static void teardown()
{
}

static char * test_ok()
{
    int value = 4;

    value = fn(value);
    pu_assert_equal("Values are equal", value, 4 + 1);
    return 0;
}

static char * test_fail()
{
    int value = 4;

    value = fn(value);
    pu_assert_equal("Values are equal", value, 4);
    return 0;
}

static void all_tests()
{
    pu_run_test(test_ok);
    pu_run_test(test_fail);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
