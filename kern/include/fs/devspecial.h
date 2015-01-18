/**
 *******************************************************************************
 * @file    devspecial.h
 * @author  Olli Vanhoja
 * @brief   Special pseudo devices.
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

#ifndef DEVSPECIAL_H
#define DEVSPECIAL_H

struct dev_info;

/*
 * These special device read and write functions can be also used for other
 * device files that do not implement read or write.
 */

int devnull_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags);
int devnull_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags);
int devzero_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags);
int devzero_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags);
int devfull_read(struct dev_info * devnfo, off_t blkno,
                 uint8_t * buf, size_t bcount, int oflags);
int devfull_write(struct dev_info * devnfo, off_t blkno,
                  uint8_t * buf, size_t bcount, int oflags);

#endif /* DEVSPECIAL_H */
