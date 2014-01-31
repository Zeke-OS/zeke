/**
 * @file test_klocks.c
 * @brief Test kernel locks.
 */

#include <stdio.h>
#include <stdint.h>
#include "punit.h"
#define KERNEL_INTERNAL
#include <sys/_mutex.h>

static mtx_t mtx;

static void setup()
{
    mtx_init(&mtx);
}

static void teardown()
{
}

static char * test_mtx_init()
{
    pu_assert_equal("", mtx.mtx_lock, 0);

    return 0;
}

static char * test_mtx_spinlock()
{
    pu_assert("", mtx_spinlock(&mtx) == 0);
    pu_assert("", mtx.mtx_lock != 0);

    return 0;
}

static char * test_mtx_unlock()
{
    pu_assert("", mtx_spinlock(&mtx) == 0);
    pu_assert("", mtx.mtx_lock != 0);
    mtx_unlock(&mtx);
    pu_assert_equal("", mtx.mtx_lock, 0);

    return 0;
}

static char * test_mtx_trylock()
{
    pu_assert("", mtx_trylock(&mtx) == 0);
    pu_assert("", mtx_trylock(&mtx) != 0);
    mtx_unlock(&mtx);
    pu_assert("", mtx_trylock(&mtx) == 0);

    return 0;
}

/* ALL TESTS *****************************************************************/

static void all_tests() {
    pu_def_test(test_mtx_init, PU_RUN);
    pu_def_test(test_mtx_spinlock, PU_RUN);
    pu_def_test(test_mtx_unlock, PU_RUN);
    pu_def_test(test_mtx_trylock, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
