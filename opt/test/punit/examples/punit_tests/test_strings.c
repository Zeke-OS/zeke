/* file test_strings.c */

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
    char str[] = "left string";

    pu_assert_str_equal("Strings are equal", str, "left string");

    return NULL;
}

static char * test_fail(vois)
{
    char str[] = "left string";

    pu_assert_str_equal("Strings are equal", str, "right string");

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
