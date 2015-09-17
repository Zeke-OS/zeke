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
    int arr1[] = {0, 1, 2, 3, 4, 5};
    int arr2[] = {0, 1, 2, 3, 4, 5};

    pu_assert_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));

    return NULL;
}

static char * test_fail(void)
{
    int arr1[] = {0, 1, 2, 3, 4, 5};
    int arr2[] = {0, 1, 2, 4, 4, 5};

    pu_assert_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));

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
