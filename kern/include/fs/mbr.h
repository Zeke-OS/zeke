/**
 *******************************************************************************
 * @file    mbr.h
 * @author  Olli Vanhoja
 * @brief   MBR driver header.
 * @section LICENSE
 * Copyright (c) 2021 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
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

#pragma once
#ifndef MBR_H
#define MBR_H

#include <fs/devfs.h>

/**
 * Try to find and register partitions from an MBR of a block device.
 * The device doesn't need to have an MBR but if it does have it the function
 * will try to register all the partitions found in it.
 * @param fd is the file that has the block device open.
 * @param[out] part_count if other than NULL is is set to the number of
 *                        partitions found.
 * @return  0 if partitions were registered and part_count is updated;
 *          -ENODEV if the fd isn't a block device;
 *          -ENOTSUP if no MBR was found or it was corrupted or invalid;
 *          -ENOMEM if the function ran out of memory.
 */
int mbr_register(int fd, int * part_count);

#endif /* MBR_H */
