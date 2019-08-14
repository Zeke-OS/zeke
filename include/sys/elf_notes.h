/*-
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright 2012 Konstantin Belousov <kib@FreeBSD.org>
 * Copyright 2000 David E. O'Brien, John D. Polstra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SYS_ELF_NOTES_H_
#define _SYS_ELF_NOTES_H_

#include <sys/elf_common.h>

#define ELFNOTE_VENDOR_ZEKE     "Zeke"
#define ELFNOTE_SECT_ZEKE_CONF  ".note.zeke.conf"

#define __ELF_NOTE_STRUCT_HEAD(_vendor_)                            \
        int32_t namesz;                                             \
        int32_t descsz;                                             \
        int32_t type;                                               \
        char name[sizeof(_vendor_)];

#define __ELF_NOTE_STRUCT_DEF(_section_, _name_)                    \
    _name_ __attribute__ ((section (_section_), aligned(4))) __used

/**
 * Note integer.
 * @param _type_ is one of the `NT_` types from `elf_common.h`
 */
#define ELFNOTE_INT(_section_, _vendor_, _type_, _name_, _value_)   \
    static const struct {                                           \
        __ELF_NOTE_STRUCT_HEAD(_vendor_)                            \
        int32_t desc;                                               \
    } __ELF_NOTE_STRUCT_DEF(_section_, _name_) = {                  \
        .namesz = sizeof(_vendor_),                                 \
        .descsz = sizeof(int32_t),                                  \
        .type = (_type_),                                           \
        .name = (_vendor_),                                         \
        .desc = (_value_)                                           \
    }

/**
 * Note string.
 * @param _type_ is one of the `NT_` types from `elf_common.h`
 * @param _str_ is the string literal to be stored. The size must be
 *              a multiple of the word size.
 */
#define ELFNOTE_STR(_section_, _vendor_, _type_, _name_, _str_)     \
    static const struct {                                           \
        __ELF_NOTE_STRUCT_HEAD(_vendor_)                            \
        char desc[];                                                \
    } __ELF_NOTE_STRUCT_DEF(_section_, _name_) = {                  \
        .namesz = sizeof(_vendor_),                                 \
        .descsz = sizeof(_str_),                                    \
        .type = (_type_),                                           \
        .name = (_vendor_),                                         \
        .desc = _str_                                               \
    }

/**
 * Request a minimum stack size of _stack_size_ bytes.
 * @param _stack_size_ is the minumum stack size required by
 *                     the executable.
 */
#define ELFNOTE_STACKSIZE(_stack_size_)                             \
    static const struct {                                           \
        __ELF_NOTE_STRUCT_HEAD(ELFNOTE_VENDOR_ZEKE)                 \
        uint32_t desc;                                              \
    } __ELF_NOTE_STRUCT_DEF(ELFNOTE_SECT_ZEKE_CONF, stacksize) = {  \
        .namesz = sizeof(ELFNOTE_VENDOR_ZEKE),                      \
        .descsz = sizeof(uint32_t),                                 \
        .type = NT_STACKSIZE,                                       \
        .name = ELFNOTE_VENDOR_ZEKE,                                \
        .desc = (_stack_size_)                                      \
    }

/**
 * Note process capabilities.
 * Note specifying the capabilities required to execute the elf binary
 * but which can be gained on exec() if altpcap is set for the file.
 * Maximum of 64 capabilities can be requested per note and an
 * unlimited number of notes of this can be created.
 */
#define ELFNOTE_CAPABILITIES(...)                                   \
    static const struct {                                           \
        __ELF_NOTE_STRUCT_HEAD(ELFNOTE_VENDOR_ZEKE)                 \
        int32_t desc[];                                             \
    } __ELF_NOTE_STRUCT_DEF(ELFNOTE_SECT_ZEKE_CONF,                 \
                            __CONCAT(caps_, __COUNTER__)) = {       \
        .namesz = sizeof(ELFNOTE_VENDOR_ZEKE),                      \
        .descsz = (sizeof((uint32_t[]){__VA_ARGS__})),              \
        .type = NT_CAPABILITIES,                                    \
        .name = ELFNOTE_VENDOR_ZEKE,                                \
        .desc = {__VA_ARGS__}                                       \
    }

/**
 * Note process non-gainable capabilities.
 * Note specifying the capabilities required to execute the elf binary.
 * These capabilities must exist in the bounding set and will be
 * promoted to the effective set.
 * Maximum of 64 capabilities can be requested per note and an
 * unlimited number of notes of this can be created.
 */
#define ELFNOTE_CAPABILITIES_REQ(...)                               \
    static const struct {                                           \
        __ELF_NOTE_STRUCT_HEAD(ELFNOTE_VENDOR_ZEKE)                 \
        int32_t desc[];                                             \
    } __ELF_NOTE_STRUCT_DEF(ELFNOTE_SECT_ZEKE_CONF,                 \
                            __CONCAT(caps_req_, __COUNTER__)) = {   \
        .namesz = sizeof(ELFNOTE_VENDOR_ZEKE),                      \
        .descsz = (sizeof((uint32_t[]){__VA_ARGS__})),              \
        .type = NT_CAPABILITIES_REQ,                                \
        .name = ELFNOTE_VENDOR_ZEKE,                                \
        .desc = {__VA_ARGS__}                                       \
    }

#endif /* _SYS_ELF_NOTES_H_ */
