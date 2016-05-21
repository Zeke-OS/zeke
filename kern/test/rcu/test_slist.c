/**
 * @file test_slist.c
 * @brief Test RCU slists.
 */

#include <errno.h>
#include <kunit.h>
#include <ktest_mib.h>
#include <libkern.h>
#include <buf.h>
#include <kmalloc.h>
#include <thread.h>
#include <rcu.h>

struct data {
    int x;
    struct rcu_cb rcu;
};

static struct rcu_slist_head list_head;

static void setup(void)
{
    memset(&list_head, 0, sizeof(list_head));
}

static void teardown(void)
{
    struct rcu_cb * n;

    while ((n = rcu_slist_remove_head(&list_head))) {
        kfree(containerof(n, struct data, rcu));
    }
}

static struct data * alloc_data(void)
{
    struct data * p;

    p = kmalloc(sizeof(struct data));
    if (!p)
        return NULL;
    memset(p, 1, sizeof(struct data));

    return p;
}


static char * test_rcu_slist_insert_head(void)
{
    struct data * p = alloc_data();

    ku_assert("", p);

    rcu_slist_insert_head(&list_head, &p->rcu);
    ku_assert_ptr_equal("Head pointer", list_head.head, &p->rcu);
    ku_assert_ptr_equal("Next pointer", p->rcu.next, NULL);

    return NULL;
}

static char * test_rcu_slist_insert_head_multi(void)
{
    struct data * tail;

    for (int i = 0; i < 3; i++) {
        struct data * p = alloc_data();
        if (i == 0)
            tail = p;

        ku_assert("", p);
        rcu_slist_insert_head(&list_head, &p->rcu);
    }

    for (int i = 0; i < 3; i++) {
        struct rcu_cb * rp = rcu_slist_remove_head(&list_head);

        ku_assert("P is a valid pointer", rp);
        if (i == 2) {
            ku_assert_ptr_equal("Tail ptr is valid", rp, &tail->rcu);
        }
        kfree(containerof(rp, struct data, rcu));
    }

    return NULL;
}

static char * test_rcu_slist_insert_after(void)
{
    struct data * n1;
    struct data * n2 = alloc_data();

    ku_assert("", n2);

    for (int i = 0; i < 3; i++) {
        struct data * p = alloc_data();

        ku_assert("", p);
        rcu_slist_insert_head(&list_head, &p->rcu);
        if (i == 1)
            n1 = p;
    }

    rcu_slist_insert_after(&n1->rcu, &n2->rcu);

    for (int i = 0; i < 4; i++) {
        struct rcu_cb * rp = rcu_slist_remove_head(&list_head);

        ku_assert("P is a valid pointer", rp);
        if (i == 2) {
            ku_assert_ptr_equal("n2 is in correct position", rp, &n2->rcu);
        }
        kfree(containerof(rp, struct data, rcu));
    }

    return NULL;
}

static char * test_rcu_slist_insert_tail(void)
{
    struct data * p1 = alloc_data();
    struct data * p2 = alloc_data();
    struct rcu_cb * n;

    ku_assert("", p1 && p2);
    rcu_slist_insert_tail(&list_head, &p1->rcu);
    rcu_slist_insert_tail(&list_head, &p2->rcu);

    n = rcu_slist_remove_head(&list_head);
    ku_assert_ptr_equal("", n, &p1->rcu);
    kfree(p1);
    n = rcu_slist_remove_head(&list_head);
    ku_assert_ptr_equal("", n, &p2->rcu);
    kfree(p2);

    return NULL;
}

static char * test_rcu_slist_remove_head_null(void)
{
    struct rcu_cb * n;

    n = rcu_slist_remove_head(&list_head);
    ku_assert_ptr_equal("", n, NULL);

    return NULL;
}

static char * test_rcu_slist_remove(void)
{
    struct data * p1 = alloc_data();
    struct data * p2 = alloc_data();
    struct data * p3 = alloc_data();
    struct rcu_cb * n;

    ku_assert("", p1 && p2 && p3);

    rcu_slist_insert_head(&list_head, &p3->rcu);
    rcu_slist_insert_head(&list_head, &p2->rcu);
    rcu_slist_insert_head(&list_head, &p1->rcu);

    n = rcu_slist_remove(&list_head, &p2->rcu);
    ku_assert_ptr_equal("The correct entry was removed", n, &p2->rcu);
    kfree(p2);

    n = rcu_slist_remove_head(&list_head);
    ku_assert_ptr_equal("The correct entry was removed", n, &p1->rcu);
    kfree(p1);

    n = rcu_slist_remove_head(&list_head);
    ku_assert_ptr_equal("The correct entry was removed", n, &p3->rcu);
    kfree(p3);

    return NULL;
}

static char * test_rcu_slist_remove_invalid(void)
{
    struct data * p1 = alloc_data();
    struct data * p2 = alloc_data();
    struct data * p3 = alloc_data();
    struct rcu_cb * n;

    ku_assert("", p1 && p2 && p3);

    rcu_slist_insert_head(&list_head, &p3->rcu);
    rcu_slist_insert_head(&list_head, &p1->rcu);

    n = rcu_slist_remove(&list_head, &p2->rcu);
    ku_assert_ptr_equal("Should be NULL", n, NULL);
    kfree(p2);

    return NULL;
}

static char * test_rcu_slist_remove_tail(void)
{
    struct data * p1 = alloc_data();
    struct data * p2 = alloc_data();
    struct rcu_cb * n;

    ku_assert("", p1 && p2);

    rcu_slist_insert_head(&list_head, &p2->rcu);
    rcu_slist_insert_head(&list_head, &p1->rcu);

    n = rcu_slist_remove_tail(&list_head);
    ku_assert_ptr_equal("The tail entry was removed", n, &p2->rcu);
    kfree(p2);

    return NULL;
}

static char * test_rcu_slist_remove_tail_null(void)
{
    struct rcu_cb * n;

    n = rcu_slist_remove_tail(&list_head);
    ku_assert_ptr_equal("", n, NULL);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_rcu_slist_insert_head, KU_RUN);
    ku_def_test(test_rcu_slist_insert_head_multi, KU_RUN);
    ku_def_test(test_rcu_slist_insert_after, KU_RUN);
    ku_def_test(test_rcu_slist_insert_tail, KU_RUN);
    ku_def_test(test_rcu_slist_remove_head_null, KU_RUN);
    ku_def_test(test_rcu_slist_remove, KU_RUN);
    ku_def_test(test_rcu_slist_remove_invalid, KU_RUN);
    ku_def_test(test_rcu_slist_remove_tail, KU_RUN);
    ku_def_test(test_rcu_slist_remove_tail_null, KU_RUN);
}

SYSCTL_TEST(rcu, slist);
