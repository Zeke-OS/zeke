#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "punit.h"

static char const abcde[] = "abcde";

static void setup(void)
{
    /* Intentionally unimplemented... */
}

static void teardown(void)
{
    /* Intentionally unimplemented... */
}

static int compare(const void * left, const void * right)
{
    return *((unsigned char *)left) - *((unsigned char *)right);
}

static char * test_bsearch(void)
{
    pu_assert("", bsearch("e", abcde, 4, 1, compare) == NULL);
    pu_assert("", bsearch("e", abcde, 5, 1, compare) == &abcde[4]);
    pu_assert("", bsearch("a", abcde + 1, 4, 1, compare) == NULL);
    pu_assert("", bsearch("0", abcde, 1, 1, compare) == NULL);
    pu_assert("", bsearch("a", abcde, 1, 1, compare) == &abcde[0]);
    pu_assert("", bsearch("a", abcde, 0, 1, compare) == NULL);
    pu_assert("", bsearch("e", abcde, 3, 2, compare) == &abcde[4]);
    pu_assert("", bsearch("b", abcde, 3, 2, compare) == NULL);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_bsearch, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
