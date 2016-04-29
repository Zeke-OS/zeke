#include <sys/time.h>
#include <ktest_mib.h>
#include <kunit.h>
#include <libkern.h>

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_mktimespec(void)
{
    struct tm tm = {
        .tm_sec = 1,
        .tm_min = 20,
        .tm_hour = 4,
        .tm_mday = 20,
        .tm_mon = 3,
        .tm_year = 92,
        /* wday, yday and dst are ignored for now. */
    };
    struct timespec ts;

    mktimespec(&ts, &tm);

    ku_assert_equal("ts equal to tm", (int)ts.tv_sec, 703743601);

    return NULL;
}

static char * test_nsec2timespec(void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };

    nsec2timespec(&ts, 1500000000);
    ku_assert_equal("1 sec", (int)ts.tv_sec, 1);
    ku_assert_equal("500000000 nsec", (int)ts.tv_nsec, 500000000);

    return NULL;
}

static char * test_timespec_add(void)
{
    struct timespec val = {
        .tv_sec = 1,
        .tv_nsec = 500000001,
    };
    struct timespec res;

    timespec_add(&res, &val, &val);
    ku_assert_equal("3 sec", (int)res.tv_sec, 3);
    ku_assert_equal("2 nsec", (int)res.tv_nsec, 2);

    return NULL;
}

static char * test_timespec_sub(void)
{
    struct timespec a = {
        .tv_sec = 3,
        .tv_nsec = 0,
    };
    struct timespec b = {
        .tv_sec = 1,
        .tv_nsec = 500000000,
    };
    struct timespec res;

    timespec_sub(&res, &a, &b);
    ku_assert_equal("1 sec", (int)res.tv_sec, 1);
    ku_assert_equal("500000000 nsec", (int)res.tv_nsec, 500000000);

    return NULL;
}

static char * test_timespec_mul(void)
{
    struct timespec val = {
        .tv_sec = 2,
        .tv_nsec = 200,
    };
    struct timespec res;

    timespec_mul(&res, &val, &val);
    ku_assert_equal("4 sec", (int)res.tv_sec, 4);
    ku_assert_equal("40000 nsec", (int)res.tv_nsec, 40000);

    return NULL;
}

static char * test_timespec_div(void)
{
    struct timespec a = {
        .tv_sec = 4,
        .tv_nsec = 0,
    };
    struct timespec b = {
        .tv_sec = 2,
        .tv_nsec = 1,
    };
    struct timespec res;

    timespec_div(&res, &a, &b);
    ku_assert_equal("2 sec", (int)res.tv_sec, 2);
    ku_assert_equal("0 nsec", (int)res.tv_nsec, 0);

    return NULL;
}

static char * test_timespec_mod(void)
{
    struct timespec a = {
        .tv_sec = 5,
        .tv_nsec = 0,
    };
    struct timespec b = {
        .tv_sec = 4,
        .tv_nsec = 1,
    };
    struct timespec res;

    timespec_mod(&res, &a, &b);
    ku_assert_equal("1 sec", (int)res.tv_sec, 1);
    ku_assert_equal("0 nsec", (int)res.tv_nsec, 0);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_mktimespec, KU_RUN);
    ku_def_test(test_nsec2timespec, KU_RUN);
    ku_def_test(test_timespec_add, KU_RUN);
    ku_def_test(test_timespec_sub, KU_RUN);
    ku_def_test(test_timespec_mul, KU_RUN);
    ku_def_test(test_timespec_div, KU_RUN);
    ku_def_test(test_timespec_mod, KU_RUN);
}

SYSCTL_TEST(generic, ctime);
