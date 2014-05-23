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
#include <hal/fb.h>
#include "fonteng.h"

/*
 * TODO Array or list?
 */
struct fb_conf fb_main;

/* Current console text cursor position (ie. where the next character will
 * be written
 */
static int consx = 0;
static int consy = 0;

/* Current fg/bg colour */
static unsigned short int fgcolour = 0xff;
static unsigned short int bgcolour = 0;

/* A small stack to allow temporary colour changes in text */
static unsigned int colour_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned int colour_sp = 8;

void fb_register(struct fb_conf * fb)
{
    fb_main = *fb;
    fb_main.cons.max_cols = 80;
    fb_main.cons.max_rows = 25;
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
    unsigned int rowbytes = CHARSIZE_Y * fb_main.pitch;
    const size_t max_rows = fb_main.cons.max_rows;

    consx = 0;
    if(consy < (max_rows - 1)) {
        consy++;
        return;
    }

    /* Copy a screen's worth of data (minus 1 character row) from the
     * second row to the first
     */

    /* Calculate the address to copy the screen data from */
    source = (int *)(fb_main.base + rowbytes);
    memmove((void *)fb_main.base, source, (max_rows - 1) * rowbytes);

    /* Clear last line on screen */
    memset((void *)(fb_main.base + (max_rows - 1) * rowbytes), 0, rowbytes);
}

/*
 * TODO This function is partially BCM2835 specific and parts of it should be
 * moved into hardware specific files.
 * TODO This could be easily converted to support unicode
 */
void console_write(char * text)
{
    unsigned int row, addr;
    int col;
    char ch;
    const char * font_glyph;

    while((ch = *text)) {
        text++;

        /* Deal with control codes */
        switch(ch)
        {
            case 1: fgcolour = 0b1111100000000000; continue;
            case 2: fgcolour = 0b0000011111100000; continue;
            case 3: fgcolour = 0b0000000000011111; continue;
            case 4: fgcolour = 0b1111111111100000; continue;
            case 5: fgcolour = 0b1111100000011111; continue;
            case 6: fgcolour = 0b0000011111111111; continue;
            case 7: fgcolour = 0b1111111111111111; continue;
            case 8: fgcolour = 0b0000000000000000; continue;
                /* Half brightness */
            case 9: fgcolour = (fgcolour >> 1) & 0b0111101111101111; continue;
            case 10: newline(); continue;
            case 11: /* Colour stack push */
                if(colour_sp)
                    colour_sp--;
                colour_stack[colour_sp] =
                    fgcolour | (bgcolour<<16);
                continue;
            case 12: /* Colour stack pop */
                fgcolour = colour_stack[colour_sp] & 0xffff;
                bgcolour = colour_stack[colour_sp] >> 16;
                if(colour_sp<8)
                    colour_sp++;
                continue;
            case 17: bgcolour = 0b1111100000000000; continue;
            case 18: bgcolour = 0b0000011111100000; continue;
            case 19: bgcolour = 0b0000000000011111; continue;
            case 20: bgcolour = 0b1111111111100000; continue;
            case 21: bgcolour = 0b1111100000011111; continue;
            case 22: bgcolour = 0b0000011111111111; continue;
            case 23: bgcolour = 0b1111111111111111; continue;
            case 24: bgcolour = 0b0000000000000000; continue;
                /* Half brightness */
            case 25: bgcolour = (bgcolour >> 1) & 0b0111101111101111; continue;
        }

        if(ch<32)
            ch=0;

        font_glyph = fonteng_getglyph(ch);
        for(row = 0; row < CHARSIZE_Y; row++) {
            int glyph_x = 0;

            for(col = 0; col < CHARSIZE_X - 1; col++) {
                int dcolor;
                addr = (row + consy * CHARSIZE_Y) * fb_main.pitch
                        + (consx * CHARSIZE_X + glyph_x) * 3;

                if(row < (CHARSIZE_Y - 1) && (font_glyph[row] & (1 << col)))
                    dcolor = fgcolour;
                else
                    dcolor = bgcolour;

                *(unsigned short int *)(fb_main.base + addr + 0) = dcolor;
                *(unsigned short int *)(fb_main.base + addr + 1) = dcolor;
                *(unsigned short int *)(fb_main.base + addr + 2) = dcolor;

                glyph_x++;
            }
        }

        if(++consx >= fb_main.cons.max_cols)
            newline();
    }
}
