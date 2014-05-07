/**
 * @file test_klocks.c
 * @brief Test kernel locks.
 */

#include <stdio.h>
#include <stdint.h>
#include "punit.h"
#define KERNEL_INTERNAL
#include <klocks.h>

static mtx_t mtx;

static void setup()
{
    mtx_init(&mtx, MTX_DEF | MTX_SPIN);
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
    pu_assert("Spinlock achieved", mtx_spinlock(&mtx) == 0);
    pu_assert("Lock value is correct", mtx.mtx_lock != 0);

    mtx_init(&mtx, MTX_DEF);
    pu_assert("Spinlock is not allowed", mtx_spinlock(&mtx) == 3);

    return 0;
}

static char * test_mtx_unlock()
{
    pu_assert("Spinlock achieved", mtx_spinlock(&mtx) == 0);
    pu_assert("Lock value is correct", mtx.mtx_lock != 0);
    mtx_unlock(&mtx);
    pu_assert_equal("Lock value is correct", mtx.mtx_lock, 0);

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
