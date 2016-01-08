/**
 *******************************************************************************
 * @file    fb.c
 * @author  Olli Vanhoja
 * @brief   Generic frame buffer driver.
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

#define FB_INTERNAL

#include <stddef.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <machine/atomic.h>
#include <buf.h>
#include <fs/dev_major.h>
#include <fs/devfs.h>
#include <fs/devspecial.h>
#include <hal/fb.h>
#include <kerror.h>
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <tty.h>
#include "splash.h"

/**
 * Minor number for the next frame buffer device.
 * The minor number is shared between a tty and a mm device but
 * the major number is, of course, different.
 */
atomic_t fb_minor = ATOMIC_INIT(0);

static void fb_mm_ref(struct buf * this);
static void fb_mm_rfree(struct buf * this);
static int fb_makemmdev(struct fb_conf * fb, dev_t dev_id);
static void draw_splash(struct fb_conf * fb);
static int fb_mm_ioctl(struct dev_info * devnfo, uint32_t request,
                       void * arg, size_t arg_len);
static int fb_mmap(struct dev_info * devnfo, size_t blkno, size_t bsize,
                   int flags, struct buf ** bp_out);

/**
 * Frame buffer buffer operations.
 * Used for mmap access.
 */
struct vm_ops fb_mm_bufops = {
    .rref = fb_mm_ref,
    .rclone = NULL, /* What ever, but we don't like clones. */
    .rfree = fb_mm_rfree, /* You can try me but it will be never free. */
};

/* TODO Should support multiple frame buffers or fail */
/*
 * TODO If fb is a default kerror output device then a fb should be registered
 * before kerror is initialized but there is no obvious way to do that, since
 * we definitely want to keep fb constructors in driver files to support
 * dynamic loading, in the future, just for example.
 */
int fb_register(struct fb_conf * fb)
{
    const int minor = atomic_inc(&fb_minor);
    dev_t devid_tty;
    dev_t devid_mm;
    int err;

    devid_tty = DEV_MMTODEV(VDEV_MJNR_FB, minor);
    devid_mm = DEV_MMTODEV(VDEV_MJNR_FBMM, minor);

    /* FIXME FB TTY breaks the boot. */
#if 0
    fb_console_init(fb);
    err = fb_console_maketty(fb, devid_tty);
    if (err) {
        KERROR(KERROR_ERR, "FB: maketty failed\n");
        return err;
    }
#endif

    err = fb_makemmdev(fb, devid_mm);
    if (err) {
        KERROR(KERROR_ERR, "FB: makemmdev failed\n");
        return err;
    }

#if 0
    draw_splash(fb);
    fb_console_write(fb, "FB ready\r\n");
#endif

    return 0;
}

static void fb_mm_free_callback(struct kobj * obj)
{
    struct buf * bp = container_of(obj, struct buf, b_obj);

    KERROR(KERROR_ERR, "FB buf object (%p) freed!\n", bp);
}

void fb_mm_initbuf(struct fb_conf * fb)
{
    struct buf * bp = &fb->mem;

    /* Just to make sure we don't have anything fancy there. */
    memset(bp, 0, sizeof(struct buf));

    mtx_init(&bp->lock, MTX_TYPE_TICKET, 0);

    /* TODO We must implement a generic initializer for this */
    kobj_init(&bp->b_obj, fb_mm_free_callback);
    fb->mem.b_flags = B_BUSY | B_NOSYNC | B_NOTSHARED | B_NOCORE;
    bp->vm_ops = &fb_mm_bufops;
    bp->b_mmu.vaddr = 0; /* Will be set when mapped. */
    bp->b_mmu.pt = NULL; /* Will be set when mapped. */
}

void fb_mm_updatebuf(struct fb_conf * restrict fb,
                     const mmu_region_t * restrict region)
{
    /*
     * The following is not a very good idea but vpt doesn't support
     * sections.
     */
    fb->mem.b_mmu.num_pages = region->num_pages * MMU_NR_COARSE_ENTR;
    fb->mem.b_mmu.paddr = region->paddr;
    fb->mem.b_mmu.control = region->control;
    fb->mem.b_data = region->vaddr; /* This is an access point for the kernel
                                     * if needed. But also marks in-core state.
                                     */
    fb->mem.b_bufsize = mmu_sizeof_region(region);
    fb->mem.b_bcount = fb->mem.b_bufsize;
}

/**
 * Ref function for mmap buf.
 * @see fb_mm_bufops.
 */
static void fb_mm_ref(struct buf * this)
{
    kobj_ref(&this->b_obj);
}

/**
 * Fre function for mmap buf.
 * @see fb_mm_bufops.
 */
static void fb_mm_rfree(struct buf * this)
{
    kobj_unref(&this->b_obj);
}

/**
 * Make a fb memory mapping device.
 * @param fb is a pointer to the frame buffer device configuration.
 * @param dev_id is the device identification number used for this device.
 */
static int fb_makemmdev(struct fb_conf * fb, dev_t dev_id)
{
    struct dev_info * dev;

    dev = kzalloc(sizeof(struct dev_info));
    if (!dev)
        return -ENOMEM;

    dev->dev_id = dev_id;
    dev->drv_name = "fb_mm";
    ksprintf(dev->dev_name, SPECNAMELEN, "fbmm%i", DEV_MINOR(dev_id));
    dev->flags = 0;
    dev->block_size = 1;
    dev->read = devnull_read;
    dev->write = devfull_write;
    dev->ioctl = fb_mm_ioctl;
    dev->mmap = fb_mmap;
    dev->opt_data = fb;

    if (make_dev(dev, 0, 0, 0666, NULL)) {
        return -ENODEV;
    }

    return 0;
}

/**
 * Draw a splash screen.
 */
static void draw_splash(struct fb_conf * fb)
{
    const size_t pitch = fb->pitch;
    const size_t base = fb->mem.b_data;
    size_t col = 0;
    size_t row = 0;

    for (int i = 0; i < splash_width * splash_height; i++) {
        uint32_t pxl[3];

        if (i % splash_width == 0) {
            col = 0;
            row++;
        } else {
            col++;
        }

        SPLASH_PIXEL(splash_data, pxl);
        set_rgb_pixel(base, pitch, col, row,
                      (pxl[0] << 16) | (pxl[1] << 8) | pxl[2]);
    }
}

/**
 * Ioctl function for rame buffer memory mapping device.
 */
static int fb_mm_ioctl(struct dev_info * devnfo, uint32_t request,
                       void * arg, size_t arg_len)
{
    struct fb_conf * fb = (struct fb_conf *)devnfo->opt_data;

    KASSERT((uintptr_t)fb > 4096, "fb should be set to some meaningful value");

    switch (request) {
    case IOCTL_FB_GETRES: /* Get framebuffer resolution */
        if (arg_len < sizeof(struct fb_resolution)) {
            return -EINVAL;
        } else {
            struct fb_resolution * fbres = (struct fb_resolution *)arg;

            fbres->width = fb->width;
            fbres->height = fb->height;
            fbres->depth = fb->depth;
            fbres->pitch = fb->pitch;
        }
        break;
    case IOCTL_FB_SETRES: /* Set framebuffer resolution. */
        if (arg_len < sizeof(struct fb_resolution)) {
            return -EINVAL;
        } else {
            struct fb_resolution * fbres = (struct fb_resolution *)arg;

            return fb->set_resolution(fb, fbres->width, fbres->height,
                                      fbres->depth);
        }
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

/**
 * Mmap function for rame buffer memory mapping device.
 */
static int fb_mmap(struct dev_info * devnfo, size_t blkno, size_t bsize,
                   int flags, struct buf ** bp_out)
{
    struct fb_conf * fb = (struct fb_conf *)devnfo->opt_data;

    KASSERT((uintptr_t)fb > 4096, "fb should be set to some meaningful value");

    /*
     * We only need to return a pointer to the buffer and shmem/mmap will handle
     * the rest, like mapping it to the process memory space.
     */

    *bp_out = &fb->mem;

    return 0;
}
