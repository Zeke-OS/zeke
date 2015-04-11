#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/fb.h>
#include "punit.h"

char * fb;
FILE * fp;

static void setup()
{
    fb = NULL;
    fp = NULL;
}

static void teardown()
{
    if (fb)
        munmap(fb, 0);
    if (fp)
        fclose(fp);
}

static char * test_mmap_fb(void)
{
    FILE * fp;
    int errno_save;

    fp = fopen("/dev/fbmm0", "r");
    pu_assert("Device file opened", fp != NULL);

    errno = 0;
    fb = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE,
              fileno(fp), 0);
    errno_save = errno;

    pu_assert("A new memory region returned", fb != MAP_FAILED);
    pu_assert_equal("No errno was set", errno_save, 0);

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_mmap_fb);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

