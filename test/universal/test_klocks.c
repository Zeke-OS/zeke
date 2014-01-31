/**
 * @file test_klocks.c
 * @brief Test kernel locks.
 */

#include <stdio.h>
#include <stdint.h>
#include "punit.h"
#define KERNEL_INTERNAL
#include <sys/_mutex.h>

static void setup()
{
}

static void teardown()
{
}

static char * test_mtx_init()
{
    mtx_t mtx;

    mtx_init(&mtx);
    pu_assert_equal("", mtx.mtx_lock, 0);

    return 0;
}

/* ALL TESTS *****************************************************************/

static void all_tests() {
    pu_def_test(test_mtx_init, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
