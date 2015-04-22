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
    char * arr1[] = {"one", "two", "three"};
    char * arr2[] = {"one", "two", "three"};

    pu_assert_str_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));
    return 0;
}

static char * test_fail()
{
    char * arr1[] = {"one", "two", "three"};
    char * arr2[] = {"one", "three", "four"};

    pu_assert_str_array_equal("Arrays are equal", arr1, arr2, sizeof(arr1)/sizeof(*arr1));
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
