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
#include "fonteng.h"

int fb_filno;
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
    const int w = bip->cols;
    const int h = bip->rows;
    static int x = 320;
    static int y = 240;
    static int dx = 1;
    static int dy = 1;

    bip->draw(bf, &resolution, x, y, bip);

    x += dx;
    y += dy;

    if (y < 0 || y > resolution.height - h)
        dy *= -1;

    if (x < 0 || x > resolution.width - w)
        dx *= -1;
}

static void draw_glyph(const char * font_glyph, int consx, int consy)
{
    int col, row;
    const size_t pitch = resolution.pitch;
    const uintptr_t base = (uintptr_t)bf;
    const size_t base_x = consx * CHARSIZE_X;
    const size_t base_y = consy * CHARSIZE_Y;
    const uint32_t fg_color = 0x00cc00;
    const uint32_t bg_color = 0x000000;

    for (row = 0; row < CHARSIZE_Y; row++) {
        for (col = 0; col < CHARSIZE_X; col++) {
            uint32_t rgb;

            rgb = (font_glyph[row] & (1 << col)) ? fg_color : bg_color;
            set_rgb_pixel(base, pitch, base_x + col, base_y + row, rgb);
        }
    }
}

void run(void)
{
    char * tmp;
    int page = 0;

    for (size_t i = 0; i < 9000; i++) {
        clear(bf, &resolution);
        bouncer();
        draw_glyph(fonteng_getglyph('c'), 10, 10);
        draw_glyph(fonteng_getglyph('n'), 12, 10);
        draw_glyph(fonteng_getglyph('b'), 14, 10);

        ioctl(fb_filno, IOCTL_FB_SETPAGE, &page);
        tmp = fb;
        fb = bf;
        bf = tmp;
        page = ~page & 1;
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

    fb_filno = fileno(fp);
    ioctl(fb_filno, IOCTL_FB_GETRES, &resolution);
    fb_size = 2 * resolution.height * resolution.pitch + resolution.width * 3;

    fb = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fb_filno, 0);
    bf = fb + resolution.height * resolution.pitch;

    set_rgb_pixel(bf, resolution.pitch, 11, 10, 0xfff);
    int page = 1;
    ioctl(fb_filno, IOCTL_FB_SETPAGE, &page);

    return 0;
#if 0

    run();

    munmap(fb, 0);

    return 0;
#endif
}
