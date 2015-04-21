#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_imaxabs(void)
{
    pu_assert("", imaxabs((intmax_t)0) == 0);
    pu_assert("", imaxabs(INTMAX_MAX) == INTMAX_MAX);
    pu_assert("", imaxabs(INTMAX_MIN + 1) == -(INTMAX_MIN + 1));

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_imaxabs, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
