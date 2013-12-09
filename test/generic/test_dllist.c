/**
 * @file test_queue.c
 * @brief Test generic thread-safe queue implementation.
 */

#include <stdio.h>
#include <stdlib.h>
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
    lst = dllist_create(tst_t, llist_node);
}

static void teardown()
{
    dllist_destroy(lst);
}

static char * test_insert_head(void)
{
    tst_t * x1;
    tst_t * x2;

    pu_assert("List created.", lst != 0);
    x1 = malloc(sizeof(tst_t));
    x2 = malloc(sizeof(tst_t));
    pu_assert("List node allocated.", x1 != 0);
    pu_assert("List node allocated.", x2 != 0);

    lst->insert_head(lst, x1);
    lst->insert_head(lst, x2);

    pu_assert_ptr_equal("Node x2 inserted as head.", lst->head, x2);
    pu_assert_ptr_equal("Node x1 is tail.", lst->tail, x1);

    pu_assert_ptr_equal("Node x2->next == x1", x2->llist_node.next, x1);
    pu_assert_ptr_equal("Node x2->prev == null", x2->llist_node.prev, 0);
    pu_assert_ptr_equal("Node x1->next == null", x1->llist_node.next, 0);
    pu_assert_ptr_equal("Node x1->prev == x2", x1->llist_node.prev, x2);

    return 0;
}

static char * test_insert_tail(void)
{
    tst_t * x1;
    tst_t * x2;

    pu_assert("List created.", lst != 0);
    x1 = malloc(sizeof(tst_t));
    x2 = malloc(sizeof(tst_t));
    pu_assert("List node allocated.", x1 != 0);
    pu_assert("List node allocated.", x2 != 0);

    lst->insert_tail(lst, x1);
    lst->insert_tail(lst, x2);

    pu_assert_ptr_equal("Node x1 inserted as head.", lst->head, x1);
    pu_assert_ptr_equal("Node x2 is tail.", lst->tail, x2);

    printf("%i, %i\n", x1, x2);
    pu_assert_ptr_equal("Node x1->next == x2", x1->llist_node.next, x2);
    pu_assert_ptr_equal("Node x1->prev == null", x1->llist_node.prev, 0);
    pu_assert_ptr_equal("Node x2->next == null", x2->llist_node.next, 0);
    pu_assert_ptr_equal("Node x2->prev == x1", x2->llist_node.prev, x1);

    return 0;
}

static void all_tests() {
    pu_def_test(test_insert_head, PU_RUN);
    pu_def_test(test_insert_tail, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

