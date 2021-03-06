/**
 * @file test_basic.c
 * @brief Test basic RCU functionality.
 */

#include <errno.h>
#include <kunit.h>
#include <libkern.h>
#include <buf.h>
#include <kmalloc.h>
#include <thread.h>
#include <rcu.h>

struct data {
    int x;
    struct rcu_cb rcu;
};

static struct data * gptr;

static void setup(void)
{
    gptr = NULL;
}

static void teardown(void)
{
    kfree(gptr);
}

static char * test_rcu_assign_pointer_and_deference(void)
{
    struct data * p;

    p = kmalloc(sizeof(struct data));
    if (!p)
        ku_assert_fail("ENOMEM");

    rcu_assign_pointer(gptr, p);
    ku_assert_ptr_equal("gptr is set correctly", gptr, p);
    p = NULL;
    p = rcu_dereference(gptr);
    ku_assert_ptr_equal("gptr is dereferenced correctly", gptr, p);

    return NULL;
}

static void * rcu_reader_thread(void * arg)
{
    struct rcu_lock_ctx ctx = rcu_read_lock();
    struct data * rd = rcu_dereference(gptr);
    for (int i = 0; i < 10; i++) {
        READ_ONCE(rd);
        /* This is the beef of the RCU implementation in Zeke */
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }
    rcu_read_unlock(&ctx);

    return NULL;
}

static pthread_t create_rcu_reader_thread(void)
{
    struct sched_param param = {
        .sched_policy = SCHED_OTHER,
        .sched_priority = NZERO,
    };

    pthread_t tid = kthread_create("rcu_test", &param, 0,
                                   rcu_reader_thread, NULL);
    if (tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a thread\n");
    }
    return tid;
}

static char * test_rcu_synchronize(void)
{
    pthread_t tid;

    struct data * const p1 = kmalloc(sizeof(struct data));
    struct data * const p2 = kmalloc(sizeof(struct data));
    if (!p1 || !p2) {
        kfree(p1);
        kfree(p2);
        ku_assert_fail("ENOMEM");
    }

    rcu_assign_pointer(gptr, p1);
    tid = create_rcu_reader_thread();
    ku_assert("tid is valid", tid > 0);
    rcu_assign_pointer(gptr, p2);
    rcu_synchronize();
    ku_assert_ptr_equal("gptr is valid", gptr, p2);
    kfree(p1);
    thread_terminate(tid); /* We must kill the kernel thread before we exit. */

    return NULL;
}

static void rcu_test_callback(struct rcu_cb * cb)
{
    KERROR(KERROR_INFO, "RCU test callback called\n");
    kfree(containerof(cb, struct data, rcu));
    KERROR(KERROR_INFO, "RCU test callback done\n");
}

static char * test_rcu_callback(void)
{
    pthread_t tid;

    struct data * const p1 = kmalloc(sizeof(struct data));
    struct data * const p2 = kmalloc(sizeof(struct data));
    if (!p1 || !p2) {
        kfree(p1);
        kfree(p2);
        ku_assert_fail("ENOMEM");
    }

    rcu_assign_pointer(gptr, p1);
    tid = create_rcu_reader_thread();
    ku_assert("tid is valid", tid > 0);
    rcu_assign_pointer(gptr, p2);
    rcu_call(&p1->rcu, rcu_test_callback);
    ku_assert_ptr_equal("gptr is valid", gptr, p2);
    thread_sleep(5000);
    thread_terminate(tid);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_rcu_assign_pointer_and_deference, KU_RUN);
    ku_def_test(test_rcu_synchronize, KU_RUN);
    ku_def_test(test_rcu_callback, KU_RUN);
}

TEST_MODULE(rcu, basic);
