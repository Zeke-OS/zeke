#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "punit.h"

static void setup()
{
}

static void teardown()
{
}


#define testprintf( s, ... ) snprintf( s, 100, __VA_ARGS__ )

/* TODO test cases from printf_testcases.h */

static char * test_retlen()
{
    pu_assert("snprintf returns length of the string that would be written",
              snprintf(NULL, 0, "foo") == 3);
    pu_assert("snprintf returns length of the string that would be written",
              snprintf(NULL, 0, "%d", 100) == 3);

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_retlen, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
