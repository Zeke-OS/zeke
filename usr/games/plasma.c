#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <sys/fb.h>
#include <sys/mman.h>
#include <unistd.h>

#define PI 3.141592654
#define PITCH 1920 /* TODO We should get this by using ioctl */

int cosinus[256];
unsigned char p1, p2, p3, p4, t1, t2, t3, t4;
int x, y;
int pal[768];
char * fb;

void plasma(void)
{
    t1 = p1;
    t2 = p2;

    for (y = 0; y < 480; y++) {
        t3 = p3;
        t4 = p4;
        for (x = 0; x < 640; x++) {
            const uintptr_t d = (uintptr_t)fb + (y) * (PITCH) + (x) * 3;
            int c, r, g, b;

            c = cosinus[t1] + cosinus[t2] + cosinus[t3] + cosinus[t4];
            r = pal[((c) >> 16) & 0xff];
            g = pal[(((c) >> 8) & 0xff) + 1];
            b = pal[((c) & 0xff) + 2];

            r = pal[((c) >> 16) & 0xff];
            g = pal[(((c) >> 8) & 0xff) + 1];
            b = pal[((c) & 0xff) + 2];

            *(char *)(d + 0) = r;
            *(char *)(d + 1) = g;
            *(char *)(d + 2) = b;

            t3 += 1;
            t4 += 3;
        }
        t1 += 2;
        t2 += 1;
    }
    p1 += 1;
    p2 -= 2;
    p3 += 3;
    p4 -= 4;
}

void prep_pal(void)
{
    for (x = 0, y = 0; x < 63 * 3; x += 3, y++) {
        pal[x] = y;
    for (x = 63 * 3, y = 63; x < 127 * 3; x += 3, y--) {
        pal[x] = y;
    }
    for (x = 127 * 3, y = 0; x < 191 * 3; x += 3, y++) {
        pal[x + 1] = y;
    }
    for (x = 191 * 3, y = 191; x < 255 * 3; x += 3, y--) {
        pal[x + 1] = y;
    }
}

void pre_calc(void)
{
    int i;

    for (i = 0; i < 256; i++) {
        cosinus[i] = 30 * (cos(i * PI / 64));
    }
}

int main(void)
{
    size_t i;
    FILE * fp;

    fp = fopen("/dev/fbmm0", "r");
    if (!fp)
        return 1;

    fb = mmap(NULL, 0x100000, PROT_READ | PROT_WRITE, MAP_PRIVATE,
              fileno(fp), 0);

    prep_pal();
    pre_calc();

    for (i = 0; i < 1000; i++) {
        plasma();
        /* sleep(1); */
    }

    munmap(fb, 0);
    fclose(fp);

    return 0;
}
