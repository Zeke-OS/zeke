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
#include <kmem.h>
#include <kstring.h>
#include <libkern.h>
#include <ptmapper.h>
#include "bcm2835_mailbox.h"
#include "bcm2835_mmio.h"
#include "bcm2835_prop.h"
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

/*
 * Data buffer between ARM and VC.
 */
static struct buf * fb_databuf;
static mmu_region_t bcm2835_fb_region = {
    .ap         = MMU_AP_RWNA,
    .control    = (MMU_CTRL_MEMTYPE_DEV | MMU_CTRL_XN),
    .pt         = &mmu_pagetable_master
};
#define fb_mailbuf              ((uint32_t *)fb_databuf->b_data)
#define fb_mailbuf_paddr        (fb_databuf->b_mmu.paddr)
#define fb_cursor_data          ((uint32_t *)(fb_databuf->b_data + 1024))
#define fb_cursor_data_paddr    (fb_databuf->b_mmu.paddr + 1024)

static int set_resolution(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth);
static void set_fb_config(struct bcm2835_fb_config * fb_config,
                          uint32_t width, uint32_t height, size_t depth);
static void update_fb_mm(struct fb_conf * fb,
                         struct bcm2835_fb_config * bcm_fb);
static int commit_fb_config(struct bcm2835_fb_config * fb_config);
static int set_cursor_state(int enable, int x, int y);
static int set_cursor_info(void);

static int __kinit__ bcm2835_fb_init(void)
{
    SUBSYS_DEP(vralloc_init);
    SUBSYS_INIT("BCM2835_fb");

    struct bcm2835_fb_config bcm_fb;
    struct fb_conf * fb;
    int err;

    fb_databuf = geteblk_special(sizeof(struct bcm2835_fb_config),
                              MMU_CTRL_MEMTYPE_SO);
    if (!fb_databuf || fb_databuf->b_data == 0) {
        KERROR(KERROR_ERR, "Unable to get a mailbuffer\n");

        return -ENOMEM;
    }

    set_fb_config(&bcm_fb, 640, 480, 24);
    err = commit_fb_config(&bcm_fb);
    if (err)
        return err;

    /* Register a new frame buffer */
    fb = kmalloc(sizeof(struct fb_conf));
    *fb = (struct fb_conf){
        .feature = 0, /* TODO HW cursor flag */
        .width  = bcm_fb.width,
        .height = bcm_fb.height,
        .pitch  = bcm_fb.pitch,
        .depth  = bcm_fb.depth,
        .set_resolution = set_resolution,
        .set_hw_cursor_state = set_cursor_state,
    };
    fb_mm_initbuf(fb);
    update_fb_mm(fb, &bcm_fb);
    fb_register(fb);

    return 0;
}

static int set_resolution(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth)
{
    struct bcm2835_fb_config bcm_fb;
    int err;

    set_fb_config(&bcm_fb, width, height, depth);
    err = commit_fb_config(&bcm_fb);
    if (err)
        return err;

    update_fb_mm(fb, &bcm_fb);

    return 0;
}

static void set_fb_config(struct bcm2835_fb_config * bcm_fb,
                          uint32_t width, uint32_t height, size_t depth)
{
    memset(bcm_fb, 0, sizeof(struct bcm2835_fb_config));
    bcm_fb->width = width;
    bcm_fb->height = height;
    bcm_fb->virtual_width = width;
    bcm_fb->virtual_height = height;
    bcm_fb->depth = depth;
    bcm_fb->x_offset = 0;
    bcm_fb->y_offset = 0;
}

/**
 * Update memory region information.
 */
static void update_fb_mm(struct fb_conf * fb, struct bcm2835_fb_config * bcm_fb)
{
    size_t size = memalign_size(bcm_fb->size, MMU_PGSIZE_SECTION);

    bcm2835_fb_region.vaddr = bcm_fb->fb_paddr;
    bcm2835_fb_region.paddr = bcm_fb->fb_paddr;
    bcm2835_fb_region.num_pages = size / MMU_PGSIZE_SECTION;
    mmu_map_region(&bcm2835_fb_region); /* Map for the kernel */
    fb_mm_updatebuf(fb, &bcm2835_fb_region);

    KERROR(KERROR_INFO, "Number of fb pages: %u\n",
           bcm2835_fb_region.num_pages);
}

static int commit_fb_config(struct bcm2835_fb_config * fb)
{
    int err;
    uint32_t resp;

    memcpy(fb_mailbuf, fb, sizeof(struct bcm2835_fb_config));

    /*
     * Adding 0x40000000 to the address tells the GPU to flush its cache after
     * writing a response.
     */
    err = bcm2835_writemailbox(BCM2835_MBCH_FB,
                               fb_mailbuf_paddr + 0x40000000);
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

    memcpy(fb, fb_mailbuf, sizeof(struct bcm2835_fb_config));

    KERROR(KERROR_INFO,
           "BCM_FB: addr = %p, width = %u, height = %u, "
           "bpp = %u, pitch = %u, size = %u\n",
           (void *)fb->fb_paddr, fb->width, fb->height,
           fb->depth, fb->pitch, fb->size);

    return 0;
}

static int blank_screen(int state)
{
    uint32_t mbuf[7] __attribute__((aligned (16)));
    int err;

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    mbuf[2] = BCM2835_PROP_TAG_FB_BLANK_SCREEN;
    mbuf[3] = 4;            /* Value buf size and req/resp */
    mbuf[4] = 4;            /* Value size */
    mbuf[5] = (state) ? 1 : 0;
    mbuf[6] = BCM2835_PROP_TAG_END;

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    return (mbuf[5] & 1);
}

static int set_cursor_state(int enable, int x, int y)
{
    uint32_t mbuf[10] __attribute__((aligned (16)));
    int err;

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    /* Set cursor state */
    mbuf[2] = 0x00008010;
    mbuf[3] = 16;           /* Value buf size and req/resp */
    mbuf[4] = 16;           /* Value size */
    mbuf[5] = (enable) ? 1 : 0;
    mbuf[6] = x;
    mbuf[7] = y;
    mbuf[8] = 0;            /* Flags: 0 = disp coords; 1 = fb coords */
    mbuf[9] = BCM2835_PROP_TAG_END;

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    return (mbuf[5] & 1) ? -EINVAL : 0;
}

/* TODO Fix me */
static int set_cursor_info(void)
{

    uint32_t mbuf[12] __attribute__((aligned (16)));
    int err;

    memset(fb_cursor_data, 0x77, 512);

    /* Format a message */
    mbuf[0] = sizeof(mbuf); /* Size */
    mbuf[1] = 0;            /* Request */
    /* Tags */
    mbuf[2] = BCM2835_PROP_TAG_FB_SET_CURSOR_INFO;
    mbuf[3] = 24;           /* Value buf size and req/resp */
    mbuf[4] = 24;           /* Value size */
    mbuf[5] = 16;           /* width */
    mbuf[6] = 16;           /* height */
    mbuf[7] = 0;            /* unused */
    mbuf[8] = fb_cursor_data_paddr;
    mbuf[9] = 0;            /* hotspotX */
    mbuf[10] = 0;           /* hotspotY */
    mbuf[11] = BCM2835_PROP_TAG_END;

    err = bcm2835_prop_request(mbuf);
    if (err)
        return err;

    return (mbuf[5] & 1) ? -EINVAL : 0;
}
