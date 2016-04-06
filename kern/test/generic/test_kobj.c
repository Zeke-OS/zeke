#include <kobj.h>
#include <ktest_mib.h>
#include <kunit.h>
#include <libkern.h>

static struct my_obj {
    struct kobj ko;
} o;
static int freed;

static void my_free(struct kobj * p)
{
    freed = 1;
}

static void setup(void)
{
    kobj_init(&o.ko, my_free);
}

static void teardown(void)
{
    freed = 0;
}

static char * test_init(void)
{
    ku_assert_ptr_equal("free ptr set", o.ko.ko_free, my_free);
    ku_assert_equal("fast lock init", o.ko.ko_fast_lock, 0);
    ku_assert_equal("refcount init", o.ko.ko_refcount, 1);

    return NULL;
}

static char * test_ref(void)
{
    int err;

    err = kobj_ref(&o.ko);
    ku_assert_equal("ref ok", err, 0);
    ku_assert_equal("refcount incremented", o.ko.ko_refcount, 2);

    return NULL;
}

static char * test_unref(void)
{
    int err;

    err = kobj_ref(&o.ko);
    ku_assert_equal("ref ok", err, 0);
    kobj_unref(&o.ko);
    ku_assert_equal("fast lock init", o.ko.ko_refcount, 1);

    return NULL;
}

static char * test_refcnt(void)
{
    int err;

    ku_assert("refcnt ok", kobj_refcnt(&o.ko) == 1);
    err = kobj_ref(&o.ko);
    ku_assert_equal("ref ok", err, 0);
    ku_assert("refcnt incr", kobj_refcnt(&o.ko) == 2);
    kobj_unref(&o.ko);
    ku_assert("refcnt decr", kobj_refcnt(&o.ko) == 1);

    return NULL;
}

static char * test_free(void)
{
    int err;

    kobj_unref(&o.ko);
    err = kobj_ref(&o.ko);
    ku_assert("ref fails", err < 0);

    return NULL;
}

static char * test_destroy(void)
{
    int err;

    err = kobj_ref(&o.ko);
    ku_assert_equal("ref ok", err, 0);
    kobj_destroy(&o.ko);
    err = kobj_ref(&o.ko);
    ku_assert("ref failed", err != 0);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_init, KU_RUN);
    ku_def_test(test_ref, KU_RUN);
    ku_def_test(test_unref, KU_RUN);
    ku_def_test(test_refcnt, KU_RUN);
    ku_def_test(test_free, KU_RUN);
    ku_def_test(test_destroy, KU_RUN);
}

SYSCTL_TEST(generic, kobj);
