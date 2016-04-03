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
    int tst;

    tst = imaxabs((intmax_t)0) == 0;
    pu_assert("The absolute value of zero equals zero", tst);

    tst = imaxabs(INTMAX_MAX) == INTMAX_MAX;
    pu_assert("Int max absolute value equals to int max", tst);

    tst = imaxabs(INTMAX_MIN + 1) == -(INTMAX_MIN + 1);
    pu_assert("abs(min + 1) == -(min + 1)", tst);

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
