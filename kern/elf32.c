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

#define KERNEL_INTERNAL 1
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

static int load_section(struct buf * (*regions)[], int i, file_t * file,
        uintptr_t rbase, struct elf32_phdr * phdr)
{
    int prot;
    struct buf * sect;
    int err;

    prot = elf32_trans_prot(phdr->p_flags);
    sect = proc_newsect(phdr->p_vaddr + rbase, phdr->p_memsz, prot);
    if (!sect)
        return -ENOMEM;

    (*regions)[i] = sect;

    memset((void *)sect->b_data, 0, phdr->p_memsz);
    file->seek_pos = 0;
    if (phdr->p_filesz > 0) {
        err = file->vnode->vnode_ops->read(file, (void *)sect->b_data,
                                           phdr->p_filesz);
        if (err < 0)
            return -ENOEXEC;
    }

    return 0;
}

int load_elf32(file_t * file, uintptr_t * vaddr_base)
{
    struct elf32_header * elfhdr = NULL;
    ssize_t slen;
    struct elf32_phdr * phdr = NULL;
    size_t phsize;
    uintptr_t rbase;
    size_t lsections = 0;
    struct buf * (*newregions)[] = NULL;
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

    if (elfhdr->e_type == ET_DYN)
        rbase = *vaddr_base;
    else if (elfhdr->e_type == ET_EXEC)
        rbase = 0;
    else {
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
    for (int i = 0; i < elfhdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz != 0)
            lsections++;
    }

    /* Allocate memory for new regions struct */
    newregions = kmalloc(lsections * sizeof(struct buf *));
    if (!newregions) {
        retval = -ENOMEM;
        goto out;
    }

    /* Load sections */
    for (int i = 0; i < elfhdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && phdr[i].p_memsz != 0) {
            retval = load_section(newregions, i, file, rbase, &phdr[i]);
            if (retval)
                goto fail;

            if (i == 0)
                *vaddr_base = phdr[i].p_vaddr + rbase;
        }
    }

    retval = proc_replace(curproc->pid, newregions, lsections);
    if (retval)
        goto fail;

    /* Interrupts will be enabled automatically. */
    thread_die(0);
    /* Never returns if previous function was called. */

    goto out;
fail:
    kfree(newregions);
out:
    kfree(phdr);
    kfree(elfhdr);
    return retval;
}
EXEC_LOADFN(load_elf32, "elf32");
