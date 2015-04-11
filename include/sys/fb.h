/**
 *******************************************************************************
 * @file    sys/fb.h
 * @author  Olli Vanhoja
 * @brief   Generic frame buffer interface.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

#pragma once
#ifndef SYS_FB_H
#define SYS_FB_H

/**
 * Set rgb pixel.
 * addr = y * pitch + x * 3
 * TODO Hw dependant
 */
#define set_rgb_pixel(_base, _pitch, _x, _y, _rgb) do {                        \
            const uintptr_t d = (uintptr_t)_base + (_y) * (_pitch) + (_x) * 3; \
            *(char *)(d + 0) = ((_rgb) >> 16) & 0xff;                          \
            *(char *)(d + 1) = ((_rgb) >> 8) & 0xff;                           \
            *(char *)(d + 2) = (_rgb) & 0xff;                                  \
} while (0)

struct fb_resolution {
    size_t width;
    size_t height;
    size_t depth;
};

#endif /* SYS_FB_H */

/**
 * @}
 */
