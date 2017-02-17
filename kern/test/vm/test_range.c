/**
 * @file test_bio.c
 * @brief Test buffered IO.
 */

#include <kunit.h>
#include <libkern.h>
#include <vm/vm.h>

static void setup(void)
{
}

static void teardown(void)
{
}


static char * test_addr_is_in_range(void)
{
    ku_assert("", VM_ADDR_IS_IN_RANGE(0, 0, 10) == !0);
    ku_assert("", VM_ADDR_IS_IN_RANGE(5, 0, 10) == !0);
    ku_assert("", VM_ADDR_IS_IN_RANGE(10, 0, 10) == !0);
    ku_assert("", VM_ADDR_IS_IN_RANGE(20, 0, 10) == 0);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_addr_is_in_range, KU_RUN);
}

TEST_MODULE(vm, range);
