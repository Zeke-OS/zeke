#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "punit.h"

const char * testfile = "test.fil";

static void setup(void)
{
    /* Intentionally unimplemented... */
}

static void teardown(void)
{
    remove(testfile);
}


char * test_fopen(void)
{
    /*
     * Some of the tests are not executed for regression tests, as the libc on
     * my system is at once less forgiving (segfaults on mode NULL) and more
     *  forgiving (accepts undefined modes).
     */
    FILE * fh;

#if 0
    TESTCASE_NOREG(fopen(NULL, NULL) == NULL);
#endif
    pu_assert("",  fopen(NULL, "w") == NULL);
#if 0
    TESTCASE_NOREG(fopen("", NULL) == NULL);
#endif
    pu_assert("",  fopen("", "w") == NULL);
    pu_assert("",  fopen("foo", "") == NULL);
#if 0
    TESTCASE_NOREG(fopen(testfile, "wq") == NULL); /* Undefined mode */
    TESTCASE_NOREG(fopen(testfile, "wr") == NULL); /* Undefined mode */
#endif
    pu_assert("",  (fh = fopen(testfile, "w")) != NULL);
    pu_assert("",  fclose(fh) == 0);

    return NULL;
}

static void all_tests(void)
{
        pu_def_test(test_fopen, PU_SKIP);
}

int main(int argc, char ** argv)
{
        return pu_run_tests(&all_tests);
}
