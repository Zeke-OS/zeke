/**
 * @file test_queue.c
 * @brief Test generic thread-safe queue implementation.
 */

#include <stdio.h>
#include "punit.h"
#include <generic/dllist.h>

typedef struct {
    int a;
    int b;
    llist_nodedsc_t llist_node;
} tst_t;

llist_t * lst;

static void setup()
{
    lst = dllist_create(tst_t);
}

static void teardown()
{
    dllist_destroy(lst);
}

static char * test_insert_head(void)
{
    tst_t * x;

    pu_assert("List created.", lst != 0);
    x = malloc(sizeof(tst_t));
    pu_assert("List node allocated.", x != 0);

    lst->insert_head(lst, x);

    pu_assert_equal("Node inserted as head.", lst->head, x);

    return 0;
}

static void all_tests() {
    pu_def_test(test_insert_head, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

