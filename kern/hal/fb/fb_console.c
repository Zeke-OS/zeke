/**
 *******************************************************************************
 * @file    fb_console.c
 * @author  Olli Vanhoja
 * @brief   Framebuffer console.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define FB_INTERNAL

#include <errno.h>
#include <termios.h>
#include <fs/devfs.h>
#include <buf.h>
#include <kstring.h>
#include <kmalloc.h>
#include <sys/ioctl.h>
#include <tty.h>
#include <hal/fb.h>
#include "fonteng.h"

/*
 * Current fg/bg color.
 */
const uint32_t def_fg_color = 0x00cc00;
const uint32_t def_bg_color = 0x000000;

static void draw_glyph(struct fb_conf * fb, const char * font_glyph,
                       int consx, int consy);
static void invert_glyph(struct fb_conf * fb, int consx, int consy);
static ssize_t fb_console_tty_read(struct tty * tty, off_t blkno,
                                   uint8_t * buf, size_t bcount, int oflags);
static ssize_t fb_console_tty_write(struct tty * tty, off_t blkno,
                                    uint8_t * buf, size_t bcount, int oflags);
static void fb_console_setconf(struct termios * conf);
static int fb_tty_ioctl(struct dev_info * devnfo, uint32_t request,
                        void * arg, size_t arg_len);

void fb_console_init(struct fb_conf * fb)
{
    const size_t left_margin = 0;
    const size_t upper_margin = (27 + 8) / CHARSIZE_Y + 1;
    struct fb_console * con = &fb->con;

    con->flags = FB_CONSOLE_WRAP;
    con->max_cols = fb->width  / CHARSIZE_X;
    con->max_rows = fb->height / CHARSIZE_Y;

    con->state.cursor_state = 1; /* TODO Not by default? */
    con->state.consx = left_margin;
    con->state.consy = upper_margin;
    con->state.fg_color = def_fg_color;
    con->state.bg_color = def_bg_color;
}

int fb_console_maketty(struct fb_conf * fb, dev_t dev_id)
{
    struct tty * tty;
    char dev_name[SPECNAMELEN];
    int err;

    ksprintf(dev_name, sizeof(dev_name), "fb%i", DEV_MINOR(dev_id));
    tty = tty_alloc("fb_tty", dev_id, dev_name, 0);
    if (!tty) {
        return -ENOMEM;
    }

    tty->opt_data = fb;
    tty->read = fb_console_tty_read;
    tty->write = fb_console_tty_write;
    tty->setconf = fb_console_setconf;
    tty->ioctl = fb_tty_ioctl;

    err = make_ttydev(tty);
    if (err) {
        tty_free(tty);
    }

    return err;
}

/**
 * New line.
 * Move to a new line, and, if at the bottom of the screen, scroll the
 * framebuffer 1 character row upwards, discarding the top row
 */
static void newline(struct fb_conf * fb)
{
    const uintptr_t base = fb->mem.b_data;
    int * source;
    /* Number of bytes in a character row */
    const unsigned int rowbytes = CHARSIZE_Y * fb->pitch;
    const size_t max_rows = fb->con.max_rows;
    size_t * const consx = &fb->con.state.consx;
    size_t * const consy = &fb->con.state.consy;
    const int cursor_prev_state = fb->con.state.cursor_state;

    if (*consy < (max_rows - 1)) {
        fb_console_set_cursor(fb, cursor_prev_state, 0, *consy + 1);
        return;
    }

    /*
     * Set cursor to a known position and hide it until scrolling is done.
     * This is only needed for SW cursor...
     */
    fb_console_set_cursor(fb, 0, 0, *consy);

    /* Calculate the address to copy the screen data from */
    source = (int *)(base + rowbytes);
    memmove((void *)base, source, (max_rows - 1) * rowbytes);

    /* Clear last line on the screen */
    memset((void *)(base + (max_rows - 1) * rowbytes), 0, rowbytes);

    /* Reset cursor state */
    fb_console_set_cursor(fb, cursor_prev_state, *consx, *consy);
}

/**
 * Draw font glyph to a character position (consx, consy).
 * @param font_glyph    is a pointer to the glyph from a font.
 * @param consx         is a pointer to the character x position.
 * @param consy         is a pointer to the character y position.
 */
static void draw_glyph(struct fb_conf * fb, const char * font_glyph,
                       int consx, int consy)
{
    int col, row;
    const size_t pitch = fb->pitch;
    const uintptr_t base = fb->mem.b_data;
    const size_t base_x = consx * CHARSIZE_X;
    const size_t base_y = consy * CHARSIZE_Y;
    const uint32_t fg_color = fb->con.state.fg_color;
    const uint32_t bg_color = fb->con.state.bg_color;

    for (row = 0; row < CHARSIZE_Y; row++) {
        for (col = 0; col < CHARSIZE_X; col++) {
            uint32_t rgb;

            rgb = (font_glyph[row] & (1 << col)) ? fg_color : bg_color;
            set_rgb_pixel(base, pitch, base_x + col, base_y + row, rgb);
        }
    }
}

/**
 * Invert glyph on character position (consx, consy).
 */
static void invert_glyph(struct fb_conf * fb, int consx, int consy)
{
    int col, row;
    const size_t pitch = fb->pitch;
    const uintptr_t base = fb->mem.b_data;
    const size_t base_x = consx * CHARSIZE_X;
    const size_t base_y = consy * CHARSIZE_Y;
    const uint32_t fg_color = fb->con.state.fg_color;

    for (row = 0; row < CHARSIZE_Y; row++) {
        for (col = 0; col < CHARSIZE_X; col++) {
            xor_pixel(base, pitch, base_x + col, base_y + row, fg_color);
        }
    }
}

/*
 * TODO This could be easily converted to support unicode
 */
void fb_console_write(struct fb_conf * fb, char * text)
{
    size_t * const consx = &fb->con.state.consx;
    size_t * const consy = &fb->con.state.consy;
    int cursor_state = fb->con.state.cursor_state;
    uint16_t ch;

    while ((ch = *text)) {
        size_t cur_x = *consx;
        text++;

        /* Deal with control codes */
        switch (ch) {
        case 0x5: /* ENQ */
            continue;
        case 0x8: /* BS */
            if (cur_x > 0)
                fb_console_set_cursor(fb, cursor_state, cur_x - 1, *consy);
            continue;
        case 0x9: /* TAB */
            fb_console_write(fb, "        ");
        case 0xd: /* CR */
            fb_console_set_cursor(fb, cursor_state, 0, *consy);
            continue;
        case 0xa: /* FF */
        case 0xb: /* VT */
        case 0xc: /* LF */
            newline(fb);
            *consx = cur_x;
            continue;
        }

        if (ch < 32)
            ch = 0;

        fb_console_set_cursor(fb, cursor_state, cur_x + 1, *consy);
        draw_glyph(fb, fonteng_getglyph(ch), cur_x, *consy);

        if (fb->con.flags & FB_CONSOLE_WRAP && *consx >= fb->con.max_cols) {
            newline(fb);
        }
    }
}

int fb_console_set_cursor(struct fb_conf * fb, int state, int col, int row)
{
    struct fb_console * con = &fb->con;

    if (!(0 <= col && col <= con->max_cols) &&
         (0 <= row && row <= con->max_rows)) {
        return -EINVAL;
    }

    if (fb->feature & FB_CONF_FEATURE_HW_CURSOR) {
        fb->set_hw_cursor_state(state, col * CHARSIZE_X, row * CHARSIZE_Y);
    } else { /* SW cursor */
        static int cursor_old_col = -1;
        static int cursor_old_row;

        if (!state) {
            if (fb->con.state.cursor_state && cursor_old_col != -1)
                invert_glyph(fb, cursor_old_col, cursor_old_row);
            cursor_old_col = -1;
        } else  {
            if (cursor_old_col != -1)
                invert_glyph(fb, cursor_old_col, cursor_old_row);
            invert_glyph(fb, col, row);
            cursor_old_col = col;
            cursor_old_row = row;
        }

    }
    fb->con.state.cursor_state = state;
    con->state.consx = col;
    con->state.consy = row;

    return 0;
}

static ssize_t fb_console_tty_read(struct tty * tty, off_t blkno,
                                   uint8_t * buf, size_t bcount, int oflags)
{
    return -ENOTSUP;
}

static ssize_t fb_console_tty_write(struct tty * tty, off_t blkno,
                                    uint8_t * buf, size_t bcount, int oflags)
{
    struct fb_conf * fb = (struct fb_conf *)tty->opt_data;

    for (size_t i = 0; i < bcount; i++) {
        char cstr[3] = { '\0', '\0', '\0' };

        cstr[0] = buf[i];
        if (cstr[0] == '\n')
            cstr[1] = '\r';
        fb_console_write(fb, cstr);
    }

    return bcount;
}

static void fb_console_setconf(struct termios * conf)
{
    /* TODO Implement setconf for fb_console */
}

static int fb_tty_ioctl(struct dev_info * devnfo, uint32_t request,
                        void * arg, size_t arg_len)
{
#if 0
    struct tty * tty = (struct tty *)devnfo->opt_data;
    struct fb_conf * fb = (struct fb_conf *)tty->opt_data;
#endif

    /* TODO Implement ioctl for fb_console */

    return -EINVAL;
}
