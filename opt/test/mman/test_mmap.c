#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include "punit.h"

static void setup()
{
}

static void teardown()
{
}

static char * test_mmap_anon(void)
{
    void * data;

    errno = 0;
    data = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

    pu_assert("a new memory region returned", data != NULL);
    pu_assert_equal("No errno was set", errno, 0);

    return NULL;
}

static char * test_mmap_anon_fixed(void)
{
#define ADDR ((void *)0xA0000000)
    void * data;
    static char msg[80];
    int errno_save;

    errno = 0;
    data = mmap(ADDR, 4096, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED, -1, 0);
    errno_save = errno;

    pu_assert("a new memory region returned", data != NULL);
    pu_assert_equal("No errno was set", errno_save, 0);
    sprintf(msg, "returned address equals %p", ADDR);
    pu_assert(msg, data == ADDR);

    return NULL;
#undef ADDR
}

static void all_tests()
{
    pu_run_test(test_mmap_anon);
    pu_run_test(test_mmap_anon_fixed);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

