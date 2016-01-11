/**
 *******************************************************************************
 * @file    strcbuf.h
 * @author  Olli Vanhoja
 * @brief   Generic circular buffer for strings.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup strcbuf
 * @{
 */

#pragma once
#ifndef STRCBUF_H
#define STRCBUF_H

#include <stddef.h>

/**
 * Strcbuf descriptor.
 */
struct strcbuf {
    size_t start;
    size_t end;
    size_t len;
    char * data;
};

/**
 * Insert line to a buffer.
 * @param buf is the buffer.
 * @param[in] msg is a zero terminated string.
 * @param len is the length of msg.
 */
void strcbuf_insert(struct strcbuf * buf, const char * msg, size_t len);

/**
 * Remove one line from a buffer.
 * @param[out]  dst is the destination array.
 * @param       len is the size of dst.
 */
size_t strcbuf_getline(struct strcbuf * buf, char * dst, size_t len);

#endif /* STRCBUF_H */

/**
 * @}
 */
