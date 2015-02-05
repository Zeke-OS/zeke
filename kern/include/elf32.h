/**
 *******************************************************************************
 * @file    elf32.h
 * @author  Olli Vanhoja
 * @brief   Header file for 32bit ELF.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1996-1998 John D. Polstra.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *******************************************************************************
 */

#pragma once
#ifndef _ELF32_H_
#define _ELF32_H_

#include <stdint.h>

/**
 * ELF header.
 */
struct elf32_header {
    uint8_t e_ident[EI_NIDENT]; /*!< File identification */
    uint16_t e_type;        /*!<
                             * File type:
                             * 1 = relocatable
                             * 2 = executable
                             * 3 = shared
                             * 4 = core
                             */
    uint16_t e_machine;     /*!< ISA. */
    uint32_t e_version;     /*!< ELF version. */
    uint32_t e_entry;       /*!< Entry point. */
    uint32_t e_phoff;       /*!< Program header offset. */
    uint32_t e_shoff;       /*!< Section header offset. */
    uint32_t e_flags;       /*!< Arch specific flags. */
    uint16_t e_ehsize;      /*!< Size of this header in bytes. */
    uint16_t e_phentsize;   /*!< Size of a program header table entry. */
    uint16_t e_phnum;       /*!< Number of entries in program header table. */
    uint16_t e_shentsize;   /*!< The size of a section header table entry. */
    uint16_t e_shnum;       /*!< Number of entries in section header table. */
    uint16_t e_shstrndx;    /*!< Index of the section header table entry that
                             *   contains the section names. */
};

/**
 * Section header.
 */
struct elf32_shdr {
    uint32_t  sh_name;      /*!< Section name (index into the section header
                             *   string table). */
    uint32_t  sh_type;      /*!< Section type. */
    uint32_t  sh_flags;     /*!< Section flags. */
    uint32_t  sh_addr;      /*!< Address in memory image. */
    uint32_t  sh_offset;    /*!< Offset in file. */
    uint32_t  sh_size;      /*!< Size in bytes. */
    uint32_t  sh_link;      /*!< Index of a related section. */
    uint32_t  sh_info;      /*!< Depends on section type. */
    uint32_t  sh_addralign; /*!< Alignment in bytes. */
    uint32_t  sh_entsize;   /*!< Size of each entry in section. */
};

/**
 * Program header.
 */
struct elf32_phdr {
    uint32_t  p_type;       /*!< Entry type. */
    uint32_t  p_offset;     /*!< File offset of contents. */
    uint32_t  p_vaddr;      /*!< Virtual address in memory image. */
    uint32_t  p_paddr;      /*!< Physical address (not used). */
    uint32_t  p_filesz;     /*!< Size of contents in file. */
    uint32_t  p_memsz;      /*!< Size of contents in memory. */
    uint32_t  p_flags;      /*!< Access permission flags. */
    uint32_t  p_align;      /*!< Alignment in memory and file. */
};

/**
 * Dynamic structure.  The ".dynamic" section contains an array of them.
 */
struct elf32_dyn {
    int32_t d_tag;          /*!< Entry type. */
    union {
        uint32_t  d_val;    /*!< Integer value. */
        uint32_t  d_ptr;    /*!< Address value. */
    } d_un;
};

/*
 * Relocation entries.
 */

/**
 * Relocations that don't need an addend field.
 */
struct elf32_rel {
    uint32_t  r_offset;     /*!< Location to be relocated. */
    uint32_t  r_info;       /*!< Relocation type and symbol index. */
};

/* Relocations that need an addend field. */
struct elf32_rela {
    uint32_t r_offset;      /*!< Location to be relocated. */
    uint32_t r_info;        /*!< Relocation type and symbol index. */
    int32_t r_addend;       /*!< Addend. */
};

/* Macros for accessing the fields of r_info. */
#define ELF32_R_SYM(info)   ((info) >> 8)
#define ELF32_R_TYPE(info)  ((unsigned char)(info))

/* Macro for constructing r_info from field values. */
#define ELF32_R_INFO(sym, type) (((sym) << 8) + (unsigned char)(type))

/**
 *  Move entry
 */
struct elf32_move {
    uint64_t m_value;       /*!< symbol value */
    uint32_t m_info;        /*!< size + index */
    uint32_t m_poffset;     /*!< symbol offset */
    uint16_t m_repeat;      /*!< repeat count */
    uint16_t m_stride;      /*!< stride info */
};

/*
 *  The macros compose and decompose values for Move.r_info
 *
 *  sym = ELF32_M_SYM(M.m_info)
 *  size = ELF32_M_SIZE(M.m_info)
 *  M.m_info = ELF32_M_INFO(sym, size)
 */
#define ELF32_M_SYM(info)   ((info) >> 8)
#define ELF32_M_SIZE(info)  ((unsigned char)(info))
#define ELF32_M_INFO(sym, size) (((sym) << 8) + (unsigned char)(size))

/**
 *  Hardware/Software capabilities entry
 */
struct elf32_cap {
    uint32_t  c_tag;      /* how to interpret value */
    union {
        uint32_t  c_val;
        uint32_t  c_ptr;
    } c_un;
};

/**
 * Symbol table entries.
 */
struct elf32_sym {
    uint32_t st_name;       /*!< String table index of name. */
    uint32_t st_value;      /*!< Symbol value. */
    uint32_t st_size;       /*!< Size of associated object. */
    unsigned char st_info;  /*!< Type and binding information. */
    unsigned char st_other; /*!< Reserved (not used). */
    uint16_t st_shndx;      /*!< Section index of symbol. */
};

/* Macros for accessing the fields of st_info. */
#define ELF32_ST_BIND(info)     ((info) >> 4)
#define ELF32_ST_TYPE(info)     ((info) & 0xf)

/* Macro for constructing st_info from field values. */
#define ELF32_ST_INFO(bind, type)   (((bind) << 4) + ((type) & 0xf))

/* Macro for accessing the fields of st_other. */
#define ELF32_ST_VISIBILITY(oth)    ((oth) & 0x3)

/**
 * Structures used by Sun & GNU symbol versioning.
 */
struct elf32_verdef {
    uint16_t vd_version;
    uint16_t vd_flags;
    uint16_t vd_ndx;
    uint16_t vd_cnt;
    uint32_t vd_hash;
    uint32_t vd_aux;
    uint32_t vd_next;
};

struct elf32_verdaux {
    uint32_t vda_name;
    uint32_t vda_next;
};

struct elf32_verneed {
    uint16_t vn_version;
    uint16_t vn_cnt;
    uint32_t vn_file;
    uint32_t vn_aux;
    uint32_t vn_next;
};

struct elf32_vernaux {
    uint32_t vna_hash;
    uint16_t vna_flags;
    uint16_t vna_other;
    uint32_t vna_name;
    uint32_t vna_next;
};

typedef uint16_t Elf32_Versym;

struct elf32_syminfo {
    uint16_t si_boundto;    /*!< direct bindings - symbol bound to */
    uint16_t si_flags;      /*!< per symbol flags */
} Elf32_Syminfo;

#endif /* _ELF32_H_ */
