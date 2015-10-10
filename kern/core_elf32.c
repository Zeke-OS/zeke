/**
 *******************************************************************************
 * @file    core_elf32.c
 * @author  Olli Vanhoja
 * @brief   32bit ELF core dumps.
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

#include <errno.h>
#include <fcntl.h>
#include <machine/endian.h>
#include <stdint.h>
#include <sys/elf32.h>
#include <buf.h>
#include <fs/fs.h>
#include <kmalloc.h>
#include <proc.h>

static off_t write2file(file_t * file, void * p, size_t size)
{
    vnode_t * vn = file->vnode;
    struct uio uio;

    uio_init_kbuf(&uio, p, size);
    return vn->vnode_ops->write(file, &uio, size);
}

static off_t write_elf_header(file_t * file, int phnum)
{
    const size_t elf32_header_size = sizeof(struct elf32_header);
    const uint8_t elf_endian =
#if (_BYTE_ORDER == _LITTLE_ENDIAN)
        ELFDATA2LSB;
#elif (_BYTE_ORDER == BIG_ENDIAN)
        ELFDATA2MSB;
#else
#error Unsuported endianess
#endif
    struct elf32_header hdr = {
        .e_type = ET_CORE,
        .e_machine = EM_ARM, /* TODO get it from somewhere. */
        .e_version = EV_CURRENT,
        .e_phoff = elf32_header_size,
        .e_shoff = 0,
        .e_ehsize = elf32_header_size,
        .e_flags = 0,
        .e_phentsize = sizeof(struct elf32_phdr),
        .e_phnum = phnum,
        .e_shentsize = sizeof(struct elf32_shdr),
        .e_shnum = 0,
        .e_shstrndx = 0,
    };

    hdr.e_ident[EI_MAG0] = ELFMAG0;
    hdr.e_ident[EI_MAG1] = ELFMAG1;
    hdr.e_ident[EI_MAG2] = ELFMAG2;
    hdr.e_ident[EI_MAG3] = ELFMAG3;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    hdr.e_ident[EI_CLASS] = ELFCLASS32;
    hdr.e_ident[EI_DATA] = elf_endian;
    hdr.e_ident[EI_OSABI] = ELFOSABI_STANDALONE; /* TODO */

    /* Write the elf header. */
    return write2file(file, &hdr, elf32_header_size);
}

static uint32_t uap2p_flags(struct buf * bp)
{
    int uap = bp->b_uflags;
    uint32_t p_flags = 0;

    p_flags |= (uap & VM_PROT_READ)     ? PF_R : 0;
    p_flags |= (uap & VM_PROT_WRITE)    ? PF_W : 0;
    p_flags |= (uap & VM_PROT_EXECUTE)  ? PF_X : 0;
    p_flags |= (uap & VM_PROT_COW)      ? PF_ZEKE_COW : 0;

    return p_flags;
}

static size_t create_notes(struct proc_info * proc, void ** notes_out)
{
    /* TODO Support all threads */
    void * notes;
    size_t notes_size = 4; /* TODO */

    /* TODO
     * - proc status
     * - thread status
     * - siginfo_t
     * - registers
     * - tls registers
     */

    notes = kzalloc(notes_size);

    *notes_out = notes;
    return notes_size;
}

static int create_pheaders(const struct vm_mm_struct * mm, size_t notes_size,
                           struct elf32_phdr ** phdr_arr_out)
{
    struct elf32_phdr * phdr_arr;
    int i, hi, nr_core_regions, phnum;
    size_t phsize;
    off_t offset;

    /*
     * Count dumpable memory regions.
     */
    nr_core_regions = 0;
    for (i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];
        if (region->b_flags & B_NOCORE)
            continue;
        nr_core_regions++;
    }

    phnum = 1 + nr_core_regions;
    phsize = phnum * sizeof(struct elf32_phdr);
    phdr_arr = kzalloc(phsize);
    if (!phdr_arr)
        return -ENOMEM;

    /* Assume that program headers start right after the elf header. */
    offset = sizeof(struct elf32_header) + phsize;

    /* NOTE section. */
    phdr_arr[0] = (struct elf32_phdr){
        .p_type = PT_NOTE,
        .p_offset = offset,
        .p_vaddr = 0,
        .p_paddr = 0,
        .p_filesz = notes_size,
        .p_memsz = 0,
        .p_flags = PF_R,
        .p_align = sizeof(uint32_t),
    };
    offset += notes_size;

    /*
     * Memory region headers.
     */
    for (i = 0, hi = 1; i < mm->nr_regions; i++, hi++) {
        struct buf * region = (*mm->regions)[i];
        struct elf32_phdr * phdr = phdr_arr + hi;

        if (region->b_flags & B_NOCORE)
            continue;

        *phdr = (struct elf32_phdr){
            .p_type = PT_LOAD,
            .p_offset = offset,
            .p_vaddr = region->b_mmu.vaddr,
            .p_paddr = region->b_mmu.paddr, /* Linux sets to 0. */
            .p_filesz = region->b_bufsize,
            .p_memsz = region->b_bufsize,
            .p_flags = uap2p_flags(region),
            .p_align = sizeof(uintptr_t),
        };
        offset += region->b_bufsize;
    }

    *phdr_arr_out = phdr_arr;
    return phnum;
}

static off_t dump_regions(file_t * file, const struct vm_mm_struct * mm)
{
    off_t err, off = 0;

    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        err = write2file(file, (void *)region->b_data, region->b_bufsize);
        if (err != region->b_bufsize)
            return err;
        off += off;
    }

    return off;
}

int core_dump2file(struct proc_info * proc, file_t * file)
{
    vnode_t * vn = file->vnode;
    struct vm_mm_struct * mm;
    struct elf32_phdr * phdr = NULL;
    void * notes = NULL;
    int phnum, retval;
    off_t err;
    size_t phsize, notes_size;

    if (vn->vnode_ops->lseek(file, 0, SEEK_SET) < 0) {
        return -EINVAL;
    }

    mm = &proc->mm;
    mtx_lock(&mm->regions_lock);

    notes_size = create_notes(proc, &notes);
    phnum = create_pheaders(mm, notes_size, &phdr);
    phsize = phnum * sizeof(struct elf32_phdr);

    if ((err = write_elf_header(file, phnum)) < 0 ||
        (err = write2file(file, phdr, phsize)) < 0 ||
        (err = write2file(file, notes, notes_size)) < 0 ||
        (err = dump_regions(file, mm)) < 0) {
        retval = err;
        goto out;
    }

    retval = 0;
out:
    kfree(phdr);
    kfree(notes);
    mtx_unlock(&mm->regions_lock);
    return retval;
}
