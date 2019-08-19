#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "punit.h"

static void setup(void)
{
    /* Intentionally unimplemented... */
}

static void teardown(void)
{
    /* Intentionally unimplemented... */
}

static char * test_abs(void)
{
    int tst;

    tst = abs(0) == 0;
    pu_assert("abs(0) == 0", tst);

    tst = abs(INT_MAX) == INT_MAX;
    pu_assert("abs(INT_MAX) == INT_MAX", tst);

    tst = abs(INT_MIN + 1) == -(INT_MIN + 1);
    pu_assert("abs(INT_MIN + 1) == -(INT_MIN + 1)", tst);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_abs, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
