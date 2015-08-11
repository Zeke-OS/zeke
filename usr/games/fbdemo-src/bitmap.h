#ifndef BITMAP_H_
#define BITMAP_H_

#include <stddef.h>

/**
 * File info structure
 */
struct bitmap_info {
    uint32_t rows;      /*!< Number if rows in the bitmap. */
    uint32_t cols;      /*!< Number of colums in the bitmap. */
    uint16_t bits_pp;
    size_t bitmap_size;
    char * bitmap;
    void (*draw)(char * fb, const struct fb_resolution * restrict r,
                 int x_off, int y_off,
                 const struct bitmap_info * restrict bip);
};

struct bitmap_info * bitmap_load(char * filename);

#endif
