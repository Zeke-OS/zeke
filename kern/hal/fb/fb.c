/**
 *******************************************************************************
 * @file    fb.c
 * @author  Olli Vanhoja
 * @brief   Generic frame buffer driver.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <stddef.h>
#include <kstring.h>
#include <kmalloc.h>
#include <hal/fb.h>
#include "fonteng.h"

/*
 * TODO Array or list?
 */
struct fb_conf * fb_main;

/* Current fg/bg colour */
const uint32_t def_fg_color = 0xffffff;
const uint32_t def_bg_color = 0x000000;

void fb_register(struct fb_conf * fb)
{
    fb_main = kmalloc(sizeof(struct fb_conf));

    memcpy(fb_main, fb, sizeof(struct fb_conf));
    fb_main->cons.max_cols = 80;
    fb_main->cons.max_rows = 25;

    fb_main->cons.state.consx = 0;
    fb_main->cons.state.consy = 0;
    fb_main->cons.state.fg_color = def_fg_color;
    fb_main->cons.state.bg_color = def_bg_color;
}

/*
 * TODO This function is partially BCM2835 specific and parts of it should be
 * moved into hardware specific files.
 */
/* Move to a new line, and, if at the bottom of the screen, scroll the
 * framebuffer 1 character row upwards, discarding the top row
 */
static void newline(void)
{
    int * source;
    /* Number of bytes in a character row */
    unsigned int rowbytes = CHARSIZE_Y * fb_main->pitch;
    const size_t max_rows = fb_main->cons.max_rows;
    size_t *const consx = &fb_main->cons.state.consx;
    size_t *const consy = &fb_main->cons.state.consy;

    *consx = 0;
    if(*consy < (max_rows - 1)) {
        (*consy)++;
        return;
    }

    /* Copy a screen's worth of data (minus 1 character row) from the
     * second row to the first
     */

    /* Calculate the address to copy the screen data from */
    source = (int *)(fb_main->base + rowbytes);
    memmove((void *)fb_main->base, source, (max_rows - 1) * rowbytes);

    /* Clear last line on screen */
    memset((void *)(fb_main->base + (max_rows - 1) * rowbytes), 0, rowbytes);
}

/*
 * TODO This function is partially BCM2835 specific and parts of it should be
 * moved into hardware specific files.
 * TODO This could be easily converted to support unicode
 */
void console_write(char * text)
{
    size_t *const consx = &fb_main->cons.state.consx;
    size_t *const consy = &fb_main->cons.state.consy;
    size_t pitch = fb_main->pitch;
    unsigned int row, addr;
    int col;
    uint16_t ch;
    const char * font_glyph;

    while ((ch = *text)) {
        text++;

        /* Deal with control codes */
        switch (ch) {
        case 0x5: /* ENQ */
            continue;
        case 0x8: /* BS */
            if (*consx > 0)
                (*consx)--;
            continue;
        case 0x9: /* TAB */
            console_write("        ");
        case 0xd: /* CR */
            *consx = 0;
            continue;
        case 0xa: /* FF */
        case 0xb: /* VT */
        case 0xc: /* LF */
            {
            int tmp = *consx;
            newline();
            *consx = tmp;
            }
            continue;
        }

        if (ch < 32)
            ch = 0;

        font_glyph = fonteng_getglyph(ch);
        for (row = 0; row < CHARSIZE_Y; row++) {
            int glyph_x = 0;

            for (col = 0; col < CHARSIZE_X - 1; col++) {
                uint32_t dcolor;
                addr = fb_main->base
                        + ((row + *consy * CHARSIZE_Y) * pitch
                        + (*consx * CHARSIZE_X + glyph_x) * 3);

                if (row < (CHARSIZE_Y - 1) && (font_glyph[row] & (1 << col)))
                    dcolor = fb_main->cons.state.fg_color;
                else
                    dcolor = fb_main->cons.state.bg_color;

                *(char *)(addr + 0) = (dcolor >> 16) & 0xff;
                *(char *)(addr + 1) = (dcolor >> 8) & 0xff;
                *(char *)(addr + 2) = dcolor & 0xff;

                glyph_x++;
            }
        }

        if(++(*consx) >= fb_main->cons.max_cols)
            newline();
    }
}
