/**
 *******************************************************************************
 * @file    elf32.c
 * @author  Olli Vanhoja
 * @brief   32bit ELF loading.
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

#include <errno.h>
#include <elf_common.h>
#include <elf32.h>

static int check_header(const char * buf)
{
    const struct elf32_header * hdr = (const struct elf32_header *)buf;

    if (!IS_ELF(hdr) ||
        hdr->e_ident[EI_CLASS]      != ELFCLASS32 ||
        hdr->e_ident[EI_DATA]       != ELFDATA2LSB || /* TODO conf */
        hdr->e_ident[EI_VERSION]    != EV_CURRENT ||
        hdr->e_phentsize            != sizeof(struct elf32_phdr) ||
        hdr->e_version              != EV_CURRENT)
        return -ENOEXEC;

    /* Make sure the machine type is supported */
    if (hdr->e_machine != EM_ARM)
        return -ENOEXEC;

    return 0;
}
