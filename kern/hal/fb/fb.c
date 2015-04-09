/**
 *******************************************************************************
 * @file    fb.c
 * @author  Olli Vanhoja
 * @brief   Generic frame buffer driver.
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

#include <stddef.h>
#include <kstring.h>
#include <kmalloc.h>
#include <kinit.h>
#include <kerror.h>
#include <hal/fb.h>
#include "splash.h"

/*
 * TODO Array or list?
 */
struct fb_conf * fb_main;

static void draw_splash(void);

/* TODO Should support multiple frame buffers or fail */
/*
 * TODO If fb is a default kerror output device then a fb should be registered
 * before kerror is initialized but there is no obvious way to do that, since
 * we definitely want to keep fb constructors in driver files to support
 * dynamic loading, in the future, just for example.
 */
void fb_register(struct fb_conf * fb)
{
    init_console(fb);
    fb_main = fb;

    draw_splash();
    fb_console_write("FB ready\r\n");
}

static void draw_splash(void)
{
    const size_t pitch = fb_main->pitch;
    const size_t base = fb_main->base;
    size_t col = 0;
    size_t row = 0;

    for (int i = 0; i < splash_width * splash_height; i++) {
        uint32_t pxl[3];

        if (i % splash_width == 0) {
            col = 0;
            row++;
        } else {
            col++;
        }

        SPLASH_PIXEL(splash_data, pxl);
        set_pixel(base, col, row, (pxl[0] << 16) | (pxl[1] << 8) | pxl[2]);
    }
}
