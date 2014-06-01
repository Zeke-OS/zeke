/**
 * @file test_kmallic.c
 * @brief Test kmalloc functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include_next <time.h>
#include <punit.h>
#include <kmalloc.h>
#include "sim_kmheap.h"

static void rnd_allocs(int n);
static int unirand(int n);

static void setup()
{
    setup_kmalloc();
}

static void teardown()
{
    teardown_kmalloc();
}

static char * kmalloc_simple(void)
{
    void * p = kmalloc(100);

    pu_assert_not_null("No error on allocation", p);
    pu_assert_ptr_equal("Allocated 100 bytes just after the first descriptor", p, block(a, 0)->data);

    return 0;
}

static char * test_krealloc(void)
{
    size_t i;
    void * p = 0;

    for (i = 1; i < 100; i++) {
        p = krealloc(p, i * 80);
        pu_assert("", p != 0);
    }

    return 0;
}

static char * test_krealloc_multi(void)
{
    size_t i;
    void * p1 = 0;
    void * p2 = 0;
    void * p3 = 0;

    for (i = 1; i < 100; i++) {
        p1 = krealloc(p1, i * 80);
        pu_assert("", p1 != 0);
        p3 = kmalloc(15);
        pu_assert("", p3 != 0);
        p2 = krealloc(p2, i * 30);
        pu_assert("", p2 != 0);
    }

    return 0;
}

static void all_tests() {
    pu_def_test(kmalloc_simple, PU_RUN);
    pu_def_test(test_krealloc, PU_RUN);
    pu_def_test(test_krealloc_multi, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

static int unirand(int n)
{
    int partSize = (n == RAND_MAX) ? 1 : 1 + (RAND_MAX - n)/(n + 1);
    int maxUsefull = partSize * n + (partSize - 1);
    int draw;
    do {
        draw = rand();
    } while (draw > maxUsefull);

    return draw/partSize;
}

