/**
 * @file test_heap.c
 * @brief Test heap used for scheduling.
 */

#include <stdio.h>
#include "punit.h"
#include "heap.h"

static void setup()
{
}

static void teardown()
{
}

static char * test_heap_insert(void)
{
    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread;
    thread.priority = 1;

    heap_insert(&heap, &thread);
    pu_assert_equal("error, 1 not inserted", heap.a[0]->priority, 1);

    return 0;
}

static char * test_heap_del_max(void)
{
    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread1;
    threadInfo_t thread2;

    thread1.priority = 1;
    thread2.priority = 2;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);
    pu_assert_equal("error, heap doesn't sort inserts correctly",
              heap.a[0]->priority, 2);

    heap_del_max(&heap);
    pu_assert_equal("error, wrong key was removed from the heap",
              heap.a[0]->priority, 1);

    return 0;
}

static char * test_heap_inc_key(void)
{
heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread1;
    threadInfo_t thread2;
    threadInfo_t thread3;

    thread1.priority = -1;
    thread2.priority = 10;
    thread3.priority = 5;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);
    heap_insert(&heap, &thread3);

    thread3.priority = 15;
    heap_inc_key(&heap, 1);

    pu_assert_equal("error, wrong key on top after heap_inc_key",
              heap.a[0]->priority, 15);

    return 0;
}

static char * test_heap_dec_key(void)
{
    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread1;
    threadInfo_t thread2;

    thread1.priority = 5;
    thread2.priority = 10;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);

    thread2.priority = -1;
    heap_dec_key(&heap, 0);

    pu_assert_equal("error, wrong key on top after heap_dec_key",
              heap.a[0]->priority, 5);

    return 0;
}

static char * test_shuffled_heap(void)
{
    int i;

    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread1;
    threadInfo_t thread2;
    threadInfo_t thread3;

    thread1.priority = 5;
    thread2.priority = 10;
    thread3.priority = 11;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);
    thread1.priority = 15;
    heap_insert(&heap, &thread3);

    /** TODO Do some tests with shuffled heap */

    return 0;
}

    /** TODO test heap with multiple insertion of the same thread */

static void all_tests() {
    pu_def_test(test_heap_insert, PU_RUN);
    pu_def_test(test_heap_del_max, PU_RUN);
    pu_def_test(test_heap_inc_key, PU_RUN);
    pu_def_test(test_heap_dec_key, PU_RUN);
    pu_def_test(test_shuffled_heap, PU_SKIP);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
