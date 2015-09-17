/* file test_arrays.c */

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
    char * arr1[] = {"one", "two", "three"};
    char * arr2[] = {"one", "two", "three"};

    pu_assert_str_array_equal("Arrays are equal", arr1, arr2,
                              sizeof(arr1) / sizeof(*arr1));

    return NULL;
}

static char * test_fail(void)
{
    char * arr1[] = {"one", "two", "three"};
    char * arr2[] = {"one", "three", "four"};

    pu_assert_str_array_equal("Arrays are equal", arr1, arr2,
                              sizeof(arr1) / sizeof(*arr1));
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
