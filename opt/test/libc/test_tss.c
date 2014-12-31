#include <stdio.h>
#include <threads.h>
#include "punit.h"

static tss_t key;
static char v;

static void setup()
{
    memset(&key, 0, sizeof(key));
    v = '\0';
}

static void teardown()
{
    tss_delete(key);
}

static char * test_tss_get()
{
    pu_assert("can create a key", tss_create(&key, NULL) == thrd_success);
    pu_assert("key is set to NULL", tss_get(key) == NULL);
    pu_assert("can set key value", tss_set(key, &v) == thrd_success);
    pu_assert("can get key value", tss_get(key) == &v);

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_tss_get);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

