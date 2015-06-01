/**
 *******************************************************************************
 * @file    emmc.h
 * @author  Olli Vanhoja
 * @brief   emmc driver.
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
 *
 *
 * Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *******************************************************************************
 */

#include <stdint.h>
#include <fs/devfs.h>

/* Delays */
#ifdef configQEMU_GUEST
#define SD_CMD_UDELAY 0
#else
#define SD_CMD_UDELAY 1000
#endif

/* SD Clock Frequencies (in Hz) */
#define SD_CLOCK_ID         400000
#define SD_CLOCK_NORMAL     25000000
#define SD_CLOCK_HIGH       50000000
#define SD_CLOCK_100        100000000
#define SD_CLOCK_208        208000000


/* Register addresses */
#define EMMC_BASE           0x20300000
#define EMMC_ARG2           0
#define EMMC_BLKSIZECNT     4
#define EMMC_ARG1           8
#define EMMC_CMDTM          0xC
#define EMMC_RESP0          0x10
#define EMMC_RESP1          0x14
#define EMMC_RESP2          0x18
#define EMMC_RESP3          0x1C
#define EMMC_DATA           0x20
#define EMMC_STATUS         0x24
#define EMMC_CONTROL0       0x28
#define EMMC_CONTROL1       0x2C
#define EMMC_INTERRUPT      0x30
#define EMMC_IRPT_MASK      0x34
#define EMMC_IRPT_EN        0x38
#define EMMC_CONTROL2       0x3C
#define EMMC_CAPABILITIES_0 0x40
#define EMMC_CAPABILITIES_1 0x44
#define EMMC_FORCE_IRPT     0x50
#define EMMC_BOOT_TIMEOUT   0x70
#define EMMC_DBG_SEL        0x74
#define EMMC_EXRDFIFO_CFG   0x80
#define EMMC_EXRDFIFO_EN    0x84
#define EMMC_TUNE_STEP      0x88
#define EMMC_TUNE_STEPS_STD 0x8C
#define EMMC_TUNE_STEPS_DDR 0x90
#define EMMC_SPI_INT_SPT    0xF0
#define EMMC_SLOTISR_VER    0xFC

#define SD_CMD_INDEX(a)             ((a) << 24)
#define SD_CMD_TYPE_NORMAL          0x0
#define SD_CMD_TYPE_SUSPEND         (1 << 22)
#define SD_CMD_TYPE_RESUME          (2 << 22)
#define SD_CMD_TYPE_ABORT           (3 << 22)
#define SD_CMD_TYPE_MASK            (3 << 22)
#define SD_CMD_ISDATA               (1 << 21)
#define SD_CMD_IXCHK_EN             (1 << 20)
#define SD_CMD_CRCCHK_EN            (1 << 19)
/* For no response */
#define SD_CMD_RSPNS_TYPE_NONE      0
/* For response R2 (with CRC), R3,4 (no CRC) */
#define SD_CMD_RSPNS_TYPE_136       (1 << 16)
/*  For responses R1, R5, R6, R7 (with CRC) */
#define SD_CMD_RSPNS_TYPE_48        (2 << 16)
/* For responses R1b, R5b (with CRC) */
#define SD_CMD_RSPNS_TYPE_48B       (3 << 16)
#define SD_CMD_RSPNS_TYPE_MASK      (3 << 16)
#define SD_CMD_MULTI_BLOCK          (1 << 5)
#define SD_CMD_DAT_DIR_HC           0
#define SD_CMD_DAT_DIR_CH           (1 << 4)
#define SD_CMD_AUTO_CMD_EN_NONE     0
#define SD_CMD_AUTO_CMD_EN_CMD12    (1 << 2)
#define SD_CMD_AUTO_CMD_EN_CMD23    (2 << 2)
#define SD_CMD_BLKCNT_EN            (1 << 1)
#define SD_CMD_DMA                  1

#define SD_ERR_CMD_TIMEOUT          0
#define SD_ERR_CMD_CRC              1
#define SD_ERR_CMD_END_BIT          2
#define SD_ERR_CMD_INDEX            3
#define SD_ERR_DATA_TIMEOUT         4
#define SD_ERR_DATA_CRC             5
#define SD_ERR_DATA_END_BIT         6
#define SD_ERR_CURRENT_LIMIT        7
#define SD_ERR_AUTO_CMD12           8
#define SD_ERR_ADMA                 9
#define SD_ERR_TUNING               10
#define SD_ERR_RSVD                 11

#define SD_ERR_MASK_CMD_TIMEOUT     (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC         (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT     (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX       (1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT    (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC        (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT    (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT   (1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12      (1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA            (1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING          (1 << (16 + SD_ERR_CMD_TUNING))

#define SD_COMMAND_COMPLETE         1
#define SD_TRANSFER_COMPLETE        (1 << 1)
#define SD_BLOCK_GAP_EVENT          (1 << 2)
#define SD_DMA_INTERRUPT            (1 << 3)
#define SD_BUFFER_WRITE_READY       (1 << 4)
#define SD_BUFFER_READ_READY        (1 << 5)
#define SD_CARD_INSERTION           (1 << 6)
#define SD_CARD_REMOVAL             (1 << 7)
#define SD_CARD_INTERRUPT           (1 << 8)

#define SD_RESP_NONE        SD_CMD_RSPNS_TYPE_NONE
#define SD_RESP_R1          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R1b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R2          (SD_CMD_RSPNS_TYPE_136 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R3          SD_CMD_RSPNS_TYPE_48
#define SD_RESP_R4          SD_CMD_RSPNS_TYPE_136
#define SD_RESP_R5          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R5b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R6          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R7          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)

#define SD_DATA_READ        (SD_CMD_ISDATA | SD_CMD_DAT_DIR_CH)
#define SD_DATA_WRITE       (SD_CMD_ISDATA | SD_CMD_DAT_DIR_HC)

#define SD_CMD_RESERVED(a)  0xffffffff

#define SUCCESS(a)          ((a)->last_cmd_success)
#define FAIL(a)             ((a)->last_cmd_success == 0)
#define TIMEOUT(a)          (FAIL(a) && (a->last_error == 0))
#define CMD_TIMEOUT(a)      (FAIL(a) && (a->last_error & (1 << 16)))
#define CMD_CRC(a)          (FAIL(a) && (a->last_error & (1 << 17)))
#define CMD_END_BIT(a)      (FAIL(a) && (a->last_error & (1 << 18)))
#define CMD_INDEX(a)        (FAIL(a) && (a->last_error & (1 << 19)))
#define DATA_TIMEOUT(a)     (FAIL(a) && (a->last_error & (1 << 20)))
#define DATA_CRC(a)         (FAIL(a) && (a->last_error & (1 << 21)))
#define DATA_END_BIT(a)     (FAIL(a) && (a->last_error & (1 << 22)))
#define CURRENT_LIMIT(a)    (FAIL(a) && (a->last_error & (1 << 23)))
#define ACMD12_ERROR(a)     (FAIL(a) && (a->last_error & (1 << 24)))
#define ADMA_ERROR(a)       (FAIL(a) && (a->last_error & (1 << 25)))
#define TUNING_ERROR(a)     (FAIL(a) && (a->last_error & (1 << 26)))

struct emmc_capabilities {
    uint32_t hci_ver;
    uint32_t capabilities[2];
};

struct sd_scr {
    uint32_t    scr[2];
    uint32_t    sd_bus_widths;
    int         sd_version;
};

struct emmc_block_dev {
    struct dev_info dev;

    uint8_t *cid;
    size_t cid_len;

    uint32_t card_supports_sdhc;
    uint32_t card_supports_18v;
    uint32_t card_ocr;
    uint32_t card_rca;
    uint32_t last_interrupt;
    uint32_t last_error;

    struct sd_scr *scr;

    int failed_voltage_switch;

    uint32_t last_cmd_reg;
    uint32_t last_cmd;
    uint32_t last_cmd_success;
    uint32_t last_r0;
    uint32_t last_r1;
    uint32_t last_r2;
    uint32_t last_r3;

    void *buf;
    int blocks_to_transfer;
    size_t block_size;
    int use_sdma;
    int card_removal;
    uint32_t base_clock;
};

struct emmc_hw_support {
    int (*emmc_power_cycle)(void);
    uint32_t (*sd_get_base_clock_hz)(struct emmc_capabilities * cap);
};
extern struct emmc_hw_support emmc_hw;
