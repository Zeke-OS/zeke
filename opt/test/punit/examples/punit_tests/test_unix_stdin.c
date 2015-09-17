/* file test_unix_stdin.c */

#include <stdio.h>
#include "punit.h"
#include "unixunit.h"

static void setup(void)
{
    uu_open_pipe();
}

static void teardown(void)
{
    uu_close_pipe();
}

static char * test_stdin_ok(void)
{
    char * input[] = {"1\n", "2\n"};
    int ret_val1, ret_val2;

    /* Write input values to stdin */
    uu_open_stdin_writer();
    uu_write_stdin(input[0]);
    uu_write_stdin(input[1]);
    uu_close_stdin_writer();

    /* Get input from stdin */
    scanf("%i", &ret_val1);
    scanf("%i", &ret_val2);

    pu_assert_equal("First value read from stdin", ret_val1, 1);
    pu_assert_equal("Second value read from stdin", ret_val2, 2);

    return NULL;
}

static char * test_stdin_fail(void)
{
    char * input[] = {"2\n", "3\n"};
    int ret_val1, ret_val2;

    /* Write input values to stdin */
    uu_open_stdin_writer();
    uu_write_stdin(input[0]);
    uu_write_stdin(input[1]);
    uu_close_stdin_writer();

    /* Get input from stdin */
    scanf("%i", &ret_val1);
    scanf("%i", &ret_val2);

    pu_assert_equal("First value read from stdin", ret_val1, 1);
    pu_assert_equal("Second value read from stdin", ret_val2, 2);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_stdin_ok, PU_RUN);
    pu_def_test(test_stdin_fail, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
