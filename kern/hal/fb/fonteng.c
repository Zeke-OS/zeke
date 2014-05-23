/**
 *******************************************************************************
 * @file    fonteng.c
 * @author  Olli Vanhoja
 * @brief   Font engine for frame buffer.
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

#include "fonts/font8x8_basic.h"
#ifdef configFB_FONT_LATIN
#include "fonts/font8x8_ext_latin.h"
#endif
#ifdef configFB_FONT_GREEK
#include "fonts/font8x8_greek.h"
#endif
#include "fonts/font8x8_box.h"
#include "fonts/font8x8_block.h"
#ifdef configFB_FONT_HIRAGANA
#include "fonts/font8x8_hiragana.h"
#endif
#include "fonteng.h"

static const char space[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const char * fonteng_getglyph(uint16_t ch)
{
    if (ch >= 0x0020 && ch <= 0x007F) {
        return (const char *)(&font8x8_basic[ch-0x20]);
#if configFB_FONT_LATIN
    } else if (ch >= 0x00A0 && ch <= 0x00FF) {
        return (const char *)(&font8x8_latin[ch]);
#endif
#ifdef configFB_FONT_GREEK
    } else if (ch >= 0x0390 && ch <= 0x03C9) {
        return (const char *)(&font8x8_greek[ch]);
#endif
    } else if (ch >= 0x2500 && ch <= 0x257F) {
        return (const char *)(&font8x8_box[ch]);
    } else if (ch >= 0x2580 && ch <= 0x259F) {
        return (const char *)(&font8x8_block[ch]);
#ifdef configFB_FONT_HIRAGANA
    } else if (ch >= 0x3040 && ch <= 0x309F) {
        return (const char *)(&font8x8_hiragana[ch]);
#endif
    } else {
        return (const char *)(&space);
    }
}
