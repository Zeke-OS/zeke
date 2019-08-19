#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_PDCLIB_int.h>
#include "punit.h"

static void setup(void)
{
    /* Intentionally unimplemented... */
}

static void teardown(void)
{
    /* Intentionally unimplemented... */
}

char * test_lldiv(void)
{
    lldiv_t result;

    result = lldiv(5, 2);
    pu_assert("", result.quot == 2 && result.rem == 1);

    result = lldiv(-5, 2);
    pu_assert("", result.quot == -2 && result.rem == -1);

    result = lldiv(5, -2);
    pu_assert("", result.quot == -2 && result.rem == 1);
    pu_assert_equal("", sizeof(result.quot), sizeof(long long));
    pu_assert_equal("", sizeof(result.rem), sizeof(long long));

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_lldiv, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
