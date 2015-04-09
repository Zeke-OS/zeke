/**
 *******************************************************************************
 * @file    kerror_fb.c
 * @author  Olli Vanhoja
 * @brief   UART klogger.
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

#include <hal/fb.h>
#include <kerror.h>
#include <sys/linker_set.h>

#if configFB == 0
#error configFB must be enabled
#endif

static void kerror_fb_puts(const char * str)
{
    size_t i = 0;
    char buf[2] = {'\0', '\0'};

    while (str[i] != '\0') {
        if (str[i] == '\n') {
            fb_console_write("\r\n");
        } else {
            buf[0] = str[i];
            fb_console_write(buf);
        }
        i++;
    }
}

static const struct kerror_klogger klogger_fb = {
    .id     = KERROR_FB,
    .init   = NULL,
    .puts   = &kerror_fb_puts,
    .read   = NULL,
    .flush  = NULL
};
DATA_SET(klogger_set, klogger_fb);
