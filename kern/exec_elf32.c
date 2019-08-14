/**
 *******************************************************************************
 * @file    exec_elf32.c
 * @author  Olli Vanhoja
 * @brief   32bit ELF loader.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/elf32.h>
#include <sys/elf_notes.h>
#include <sys/priv.h>
#include <buf.h>
#include <exec.h>
#include <fs/fs.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <thread.h>
#include <vm/vm.h>

/**
 * Elf parsing context.
 */
struct elf_ctx {
    file_t * file;
    struct elf32_header elfhdr;
    struct elf32_phdr * phdr;
    uintptr_t rbase;
    /* out */
    uintptr_t vaddr_base;
    size_t stack_size; /*!< Preferred minimum stack size. */
};

static int check_header(const struct elf32_header * hdr)
{
    if (!IS_ELF(hdr) ||
        hdr->e_ident[EI_CLASS]      != ELFCLASS32 ||
        hdr->e_ident[EI_DATA]       != ELFDATA_MACH ||
        hdr->e_ident[EI_VERSION]    != EV_CURRENT ||
        hdr->e_phentsize            != sizeof(struct elf32_phdr) ||
        hdr->e_version              != EV_CURRENT) {
        return -ENOEXEC;
    }

    /*
     * Make sure the machine type is supported.
     * TODO Get the machine type from somewhere.
     */
    if (hdr->e_machine != EM_ARM)
        return -ENOEXEC;

    /* Other sanity checks */
    if (hdr->e_phentsize != sizeof(struct elf32_phdr) || hdr->e_phoff == 0 ||
        hdr->e_phnum == 0) {
        return -ENOEXEC;
    }
    if (hdr->e_shnum == 0 || hdr->e_shentsize != sizeof(struct elf32_shdr))
        return -ENOEXEC;

    return 0;
}

/**
 * Read elf32 headers.
 * @param[out] elfhdr is the target struct for elf headers.
 * @param file is the file containing an elf image.
 */
static int read_elf32_header(struct elf32_header * elfhdr, file_t * file)
{
    const size_t elf32_header_size = sizeof(struct elf32_header);
    ssize_t bytes_read;
    vnode_t * vn = file->vnode;
    struct uio uio;

    /* Read elf header */
    if (vn->vnode_ops->lseek(file, 0, SEEK_SET) < 0)
        return -ENOEXEC;

    uio_init_kbuf(&uio, elfhdr, elf32_header_size);

    bytes_read = vn->vnode_ops->read(file, &uio, elf32_header_size);
    if (bytes_read != elf32_header_size) {
        KERROR_DBG("Reading elf failed (bytes_read = %d)\n",
                   (int)bytes_read);
        return -ENOEXEC;
    }

    /* Verify elf header */
    return check_header(elfhdr);
}

/**
 * Read elf32 program headers.
 */
static int read_program_headers(struct elf_ctx * ctx)
{
    vnode_t * vn = ctx->file->vnode;
    struct elf32_phdr * phdr = NULL;
    struct uio uio;
    size_t phsize;

    phsize = ctx->elfhdr.e_phnum * sizeof(struct elf32_phdr);
    phdr = kmalloc(phsize);
    if (!phdr)
        goto fail;

    if (vn->vnode_ops->lseek(ctx->file, ctx->elfhdr.e_phoff, SEEK_SET) < 0)
        goto fail;

    uio_init_kbuf(&uio, phdr, phsize);
    if (vn->vnode_ops->read(ctx->file, &uio, phsize) != (ssize_t)phsize)
        goto fail;

    ctx->phdr = phdr;
    return 0;
fail:
    kfree(phdr);
    return -ENOEXEC;
}

static int p_flags2b_uflags(uint32_t flags)
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

/**
 * Count and verify loadable sections.
 */
static int verify_loadable_sections(struct elf_ctx * ctx)
{
    size_t i, nr_newsections = 0;

    for (i = 0; i < ctx->elfhdr.e_phnum; i++) {
        if (ctx->phdr[i].p_type == PT_LOAD && ctx->phdr[i].p_memsz != 0)
            nr_newsections++;

        /* Check that no section is going to be mapped below the base limit. */
        if (ctx->phdr[i].p_type != PT_GNU_STACK &&
            ctx->phdr[i].p_vaddr + ctx->rbase < configEXEC_BASE_LIMIT) {
            KERROR_DBG("Invalid section mapping: (base = %p) < %p\n",
                       (void *)(ctx->phdr[i].p_vaddr + ctx->rbase),
                       (void *)(configEXEC_BASE_LIMIT));
            return -ENOEXEC;
        }
    }

    if (nr_newsections > 2) {
        KERROR_DBG("too many sections\n");
        return -ENOEXEC;
    }

    return 0;
}

/**
 * Read section from an elf file to memory.
 */
static ssize_t read_section(struct elf_ctx * ctx, size_t sect_index,
                            void * out, size_t size)
{
    vnode_t * vn = ctx->file->vnode;
    struct elf32_phdr * phdr = &ctx->phdr[sect_index];
    struct uio uio;

    if (phdr->p_filesz == 0)
        return -ENOEXEC;

    if (vn->vnode_ops->lseek(ctx->file, phdr->p_offset, SEEK_SET) < 0)
        return -ENOEXEC;

    size = min(size, phdr->p_filesz);
    uio_init_kbuf(&uio, out, size);
    return vn->vnode_ops->read(ctx->file, &uio, size);
}

/**
 * Create a memory region and load a section to it.
 */
static int load_section(struct elf_ctx * ctx, size_t sect_index,
                        struct buf ** region)
{
    struct elf32_phdr * phdr = &ctx->phdr[sect_index];
    struct buf * sect;
    void * ldp;
    int prot, err;

    if (phdr->p_memsz < phdr->p_filesz) {
        return -ENOEXEC;
    }

    prot = p_flags2b_uflags(phdr->p_flags);
    sect = vm_newsect(phdr->p_vaddr + ctx->rbase, phdr->p_memsz, prot);
    if (!sect) {
        return -ENOMEM;
    }

    ldp = (void *)(sect->b_data + (phdr->p_vaddr - sect->b_mmu.vaddr));
    err = read_section(ctx, sect_index, ldp, phdr->p_memsz);
    if (err < 0) {
        if (sect->vm_ops->rfree) {
            sect->vm_ops->rfree(sect);
        }
        return -ENOEXEC;
    }

    *region = sect;
    return 0;
}

static void nt_version(struct elf_note * note, size_t align)
{
    char * vendor = (char *)note + sizeof(struct elf_note);
    char * value = vendor + note->n_namesz;

    KERROR_DBG("Vendor: %s, Value: %s\n", vendor, value);
    /* TODO Do something with the nt_version note */
}

static size_t nt_stacksize(struct elf_note * note, size_t align)
{
    char * vendor = (char *)note + sizeof(struct elf_note);
    uint32_t value = *(uint32_t *)(vendor + memalign_size(note->n_namesz,
                                                          align));
    KERROR_DBG("Vendor: %s, Value: %u\n", vendor, value);

    if (strncmp(vendor, ELFNOTE_VENDOR_ZEKE, note->n_namesz))
        return 0; /* Not ours */

    return value;
}

/**
 * Read a Zeke process capabilities request note
 * @param altpcap Set if the file is allowed to set bounding capabilities.
 */
static size_t nt_capabilities(struct elf_note * note,
                              size_t align,
                              int altpcap,
                              struct cred * cred)
{
    char * vendor = (char *)note + sizeof(struct elf_note);
    const size_t nr_capabilities = note->n_descsz / sizeof(uint32_t);
    uint32_t * value = (uint32_t *)(vendor + memalign_size(note->n_namesz,
                                                           align));

    if (strncmp(vendor, ELFNOTE_VENDOR_ZEKE, note->n_namesz))
        return 0; /* Not ours */

    for (size_t i = 0; i < nr_capabilities; i++) {
        const int priv = (int)value[i];
        int err;

        if (priv >= _PRIV_MENT) {
            KERROR_DBG("Invalid capability: %u\n", priv);
            return -EINVAL;
        }
        KERROR_DBG("Add capability: %d\n", priv);

        /*
         * We can only set bounding capabilities if the file system allows it
         * and if the note type is NT_CAPABILITIES. NT_CAPABILITIES_REQ doesn't
         * allow setting the bounding set.
         */
        if (altpcap && note->n_type == NT_CAPABILITIES) {
            err = priv_cred_bound_set(cred, priv);
            if (err) {
                KERROR_DBG("Could not set bound capability: %u\n", priv);
                return err;
            }
        }

        /*
         * We can always attempt to set effective capabilities but it may fail
         * if the process doesn't have the same capabilities in the bounding
         * capabilities set.
         */
        err = priv_cred_eff_set(cred, priv);
        if (err) {
            KERROR_DBG("Could not set eff capability: %u\n", priv);
            return err;
        }
    }

    return nr_capabilities;
}

static int load_notes(struct proc_info * proc,
                      struct elf_ctx * ctx,
                      size_t sect_index)
{
    const int altpcap = !!(ctx->file->oflags & O_EXEC_ALTPCAP);
    struct elf32_phdr * phdr = &ctx->phdr[sect_index];
    const size_t align = phdr->p_align;
    size_t off = 0;
    uint8_t * sect;
    int retval;

    sect = kzalloc(phdr->p_memsz);
    if (!sect) {
        retval = -ENOMEM;
        goto out;
    }

    if (read_section(ctx, sect_index, sect, phdr->p_memsz) < 0) {
        KERROR_DBG("Failed to read a notes\n");
        retval = -ENOEXEC;
        goto out;
    }

    do {
        struct elf_note * note = (struct elf_note *)(sect + off);
        size_t note_size;

        if ((uintptr_t)note & (align - 1)) {
            /* Incorrect alignment */
            KERROR_DBG("Aligment fault %p %p\n", note, sect);
            retval = -ENOEXEC;
            goto out;
        }
        note_size = sizeof(struct elf_note) +
                    memalign_size(note->n_namesz, align) +
                    memalign_size(note->n_descsz, align);
        if (off + note_size > phdr->p_memsz) {
            retval = -ENOEXEC;
            goto out;
        }

        switch (note->n_type) {
        case NT_VERSION:
            nt_version(note, align);
            break;
        case NT_STACKSIZE: /* Preferred minimum stack size */
            ctx->stack_size = nt_stacksize(note, align);
            break;
        case NT_CAPABILITIES:
        case NT_CAPABILITIES_REQ:
            /* TODO Only allow settings caps if VFS allows it */
            retval = nt_capabilities(note, align, altpcap, &proc->cred);
            if (retval < 0) {
                goto out;
            }
        default:
            break;
        }
        off += note_size;
    } while (off < phdr->p_memsz);

    retval = 0;
out:
    kfree(sect);
    return retval;
}

/**
 * Parse all supported elf sections.
 */
static int parse_pheaders(struct proc_info * proc, struct elf_ctx * ctx)
{
    int e_type = ctx->elfhdr.e_type;
    size_t phnum = ctx->elfhdr.e_phnum;
    size_t i, nr_exec = 0;

    /*
     * TODO support PT_GNU_STACK header type
     * https://wiki.gentoo.org/wiki/Hardened/GNU_stack_quickstart
     *
     * TODO Is support for EXIDX header needed?
     */

    for (i = 0; i < phnum; i++) {
        struct elf32_phdr * phdr = &ctx->phdr[i];
        struct buf * sect;
        int err;

        switch (phdr->p_type) {
        case PT_LOAD:
            if (phdr->p_memsz == 0)
                break;

            if ((err = load_section(ctx, i, &sect)))
                return err;

            if (e_type == ET_EXEC && nr_exec < 2) {
                const int reg_nr = (nr_exec == 0) ? MM_CODE_REGION
                                                  : MM_HEAP_REGION;

                if (nr_exec == 0)
                    ctx->vaddr_base = phdr->p_vaddr + ctx->rbase;
                err = vm_replace_region(proc, sect, reg_nr, VM_INSOP_MAP_REG);
                if (err) {
                    KERROR(KERROR_ERR, "Failed to replace a region\n");
                    return err;
                }

                nr_exec++;
            } else {
                err = vm_insert_region(proc, sect, VM_INSOP_MAP_REG);
                if (err < 0) {
                    KERROR(KERROR_ERR, "Failed to insert a region\n");
                    return -1;
                }
            }
            break;
        case PT_NOTE:
            err = load_notes(proc, ctx, i);
            if (err) {
                KERROR(KERROR_ERR, "Failed to read notes\n");
                return -1;
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

int test_elf32(file_t * file)
{
    struct elf32_header elfhdr;

    return read_elf32_header(&elfhdr, file);
}

int load_elf32(struct proc_info * proc, file_t * file, uintptr_t * vaddr_base,
               size_t * stack_size)
{
    struct elf_ctx ctx = {
        .file = file,
        .phdr = NULL,
        .stack_size = 0,
    };
    int retval;

    if (!vaddr_base)
        return -EINVAL;

    if (read_elf32_header(&ctx.elfhdr, file))
        return -ENOEXEC;

    switch (ctx.elfhdr.e_type) {
    case ET_DYN:
        ctx.rbase = *vaddr_base;
        break;
    case ET_EXEC:
        ctx.rbase = 0;
        break;
    default:
        return -ENOEXEC;
    }

    if (read_program_headers(&ctx)) {
        return -ENOEXEC;
    }

    if ((retval = verify_loadable_sections(&ctx)) ||
        (retval = parse_pheaders(proc, &ctx))) {
        goto out;
    }

    *stack_size = ctx.stack_size;
    retval = 0;
out:
    kfree(ctx.phdr);

    return retval;
}

EXEC_LOADER(test_elf32, load_elf32, "elf32");
