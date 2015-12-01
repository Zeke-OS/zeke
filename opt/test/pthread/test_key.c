#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "punit.h"

static pthread_key_t key;
static int key_deleted;
static int retval;

static void setup(void)
{
    retval = pthread_key_create(&key, NULL);
}

static void teardown(void)
{
    if (!key_deleted) {
        pthread_key_delete(key);
    }
}

static char * test_key_create(void)
{
    pu_assert_equal("A key was created succesfully", retval, 0);

    retval = pthread_key_delete(key);
    key_deleted = !0;
    pu_assert_equal("The key was deleted", retval, 0);

    return NULL;
}

static char * test_getspecific_null(void)
{
    void * val;

    val = pthread_getspecific(key);
    pu_assert("Value should be equal to NULL", val == NULL);

    return NULL;
}

static char * test_setspecific(void)
{
    void * val;

    retval = pthread_setspecific(key, (void *)1);
    pu_assert_equal("", retval, 0);

    val = pthread_getspecific(key);
    pu_assert_ptr_equal("val should be equal to what was set", val, (void *)1);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_key_create, PU_RUN);
    pu_def_test(test_getspecific_null, PU_RUN);
    pu_def_test(test_setspecific, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
