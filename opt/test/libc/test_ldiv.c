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

char * test_ldiv(void)
{
    ldiv_t result;

    result = ldiv(5, 2);
    pu_assert("", result.quot == 2 && result.rem == 1);

    result = ldiv(-5, 2);
    pu_assert("", result.quot == -2 && result.rem == -1);

    result = ldiv(5, -2);
    pu_assert("", result.quot == -2 && result.rem == 1);
    pu_assert_equal("", sizeof(result.quot), sizeof(long));
    pu_assert_equal("", sizeof(result.rem), sizeof(long));

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_ldiv, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
