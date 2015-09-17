/* file test_null.c */

#include <stdio.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_ok(void)
{
    void * ptr1 = NULL;
    void * ptr2 = &ptr1;

    pu_assert_null("ptr1 is null", ptr1);
    pu_assert_not_null("ptr2 is not null", ptr2);

    return NULL;
}

static char * test_fail1(void)
{
    char a = 'a';
    void * ptr1 = &a;
    void * ptr2 = NULL;

    pu_assert_null("ptr1 is null", ptr1);
    pu_assert_not_null("ptr2 is not null", ptr2);

    return NULL;
}

static char * test_fail2(void)
{
    void * ptr2 = NULL;

    pu_assert_not_null("ptr2 is not null", ptr2);

    return NULL;
}


static void all_tests(void)
{
    pu_def_test(test_ok, PU_RUN);
    pu_def_test(test_fail1, PU_RUN);
    pu_def_teast(test_fail2, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
