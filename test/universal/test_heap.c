/**
 * @file test_heap.c
 *
 */

#include <stdio.h>
#include "minunit.h"
#include "heap.h"

static char * test_heap_insert(void)
{
    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread;
    thread.priority = 1;

    heap_insert(&heap, &thread);
    mu_assert("error, 1 not inserted", heap.a[0]->priority == 1);

    return 0;
}

static char * test_heap_del_max(void)
{
    heap_t heap = HEAP_NEW_EMPTY;

    threadInfo_t thread1;
    thread1.priority = 1;

    threadInfo_t thread2;
    thread2.priority = 2;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);
    mu_assert("error, heap doesn't sort inserts correctly",
              heap.a[0]->priority == 2);

    heap_del_max(&heap);
    mu_assert("error, wrong key was removed from the heap",
              heap.a[0]->priority == 1);

    return 0;
}

static char * all_tests() {
    mu_run_test(test_heap_insert);
    mu_run_test(test_heap_del_max);
    return 0;
}

int main(int argc, char **argv)
{
    return mu_run_tests(&all_tests);
}
