#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bitmap.h"

char * fb;
char * bf;
struct fb_resolution resolution;
struct bitmap_info * bip;

void clear(char * buf, const struct fb_resolution * r)
{
    memset(buf, 0, resolution.height * r->pitch + resolution.width * 3);
}

void bouncer(void)
{
    int x = 320, y = 240;
    int dx, dy;
    const int w = bip->cols;
    const int h = bip->rows;

    dx = dy = 1;

    while (1) {
        clear(bf, &resolution);
        bip->draw(bf, &resolution, x, y, bip);
        memcpy(fb, bf, resolution.height * resolution.pitch
                       + resolution.width * 3);

        x += dx;
        y += dy;

        if (y < 0 || y > resolution.height - h)
            dy *= -1;

        if (x < 0 || x > resolution.width - w)
            dx *= -1;
    }
}

int main(void)
{
    struct sched_param param = { .sched_priority = 0 };
    FILE * fp;
    size_t fb_size;

    if (sched_setscheduler(getpid(), SCHED_FIFO, &param)) {
        perror("failed to set SCHED_FIFO");
        return 1;
    }

    bip = bitmap_load("ball.bmp");
    if (!bip) {
        perror("Failed to load a bitmap");
        return 1;
    }

    fp = fopen("/dev/fbmm0", "r");
    if (!fp) {
        perror("Failed to open fb");
        return 1;
    }

    ioctl(fileno(fp), IOCTL_FB_GETRES, &resolution);
    fb_size = resolution.height * resolution.pitch + resolution.width * 3;

    fb = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(fp), 0);
    bf = malloc(fb_size);

    bouncer();

    munmap(fb, 0);

    return 0;
}
