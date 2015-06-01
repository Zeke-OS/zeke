/**
 *******************************************************************************
 * @file    bcm2835_prop.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 Property interface.
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

#pragma once
#ifndef BCM2835_PROP_H
#define BCM2835_PROP_H

#include <stdint.h>

#define BCM2835_PROP_REQUEST 0x0
#define BCM2835_PROP_TAG_END 0x0

/* VideoCore */
#define BCM2835_PROP_TAG_GET_FIRMWARE           0x00000001
/* HW */
#define BCM2835_PROP_TAG_GET_GET_BOARD_MODEL    0x00010001
#define BCM2835_PROP_TAG_GET_BOARD_REVISION     0x00010002
#define BCM2835_PROP_TAG_GET_MAC_ADDRESS        0x00010003
#define BCM2835_PROP_TAG_GET_BOARD_SERIAL       0x00010004
#define BCM2835_PROP_TAG_GET_ARM_MEMORY         0x00010005
#define BCM2835_PROP_TAG_GET_VC_MEMORY          0x00010006
#define BCM2835_PROP_TAG_GET_CLOCKS             0x00010007
/* Config */
#define BCM2835_PROP_TAG_GET_CMDLINE            0x00050001
/* Resources */
#define BCM2835_PROP_TAG_GET_DMA_CHANS          0x00060001
/* Power */
#define BCM2835_PROP_TAG_GET_PWR_STATE          0x00020001
#define BCM2835_PROP_TAG_GET_TIMING             0x00020002
#define BCM2835_PROP_TAG_SET_PWR_STATE          0x00028001
/* Clocks */
#define BCM2835_PROP_TAG_GET_CLK_STATE          0x00030001
#define BCM2835_PROP_TAG_SET_CLK_STATE          0x00038001
#define BCM2835_PROP_TAG_GET_CLK_RATE           0x00030002
#define BCM2835_PROP_TAG_SET_CLK_RATE           0x00038002
#define BCM2835_PROP_TAG_GET_MAX_CLK_RATE       0x00030004
#define BCM2835_PROP_TAG_GET_MIN_CLK_RATE       0x00030007
#define BCM2835_PROP_TAG_GET_TURBO              0x00030009
#define BCM2835_PROP_TAG_SET_TURBO              0x00038009
/* Voltage */
#define BCM2835_PROP_TAG_GET_VOLTAGE            0x00030003
#define BCM2835_PROP_TAG_SET_VOLTAGE            0x00038003
#define BCM2835_PROP_TAG_GET_MAX_VOLTAGE        0x00030005
#define BCM2835_PROP_TAG_GET_MIN_VOLTAGE        0x00030008
#define BCM2835_PROP_TAG_GET_TEMP               0x00030006
#define BCM2835_PROP_TAG_GET_MAX_TEMP           0x0003000a
/* Memory */
#define BCM2835_PROP_TAG_ALLOC_MEM              0x0003000c
#define BCM2835_PROP_TAG_LOCK_MEM               0x0003000d
#define BCM2835_PROP_TAG_UNLOCK_MEM             0x0003000e
#define BCM2835_PROP_TAG_RELE_MEM               0x0003000f
#define BCM2835_PROP_TAG_EXEC_CODE              0x00030010
#define BCM2835_PROP_TAG_GET_DISPMANX_MEM_HNDL  0x00030014
#define BCM2835_PROP_TAG_GET_EDID_BLOCK         0x00030020
/* FB */
#define BCM2835_PROP_TAG_FB_ALLOC_BUF           0x00040001
#define BCM2835_PROP_TAG_FB_RELE_BUF            0x00048001
#define BCM2835_PROP_TAG_FB_BLANK_SCREEN        0x00040002
#define BCM2835_PROP_TAG_FB_GET_PHYSDISP_SIZE   0x00040003
#define BCM2835_PROP_TAG_FB_TEST_PHYSDISP_SIZE  0x00044003
#define BCM2835_PROP_TAG_FB_SET_PHYSDISP_SIZE   0x00044003
#define BCM2835_PROP_TAG_FB_GET_VIRT_BUF_SIZE   0x00040004
#define BCM2835_PROP_TAG_FB_TEST_VIRT_BUF_SIZE  0x00044004
#define BCM2835_PROP_TAG_FB_SET_VIRT_BUF_SIZE   0x00048004
#define BCM2835_PROP_TAG_FB_GET_DEPTH           0x00040005
#define BCM2835_PROP_TAG_FB_TEST_DEPTH          0x00044005
#define BCM2835_PROP_TAG_FB_SET_DEPTH           0x00048005
#define BCM2835_PROP_TAG_FB_GET_PXL_ORDER       0x00040006
#define BCM2835_PROP_TAG_FB_TEST_PXL_ORDER      0x00044006
#define BCM2835_PROP_TAG_FB_SET_PXL_ORDER       0x00048006
#define BCM2835_PROP_TAG_FB_GET_ALPHA_MODE      0x00040007
#define BCM2835_PROP_TAG_FB_TEST_ALPHA_MODE     0x00044007
#define BCM2835_PROP_TAG_FB_SET_ALPHA_MODE      0x00048007
#define BCM2835_PROP_TAG_FB_GET_PITCH           0x00040008
#define BCM2835_PROP_TAG_FB_GET_VIRT_OFFSET     0x00040009
#define BCM2835_PROP_TAG_FB_TEST_VIRT_OFFSET    0x00044009
#define BCM2835_PROP_TAG_FB_SET_VIRT_OFFSET     0x00048009
#define BCM2835_PROP_TAG_FB_GET_OVERSCAN        0x0004000a
#define BCM2835_PROP_TAG_FB_TEST_OVERSCAN       0x0004400a
#define BCM2835_PROP_TAG_FB_SET_OVERSCAN        0x0004800a
#define BCM2835_PROP_TAG_FB_GET_PALETTE         0x0004000b
#define BCM2835_PROP_TAG_FB_TEST_PALETTE        0x0004400b
#define BCM2835_PROP_TAG_FB_SET_PALETTE         0x0004800b
#define BCM2835_PROP_TAG_FB_SET_CURSOR_INFO     0x00008011
#define BCM2835_PROP_TAG_FB_SET_CURSOR_STATE    0x00008010


/**
 * Make a property request to BCM2835 VC.
 * @param request is a regularly formatted property request but it doesn't
 *                have to be in any special memory region as the subsystem
 *                will handle that part.
 * @return Returns 0 if succeed and copies response data to request;
 *         Otherwise returns an error code, a negative errno value.
 */
int bcm2835_prop_request(uint32_t * request);

#endif /* BCM2835_PROP_H */
