#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "punit.h"

char * data;
FILE * fp;

static void setup()
{
    data = NULL;
    fp = NULL;
}

static void teardown()
{
    if (data)
        munmap(data, 0);
    if (fp)
        fclose(fp);
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
    FILE * fp;
    int errno_save;
    char str1[80];
    char str2[80];

    memset(str1, '\0', sizeof(str1));
    memset(str2, '\0', sizeof(str2));

    fp = fopen("/root/README.markdown", "r");
    pu_assert("fp not NULL", fp != NULL);

    errno = 0;
    data = mmap(NULL, 2 * sizeof(str1), PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    errno_save = errno;

    pu_assert("a new memory region returned", data != MAP_FAILED);
    pu_assert_equal("No errno was set", errno_save, 0);

    memcpy(str1, data, sizeof(str1) - 1);
    fread(str2, sizeof(char), sizeof(str2) - 1, fp);
    pu_assert("Strings are equal", strcmp(str1, str2) == 0);

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_mmap_anon, PU_RUN);
    pu_def_test(test_mmap_anon_fixed, PU_RUN);
    pu_def_test(test_mmap_file, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

