#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fb.h>
#include "bitmap.h"

#define IS_POS_BOUNDED(_X_, _Y_, _X_MAX_, _Y_MAX_) \
    ((_X_ >= 0) && (_Y_ >= 0) && (_X_ <= _X_MAX_) && (_Y_ <= _Y_MAX_))

static uint32_t read_image_info(FILE * fp, long offset, int n)
{
    uint32_t value = 0;
    size_t i;

    if (fseek(fp, offset, SEEK_SET)) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        char c;

        c = getc(fp);
        value += c << i * 8;
    }

    return value;
}

static void draw_1bit(char * fb, const struct fb_resolution * restrict r,
                      int x_off, int y_off,
                      const struct bitmap_info * restrict bip)
{
    int x = 0, y = bip->rows - 1;
    size_t i;
    const size_t max = bip->rows * (bip->cols / 8);
    char * c = bip->bitmap;

    for (i = 0; i < max; i++) {
        size_t b;
        char byte = *c++;

        for (b = 0; b < 8; b++) {
            const char mask = 0x80 >> b;
            const int fb_x = x + x_off;
            const int fb_y = y + y_off;

            if (IS_POS_BOUNDED(fb_x, fb_y, r->width, r->height)) {
                set_rgb_pixel(fb, r->pitch, fb_x, fb_y,
                              (~byte & mask) ? 0xffffff : 0);
            }
            if (++x == bip->cols) {
                x = 0;
                y--;
            }
            if (y == 0)
                return;
        }
    }
}

static void draw_32bit(char * fb, const struct fb_resolution * restrict r,
                       int x_off, int y_off,
                       const struct bitmap_info * restrict bip)
{
    int x = 0, y = bip->rows - 1;
    size_t i;
    const size_t max = bip->rows * bip->cols;
    uint32_t * p = (uint32_t *)bip->bitmap;

    for (i = 0; i < max; i++) {
        const int fb_x = x + x_off;
        const int fb_y = y + y_off;

        if (IS_POS_BOUNDED(fb_x, fb_y, r->width, r->height)) {
            *(uint32_t *)((uintptr_t)fb + fb_y * r->pitch + fb_x * 3) = *p++;
        }
        if (++x == bip->cols) {
            x = 0;
            y--;
        }
        if (y == 0)
            return;
    }
}

struct bitmap_info * bitmap_load(char * filename)
{
    struct bitmap_info * bip;
    FILE * fp = NULL;
    uint32_t offset;

    bip = calloc(1, sizeof(struct bitmap_info));
    if (!bip)
        return NULL;

    fp = fopen(filename, "r");
    if (!fp)
        goto fail;

    if (fgetc(fp) != 'B' || fgetc(fp) != 'M')
        goto fail;

    /* Get BMP info */
    offset = read_image_info(fp, 0x0A, 4);
    bip->cols = read_image_info(fp, 0x12, 4);
    bip->rows = read_image_info(fp, 0x16, 4);
    bip->bits_pp = (uint16_t)read_image_info(fp, 0x1C, 2);
    bip->bitmap_size = read_image_info(fp, 0x22, 4);

    if (bip->bits_pp == 1)
        bip->draw = draw_1bit;
    else if (bip->bits_pp == 32)
        bip->draw = draw_32bit;
    else
        goto fail;

    if (bip->bitmap_size == 0)
        bip->bitmap_size = (bip->cols * bip->bits_pp + 7) / 8 * abs(bip->rows);
    bip->bitmap = malloc(bip->bitmap_size);
    if (!bip->bitmap)
        goto fail;
    fseek(fp, offset, SEEK_SET);
    if (fread(bip->bitmap, 1, bip->bitmap_size, fp) < bip->bitmap_size) {
        goto fail;
    }

    fclose(fp);
    return bip;
fail:
    if (bip && bip->bitmap)
        free(bip->bitmap);

    free(bip);
    if (fp)
        fclose(fp);
    return NULL;
}
