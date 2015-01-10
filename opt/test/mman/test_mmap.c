#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "punit.h"

char * data;

static void setup()
{
    data = NULL;
}

static void teardown()
{
    if (data) {
        munmap(data, 0);
    }
}

static char * test_mmap_anon(void)
{
    errno = 0;
    data = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

    pu_assert("a new memory region returned", data != MAP_FAILED);
    pu_assert_equal("No errno was set", errno, 0);

    memset(data, 0xff, 200);
    pu_assert("memory is accessible", data[50] == 0xff);

    return NULL;
}

static char * test_mmap_anon_fixed(void)
{
#define ADDR ((void *)0xA0000000)
    static char msg[80];
    int errno_save;

    errno = 0;
    data = mmap(ADDR, 4096, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED, -1, 0);
    errno_save = errno;

    pu_assert("a new memory region returned", data != MAP_FAILED);
    pu_assert_equal("No errno was set", errno_save, 0);
    sprintf(msg, "returned address equals %p", ADDR);
    pu_assert(msg, data == ADDR);

    memset(data, 0xff, 200);
    pu_assert("memory is accessible", data[50] == 0xff);

    return NULL;
#undef ADDR
}

static char * test_mmap_file(void)
{
    int fd;
    int errno_save;
    char str[80];

    memset(str, '\0', sizeof(str));

    fd = open("/root/README.markdown", O_RDONLY);
    pu_assert("fd is ok", fd > 0);

    errno = 0;
    data = mmap(NULL, 2 * sizeof(str), PROT_READ, 0, fd, 0);
    errno_save = errno;

    pu_assert("a new memory region returned", data != MAP_FAILED);
    pu_assert_equal("No errno was set", errno_save, 0);

    memcpy(str, data, sizeof(str) - 1);
    printf("%s\n", str);

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_mmap_anon);
    pu_run_test(test_mmap_anon_fixed);
    pu_run_test(test_mmap_file);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

