/**
 *******************************************************************************
 * @file    fb.h
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

#pragma once
#ifndef HAL_FB_H
#define HAL_FB_H

#include <stddef.h>
#include <stdint.h>
#include <sys/fb.h>
#include <sys/linker_set.h>

struct dev_info;

struct fb_console {
    size_t max_cols;
    size_t max_rows;
    struct cons_state {
        size_t consx;
        size_t consy;
        uint32_t fg_color; /*!< Current fg color. */
        uint32_t bg_color; /*!< Current bg color. */
    } state;
};

struct fb_conf {
    struct buf * mem;
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t base;
    struct fb_console con;

    /**
     * Change screen resolution.
     * This should be set by the actual hw driver.
     * @param width is the screen width.
     * @param height is the screen height.
     * @param depth is the color depth.
     */
    int (*set_resolution)(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth);
};

void fb_register(struct fb_conf * fb);
void fb_console_write(struct fb_conf * fb, char * text);

#ifdef FB_INTERNAL
/**
 * Initialize fb_console struct in fb_conf struct.
 * This function can be only called after (or by) fb_register() has been called
 * and normally this function shall be called only by fb_register().
 */
void fb_console_init(struct fb_conf * fb);

int fb_console_maketty(struct fb_conf * fb);

int fb_ioctl(struct dev_info * devnfo, uint32_t request,
             void * arg, size_t arg_len);
#endif

#endif /* HAL_FB_H */
