/**
 * @file test_queue.c
 * @brief Test generic thread-safe queue implementation.
 */

#include <kunit.h>
#include <queue_r.h>

int tarr[5];
queue_cb_t queue;

static void setup(void)
{
    size_t i;

    for (i = 0; i < sizeof(tarr) / sizeof(int); i++) {
        tarr[i] = 0;
    }

    queue = queue_create(&tarr, sizeof(int), sizeof(tarr));
}

static void teardown(void)
{
    queue_clear_from_push_end(&queue);
}

static char * test_queue_single_push(void)
{
    int x = 5;
    int err;

    err = queue_push(&queue, &x);
    ku_assert("error, push failed", err != 0);
    ku_assert_equal("error, value of x was not pushed to the first index", tarr[0], x);

    return NULL;
}

static char * test_queue_single_pop(void)
{
    int x = 5;
    int y;
    int err;

    err = queue_push(&queue, &x);
    ku_assert("error, push failed", err != 0);

    err = queue_pop(&queue, &y);
    ku_assert("error, pop failed", err != 0);
    ku_assert_equal("Returned value is same as pushed", x, y);

    return NULL;
}

static char * test_queue_pop_fail(void)
{
    int y;
    int err;

    err = queue_pop(&queue, &y);
    ku_assert("pop should fail", err == 0);

    return NULL;
}

static char * test_queue_peek_ok(void)
{
    int x = 5;
    int *xp = NULL;
    int err;

    err = queue_push(&queue, &x);
    ku_assert("error, push failed", err != 0);

    err = queue_peek(&queue, (void **)&xp);
    ku_assert("peek is ok", err != 0);
    ku_assert("xp should be set", xp != NULL);
    ku_assert_equal("Value of *xp is valid", *xp, x);

    return NULL;
}

static char * test_queue_peek_fail(void)
{
    int *xp = NULL;
    int err;

    err = queue_peek(&queue, (void **)&xp);
    ku_assert("peek should fail due to an empty queue", err == 0);

    return NULL;
}

static char * test_queue_skip_one(void)
{
    int x = 0;
    int err, ret;

    err = queue_push(&queue, &x);
    ku_assert("error, push failed", err != 0);

    ret = queue_skip(&queue, 1);
    ku_assert_equal("One element skipped", ret, 1);

    return NULL;
}

static char * test_queue_alloc(void)
{
    int * p;
    int y;
    int err;

    p = queue_alloc_get(&queue);
    ku_assert("Alloc not null", p != NULL);

    *p = 5;

    err = queue_pop(&queue, &y);
    ku_assert("pop should fail", err == 0);

    queue_alloc_commit(&queue);

    err = queue_pop(&queue, &y);
    ku_assert("error, pop failed", err != 0);
    ku_assert_equal("Returned value is same as pushed", 5, y);

    return NULL;
}

static char * test_queue_is_empty(void)
{
    ku_assert("Queue is empty", queue_isempty(&queue) != 0);

    return NULL;
}

static char * test_queue_is_not_empty(void)
{
    int x = 1;

    queue_push(&queue, &x);
    ku_assert("Queue is empty", queue_isempty(&queue) == 0);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_queue_single_push, KU_RUN);
    ku_def_test(test_queue_single_pop, KU_RUN);
    ku_def_test(test_queue_pop_fail, KU_RUN);
    ku_def_test(test_queue_peek_ok, KU_RUN);
    ku_def_test(test_queue_peek_fail, KU_RUN);
    ku_def_test(test_queue_skip_one, KU_RUN);
    ku_def_test(test_queue_alloc, KU_RUN);
    ku_def_test(test_queue_is_empty, KU_RUN);
    ku_def_test(test_queue_is_not_empty, KU_RUN);
}

TEST_MODULE(generic, queue);
