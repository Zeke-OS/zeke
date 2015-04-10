/**
 *******************************************************************************
 * @file    bcm2835_fb.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 frame buffer driver.
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

#include <buf.h>
#include <hal/core.h>
#include <hal/fb.h>
#include <hal/hw_timers.h>
#include <hal/mmu.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <ptmapper.h>
#include "bcm2835_mailbox.h"
#include "bcm2835_mmio.h"
#include "bcm2835_timers.h"

#if configBCM_MB == 0
#error configBCM_MB is required by the fb driver
#endif

struct bcm2835_fb_config {
    uint32_t width;             /*!< Width of the requested frame buffer. */
    uint32_t height;            /*!< Height of the requested frame buffer. */
    uint32_t virtual_width;     /*!< Virtual width. */
    uint32_t virtual_height;    /*!< Virtual height. */
    uint32_t pitch;             /*!< Pitch. (Set by the GPU) */
    uint32_t depth;             /*!< Requested number of bits per pixel. */
    uint32_t x_offset;
    uint32_t y_offset;
    uint32_t fb_paddr;          /*!< Pointer to the fb set by the GPU. */
    uint32_t size;              /*!< Size of the fb set by the GPU. */
    uint16_t cmap[256];
};

static struct buf * fb_mbuf;
static mmu_region_t bcm2835_fb_region = {
    .num_pages  = 1, /* TODO We may want to calculate this */
    .ap         = MMU_AP_RWNA,
    .control    = (MMU_CTRL_MEMTYPE_DEV | MMU_CTRL_XN),
    .pt         = &mmu_pagetable_master
};

static int set_resolution(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth);
static void set_fb_config(struct bcm2835_fb_config * fb_config,
                          uint32_t width, uint32_t height, size_t depth);
static int commit_fb_config(struct bcm2835_fb_config * fb_config);

static int bcm2835_fb_init(void) __attribute__((constructor));
static int bcm2835_fb_init(void)
{
    SUBSYS_DEP(vralloc_init);
    SUBSYS_INIT("BCM2835_fb");

    int err;
    struct bcm2835_fb_config fb_bcm;
    struct fb_conf * fb;

    fb_mbuf = geteblk_special(sizeof(struct bcm2835_fb_config),
                              MMU_CTRL_MEMTYPE_SO);
    if (!fb_mbuf || fb_mbuf->b_data == 0) {
        KERROR(KERROR_ERR, "Unable to get a mailbuffer\n");

        return -ENOMEM;
    }

    set_fb_config(&fb_bcm, 640, 480, 24);
    err = commit_fb_config(&fb_bcm);
    if (err)
        return err;

    bcm2835_fb_region.vaddr = fb_bcm.fb_paddr;
    bcm2835_fb_region.paddr = fb_bcm.fb_paddr;
    mmu_map_region(&bcm2835_fb_region);

    /* Register a new frame buffer */
    fb = kmalloc(sizeof(struct fb_conf));
    *fb = (struct fb_conf){
        .width  = fb_bcm.width,
        .height = fb_bcm.height,
        .pitch  = fb_bcm.pitch,
        .depth  = fb_bcm.depth,
        .base   = fb_bcm.fb_paddr,
        .set_resolution = set_resolution,
    };
    fb_register(fb);

    return 0;
}

static int set_resolution(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth)
{
    struct bcm2835_fb_config fb_bcm;

    set_fb_config(&fb_bcm, width, height, depth);

    return commit_fb_config(&fb_bcm);
}

static void set_fb_config(struct bcm2835_fb_config * fb,
                          uint32_t width, uint32_t height, size_t depth)
{
    memset(fb, 0, sizeof(struct bcm2835_fb_config));
    fb->width = width;
    fb->height = height;
    fb->virtual_width = width;
    fb->virtual_height = height;
    fb->depth = depth;
    fb->x_offset = 0;
    fb->y_offset = 0;
}

static int commit_fb_config(struct bcm2835_fb_config * fb)
{
    int err;
    uint32_t resp;

    memcpy((char *)fb_mbuf->b_data, fb, sizeof(struct bcm2835_fb_config));

    /*
     * Adding 0x40000000 to the address tells the GPU to flush its cache after
     * writing a response.
     */
    err = bcm2835_writemailbox(BCM2835_MBCH_FB,
                               fb_mbuf->b_mmu.paddr + 0x40000000);
    if (err) {
        KERROR(KERROR_DEBUG, "\tGPU init failed (err: %u)\n", err);
        return -EIO;
    }

    err = bcm2835_readmailbox(BCM2835_MBCH_FB, &resp);
    if (err || resp) {
        KERROR(KERROR_DEBUG, "\tGPU init failed (err: %u, resp: %u)\n",
               err, resp);
        return -EIO;
    }

    KERROR(KERROR_INFO,
           "BCM_FB: addr = %p, width = %u, height = %u, "
           "bpp = %u, pitch = %u, size = %u\n",
           (void *)fb->fb_paddr, fb->width, fb->height,
           fb->depth, fb->pitch, fb->size);

    memcpy(fb, (char *)fb_mbuf->b_data, sizeof(struct bcm2835_fb_config));

    return 0;
}
