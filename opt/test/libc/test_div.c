#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_PDCLIB_int.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

char * test_div(void)
{
    div_t result;

    result = div(5, 2);
    pu_assert("", result.quot == 2 && result.rem == 1);

    result = div(-5, 2);
    pu_assert("", result.quot == -2 && result.rem == -1);

    result = div(5, -2);
    pu_assert("", result.quot == -2 && result.rem == 1);
    pu_assert_equal("", sizeof(result.quot), sizeof(int));
    pu_assert_equal("", sizeof(result.rem), sizeof(int));

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_div, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
