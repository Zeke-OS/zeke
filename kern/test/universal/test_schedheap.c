/**
 * @file test_schedheap.c
 * @brief Test heap used for scheduling.
 */

#include <stdio.h>
#include <punit.h>
#include <sched_tiny/heap.h>

static void print_heap(heap_t heap);

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

static void print_heap(heap_t heap)
{
    int i;

    for (i = 0; i <= heap.size; i++)
        printf("%i(%i), ", heap.a[i]->id, heap.a[i]->priority);
    printf("\n");
}

static char * test_heap_reschedule(void)
{
    heap_t heap = HEAP_NEW_EMPTY;
    threadInfo_t thread1, thread2, thread3, thread4, thread5;

    thread1.priority = osPriorityAboveNormal;
    thread1.id = 1;
    thread2.priority = osPriorityHigh;
    thread2.id = 2;
    thread3.priority = osPriorityLow;
    thread3.id = 3;
    thread4.priority = osPriorityLow;
    thread4.id = 4;
    thread5.priority = osPriorityIdle;
    thread5.id = 5;

    heap_insert(&heap, &thread1);
    heap_insert(&heap, &thread2);
    heap_insert(&heap, &thread3);
    heap_insert(&heap, &thread4);
    heap_insert(&heap, &thread5);

    //print_heap(heap);

    heap_reschedule_root(&heap, osPriorityLow);

    //print_heap(heap);

    pu_assert_equal("error, root should be now thread1",
        heap.a[0]->id, 1);

    pu_assert_equal("error, thread1 priority is incorrect",
        heap.a[0]->priority, osPriorityAboveNormal);

    heap_del_max(&heap);

    //print_heap(heap);

    pu_assert("error, thread2 should not pop at least as a second thread on the queue of threads with same priority",
        heap.a[0]->id != thread2.id);

    heap_del_max(&heap);
    heap_del_max(&heap);
    heap_del_max(&heap);

    pu_assert_equal("error, thread5 should be the last one to pop",
        heap.a[0]->id, thread5.id);

    return 0;
}

static char * test_shuffled_heap(void)
{
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
    pu_def_test(test_heap_reschedule, PU_RUN);
    pu_def_test(test_shuffled_heap, PU_SKIP);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
