/* file test_arrays.c */

#include <stdio.h>
#include "punit.h"

static void setup()
{
}

static void teardown()
{
}

static char * test_ok()
{
    int arr1[] = {0, 1, 2, 3, 4, 5};
    int arr2[] = {0, 1, 2, 3, 4, 5};

    pu_assert_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));
    return 0;
}

static char * test_fail()
{
    int arr1[] = {0, 1, 2, 3, 4, 5};
    int arr2[] = {0, 1, 2, 4, 4, 5};

    pu_assert_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));
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
