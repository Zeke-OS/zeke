/**
 *******************************************************************************
 * @file    elf32.c
 * @author  Olli Vanhoja
 * @brief   32bit ELF loading.
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

#include <errno.h>
#include <kmalloc.h>
#include <kstring.h>
#include <vm/vm.h>
#include <buf.h>
#include <thread.h>
#include <proc.h>
#include <exec.h>
#include <elf_common.h>
#include <elf32.h>

static int check_header(const struct elf32_header * hdr)
{
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

    /* Other sanity checks */
    if (hdr->e_phentsize != sizeof(struct elf32_phdr) || hdr->e_phoff == 0 ||
            hdr->e_phnum == 0)
        return -ENOEXEC;
    if (hdr->e_shnum == 0 || hdr->e_shentsize != sizeof(struct elf32_shdr))
        return -ENOEXEC;

    return 0;
}

static int elf32_trans_prot(uint32_t flags)
{
    int prot = 0;

    if (flags & PF_X)
        prot |= VM_PROT_EXECUTE;
    if (flags & PF_W)
        prot |= VM_PROT_WRITE;
    if (flags & PF_R)
        prot |= VM_PROT_READ;

    return prot;
}

static int load_section(struct buf ** region, file_t * file,
        uintptr_t rbase, struct elf32_phdr * phdr)
{
    int prot;
    struct buf * sect;
    int err;

    if (phdr->p_memsz < phdr->p_filesz)
        return -ENOEXEC;

    prot = elf32_trans_prot(phdr->p_flags);
    sect = vm_newsect(phdr->p_vaddr + rbase, phdr->p_memsz, prot);
    if (!sect)
        return -ENOMEM;

    if (phdr->p_filesz > 0) {
        void * ldp;

        ldp = (void *)(sect->b_data + (phdr->p_vaddr - sect->b_mmu.vaddr));

        file->seek_pos = phdr->p_offset;
        err = file->vnode->vnode_ops->read(file, ldp, phdr->p_filesz);
        if (err < 0) {
            sect->vm_ops->rfree(sect);
            return -ENOEXEC;
        }
    }

    *region = sect;
    return 0;
}

int load_elf32(struct proc_info * proc, file_t * file, uintptr_t * vaddr_base)
{
    struct elf32_header * elfhdr = NULL;
    ssize_t slen;
    struct elf32_phdr * phdr = NULL;
    size_t phsize, nr_newsections;
    int e_type;
    uintptr_t rbase;
    int retval = 0;

    elfhdr = kmalloc(sizeof(struct elf32_header));
    if (!elfhdr)
        return -ENOMEM;

    if (!vaddr_base)
        return -EINVAL;

    /* Read elf header */
    file->seek_pos = 0;
    slen = file->vnode->vnode_ops->read(file, elfhdr,
            sizeof(struct elf32_header));
    if (slen != sizeof(struct elf32_header)) {
        retval = -ENOEXEC;
        goto out;
    }

    /* Verify elf header */
    retval = check_header(elfhdr);
    if (retval)
        goto out;

    e_type = elfhdr->e_type;
    if (e_type == ET_DYN) {
        rbase = *vaddr_base;
    } else if (e_type == ET_EXEC) {
        rbase = 0;
    } else {
        retval = -ENOEXEC;
        goto out;
    }

    /* Read program headers */
    phsize = elfhdr->e_phnum * sizeof(struct elf32_phdr);
    phdr = kmalloc(phsize);
    if (!phdr) {
        retval = -ENOMEM;
        goto out;
    }
    file->seek_pos = elfhdr->e_phoff;
    if (file->vnode->vnode_ops->read(file, phdr, phsize) != phsize) {
        retval = -ENOEXEC;
        goto out;
    }

    /* Count loadable sections. */
    nr_newsections = 0;
    for (size_t i = 0; i < elfhdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz != 0)
            nr_newsections++;

        /* Check that no section is going to be mapped below the base limit. */
        if (phdr[i].p_vaddr + rbase < configEXEC_BASE_LIMIT)
            return -ENOEXEC;
    }
    if (nr_newsections > 2)
        return -ENOEXEC;

    /* Unload regions above HEAP */
    vm_unload_regions(proc, MM_HEAP_REGION + 1, -1);

    /* Load sections */
    for (size_t i = 0; i < elfhdr->e_phnum; i++) {
        const char * const panicmsg = "Failed to map a section while in exec.";
        struct buf * sect;
        int err;

        if (!(phdr[i].p_type == PT_LOAD && phdr[i].p_memsz != 0))
            continue;

        retval = load_section(&sect, file, rbase, &phdr[i]);
        if (retval)
            goto out;

        if (e_type == ET_EXEC && i < 2) {
            const int reg_nr = (i == 0) ? MM_CODE_REGION : MM_HEAP_REGION;

            if (i == 0)
                *vaddr_base = phdr[i].p_vaddr + rbase;
            err = vm_replace_region(proc, sect, reg_nr,
                                    VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
            if (err) {
                panic(panicmsg);
            }
        } else {
            err = vm_insert_region(proc, sect,
                                   VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
            if (err < 0) {
                panic(panicmsg);
            }
        }
    }

    goto out;
out:
    kfree(phdr);
    kfree(elfhdr);

    return retval;
}
EXEC_LOADFN(load_elf32, "elf32");
