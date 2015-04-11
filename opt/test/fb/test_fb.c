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

    size_t palette[16];
    for (size_t i = 0; i < 16; i++)
        palette[i] = i * 0x775511;

    for (size_t y = 0; y < 480; y++) {
        for (size_t x = 0; x < 640; x++) {
            const size_t pitch = 1920;
            size_t color = palette[((x / 10) * 10 + (y / 10) * 10) % 16];
            set_rgb_pixel(fb, pitch, x, y, color);
        }
    }

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

