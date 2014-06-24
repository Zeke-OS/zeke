/**
 *******************************************************************************
 * @file    libkern.h
 * @author  Olli Vanhoja
 * @brief   RPI emmc driver.
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

/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS     - SD Group Physical Layer Simplified Specification ver 3.00
 * HCSS        - SD Group Host Controller Simplified Specification ver 3.00
 *
 * Broadcom BCM2835 Peripherals Guide
 */

#include <autoconf.h>
#include <stdint.h>
#include <libkern.h>
#include <kstring.h>
#include <kmalloc.h>
#include <kerror.h>
#include <hal/hw_timers.h>
#include <fs/block.h>
#include "../bcm2835/bcm2835_mmio.h"
#include "../bcm2835/bcm2835_timers.h"
#include "../bcm2835/bcm2835_mailbox.h"

#if configBCM_MB == 0
#error configBCM_MB is required by rpi_emmc driver
#endif

#if configDEBUG >= KERROR_DEBUG
#define EMMC_DEBUG 1
#endif

/* Configuration options */

/* Enable 1.8V support */
#define SD_1_8V_SUPPORT

/* Enable 4-bit support */
#define SD_4BIT_DATA

/* SD Clock Frequencies (in Hz) */
#define SD_CLOCK_ID         400000
#define SD_CLOCK_NORMAL     25000000
#define SD_CLOCK_HIGH       50000000
#define SD_CLOCK_100        100000000
#define SD_CLOCK_208        208000000

/*
 * Enable SDXC maximum performance mode
 * Requires 150 mA power so disabled on the RPi for now
 */
/*#define SDXC_MAXIMUM_PERFORMANCE */

/* Enable SDMA support */
/*#define SDMA_SUPPORT */

/* TODO */
/* SDMA buffer address */
/*#define SDMA_BUFFER     0x6000 */
/*#define SDMA_BUFFER_PA  (SDMA_BUFFER + 0xC0000000) */

/* Enable card interrupts */
/*#define SD_CARD_INTERRUPTS */

/* Enable EXPERIMENTAL (and possibly DANGEROUS) SD write support */
#define SD_WRITE_SUPPORT 1

/* The particular SDHCI implementation */
#define SDHCI_IMPLEMENTATION_GENERIC        0
#define SDHCI_IMPLEMENTATION_BCM_2708       1
#define SDHCI_IMPLEMENTATION                SDHCI_IMPLEMENTATION_BCM_2708

static char driver_name[] = "emmc";
static char device_name[] = "emmc0"; /* We use a single device name as there
                                         * is only one card slot in the RPi */

static uint32_t hci_ver;
static uint32_t capabilities_0;
static uint32_t capabilities_1;

static uint32_t mailbuffer[10] __attribute__((aligned (16)));

struct sd_scr {
    uint32_t    scr[2];
    uint32_t    sd_bus_widths;
    int         sd_version;
};

struct emmc_block_dev {
    struct block_dev bd;

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

#define SUCCESS(a)          (a->last_cmd_success)
#define FAIL(a)             (a->last_cmd_success == 0)
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

#define SD_VER_UNKNOWN      0
#define SD_VER_1            1
#define SD_VER_1_1          2
#define SD_VER_2            3
#define SD_VER_3            4
#define SD_VER_4            5

static char *sd_versions[] = {
    "unknown", "1.0 and 1.01", "1.10", "2.00", "3.0x", "4.xx"
};

#ifdef EMMC_DEBUG
static char *err_irpts[] = {
    "CMD_TIMEOUT", "CMD_CRC", "CMD_END_BIT", "CMD_INDEX",
    "DATA_TIMEOUT", "DATA_CRC", "DATA_END_BIT", "CURRENT_LIMIT",
    "AUTO_CMD12", "ADMA", "TUNING", "RSVD"
};
#endif

int sd_read(struct block_dev *, off_t, uint8_t *, size_t buf_size);
int sd_write(struct block_dev *, off_t, uint8_t *, size_t buf_size);

static uint32_t sd_commands[] = {
    SD_CMD_INDEX(0),
    SD_CMD_RESERVED(1),
    SD_CMD_INDEX(2) | SD_RESP_R2,
    SD_CMD_INDEX(3) | SD_RESP_R6,
    SD_CMD_INDEX(4),
    SD_CMD_INDEX(5) | SD_RESP_R4,
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_INDEX(7) | SD_RESP_R1b,
    SD_CMD_INDEX(8) | SD_RESP_R7,
    SD_CMD_INDEX(9) | SD_RESP_R2,
    SD_CMD_INDEX(10) | SD_RESP_R2,
    SD_CMD_INDEX(11) | SD_RESP_R1,
    SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_INDEX(15),
    SD_CMD_INDEX(16) | SD_RESP_R1,
    SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ |
                       SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(20) | SD_RESP_R1b,
    SD_CMD_RESERVED(21),
    SD_CMD_RESERVED(22),
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE |
                       SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_RESERVED(26),
    SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(28) | SD_RESP_R1b,
    SD_CMD_INDEX(29) | SD_RESP_R1b,
    SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(31),
    SD_CMD_INDEX(32) | SD_RESP_R1,
    SD_CMD_INDEX(33) | SD_RESP_R1,
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_INDEX(38) | SD_RESP_R1b,
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_RESERVED(41),
    SD_CMD_RESERVED(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_RESERVED(51),
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_INDEX(55) | SD_RESP_R1,
    SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

static uint32_t sd_acommands[] = {
    SD_CMD_RESERVED(0),
    SD_CMD_RESERVED(1),
    SD_CMD_RESERVED(2),
    SD_CMD_RESERVED(3),
    SD_CMD_RESERVED(4),
    SD_CMD_RESERVED(5),
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_RESERVED(7),
    SD_CMD_RESERVED(8),
    SD_CMD_RESERVED(9),
    SD_CMD_RESERVED(10),
    SD_CMD_RESERVED(11),
    SD_CMD_RESERVED(12),
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_RESERVED(15),
    SD_CMD_RESERVED(16),
    SD_CMD_RESERVED(17),
    SD_CMD_RESERVED(18),
    SD_CMD_RESERVED(19),
    SD_CMD_RESERVED(20),
    SD_CMD_RESERVED(21),
    SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_RESERVED(24),
    SD_CMD_RESERVED(25),
    SD_CMD_RESERVED(26),
    SD_CMD_RESERVED(27),
    SD_CMD_RESERVED(28),
    SD_CMD_RESERVED(29),
    SD_CMD_RESERVED(30),
    SD_CMD_RESERVED(31),
    SD_CMD_RESERVED(32),
    SD_CMD_RESERVED(33),
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_RESERVED(38),
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_INDEX(41) | SD_RESP_R3,
    SD_CMD_INDEX(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_RESERVED(55),
    SD_CMD_RESERVED(56),
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

/* The actual command indices */
#define GO_IDLE_STATE           0
#define ALL_SEND_CID            2
#define SEND_RELATIVE_ADDR      3
#define SET_DSR                 4
#define IO_SET_OP_COND          5
#define SWITCH_FUNC             6
#define SELECT_CARD             7
#define DESELECT_CARD           7
#define SELECT_DESELECT_CARD    7
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define VOLTAGE_SWITCH          11
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define GO_INACTIVE_STATE       15
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLOCK       17
#define READ_MULTIPLE_BLOCK     18
#define SEND_TUNING_BLOCK       19
#define SPEED_CLASS_CONTROL     20
#define SET_BLOCK_COUNT         23
#define WRITE_BLOCK             24
#define WRITE_MULTIPLE_BLOCK    25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START      32
#define ERASE_WR_BLK_END        33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56

#define IS_APP_CMD              0x80000000
#define ACMD(a)                 (a | IS_APP_CMD)
#define SET_BUS_WIDTH           (6 | IS_APP_CMD)
#define SD_STATUS               (13 | IS_APP_CMD)
#define SEND_NUM_WR_BLOCKS      (22 | IS_APP_CMD)
#define SET_WR_BLK_ERASE_COUNT  (23 | IS_APP_CMD)
#define SD_SEND_OP_COND         (41 | IS_APP_CMD)
#define SET_CLR_CARD_DETECT     (42 | IS_APP_CMD)
#define SEND_SCR                (51 | IS_APP_CMD)

#define SD_RESET_CMD            (1 << 25)
#define SD_RESET_DAT            (1 << 26)
#define SD_RESET_ALL            (1 << 24)

#define SD_GET_CLOCK_DIVIDER_FAIL    0xffffffff

void rpi_emmc_init(void) __attribute__((constructor));
void rpi_emmc_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(fs_init);

    struct block_device *sd_dev = NULL;
    if (sd_card_init(&sd_dev) == 0) {
        struct block_device *c_dev = sd_dev;
#ifdef ENABLE_BLOCK_CACHE
        uintptr_t cache_start = alloc_buf(BLOCK_CACHE_SIZE);

        if (cache_start != 0)
            cache_init(sd_dev, &c_dev, cache_start, BLOCK_CACHE_SIZE);
#endif

#ifdef configMBR
        mbr_register(c_dev, (void*)0, (void*)0);
#endif
    }

    SUBSYS_INITFINI("rpi_emmc OK");
}

static void sd_power_off()
{
    /* Power off the SD card */
    uint32_t control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
    /* Set SD Bus Power bit off in Power Control Register */
    control0 &= ~(1 << 8);
    mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
}

static uint32_t sd_get_base_clock_hz()
{
    uint32_t base_clock;
#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_GENERIC
    capabilities_0 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_0);
    base_clock = ((capabilities_0 >> 8) & 0xff) * 1000000;
#elif SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
    /*
     * Get the base clock rate
     * set up the buffer
     */
    mailbuffer[0] = 8 * 4;      /* size of this message */
    mailbuffer[1] = 0;          /* this is a request */

    /* next comes the first tag */
    mailbuffer[2] = 0x00030002; /* get clock rate tag */
    mailbuffer[3] = 0x8;        /* value buffer size */
    mailbuffer[4] = 0x4;        /* is a request, value length = 4 */
    mailbuffer[5] = 0x1;        /* clock id + space to return clock id */
    mailbuffer[6] = 0;          /* space to return rate (in Hz) */

    /* closing tag */
    mailbuffer[7] = 0;

    /* send the message */
    bcm2835_writemailbox(BCM2835_MBCH_PROP, (uint32_t)(&mailbuffer));

    /* read the response */
    bcm2835_readmailbox(BCM2835_MBCH_PROP);

    if (mailbuffer[1] != BCM2835_STATUS_SUCCESS) {
        KERROR(KERROR_ERR,
               "EMMC: property mailbox did not return a valid response.\n");
        return 0;
    }

    if (mailbuffer[5] != 0x1) {
        KERROR(KERROR_ERR,
               "EMMC: property mailbox did not return a valid clock id.\n");
        return 0;
    }

    base_clock = mailbuffer[6];
#else
    KERROR(KERROR_ERR,
           "EMMC: get_base_clock_hz() is not implemented for this "
           "architecture.\n");
    return 0;
#endif

#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "EMMC: base clock rate is %i Hz\n",
                 base_clock);
        KERROR(KERROR_DEBUG, buf);
    }
#endif
    return base_clock;
}

#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
static int bcm_2708_power_off()
{
    /* Power off the SD card */
    /* set up the buffer */
    mailbuffer[0] = 8 * 4;  /* size of this message */
    mailbuffer[1] = 0;      /* this is a request */

    /* next comes the first tag */
    mailbuffer[2] = 0x00028001; /* set power state tag */
    mailbuffer[3] = 0x8;        /* value buffer size */
    mailbuffer[4] = 0x8;        /* is a request, value length = 8 */
    mailbuffer[5] = 0x0;        /* device id and device id also returned here */
    mailbuffer[6] = 0x2;  /* set power off, wait for stable and returns state */

    /* closing tag */
    mailbuffer[7] = 0;

    /* send the message */
    bcm2835_writemailbox(BCM2835_MBCH_PROP, (uint32_t)(&mailbuffer));

    /* read the response */
    bcm2835_readmailbox(BCM2835_MBCH_PROP);

    if (mailbuffer[1] != BCM2835_STATUS_SUCCESS) {
        KERROR(KERROR_ERR,
               "EMMC: bcm_2708_power_off(): property mailbox did not return "
               "a valid response.\n");
        return -1;
    }

    if (mailbuffer[5] != 0x0) {
        KERROR(KERROR_ERR, "EMMC: property mailbox did not return "
                           "a valid device id.\n");
        return -1;
    }

    if ((mailbuffer[6] & 0x3) != 0) {
#ifdef EMMC_DEBUG
        {
            char buf[80];
            ksprintf(buf, sizeof(buf),
                     "EMMC: bcm_2708_power_off(): "
                     "device did not power off successfully (%08x).\n",
                    mailbuffer[6]);
            KERROR(KERROR_DEBUG, buf);
        }
#endif
        return 1;
    }

    return 0;
}

static int bcm_2708_power_on()
{
    /* Power on the SD card */
    /* set up the buffer */
    mailbuffer[0] = 8 * 4;      /* size of this message */
    mailbuffer[1] = 0;          /* this is a request */

    /* next comes the first tag */
    mailbuffer[2] = 0x00028001; /* set power state tag */
    mailbuffer[3] = 0x8;        /* value buffer size */
    mailbuffer[4] = 0x8;        /* is a request, value length = 8 */
    mailbuffer[5] = 0x0;        /* device id and device id also returned here */
    mailbuffer[6] = 0x3;  /* set power off, wait for stable and returns state */

    /* closing tag */
    mailbuffer[7] = 0;

    /* send the message */
    bcm2835_writemailbox(BCM2835_MBCH_PROP, (uint32_t)(&mailbuffer));

    /* read the response */
    bcm2835_readmailbox(BCM2835_MBCH_PROP);

    if (mailbuffer[1] != BCM2835_STATUS_SUCCESS) {
        KERROR(KERROR_ERR,
               "EMMC: bcm_2708_power_on(): property mailbox did not return "
               "a valid response.\n");
        return -1;
    }

    if (mailbuffer[5] != 0x0) {
        KERROR(KERROR_ERR, "EMMC: property mailbox did not return "
                           "a valid device id.\n");
        return -1;
    }

    if ((mailbuffer[6] & 0x3) != 1) {
#ifdef EMMC_DEBUG
        char buf[80];
        ksprintf(buf, sizeof(buf),
                 "EMMC: bcm_2708_power_on(): "
                 "device did not power on successfully (%08x).\n",
                 mailbuffer[6]);
        KERROR(KERROR_DEBUG, buf);
#endif
        return 1;
    }

    return 0;
}

static int bcm_2708_power_cycle()
{
    if (bcm_2708_power_off() < 0)
        return -1;

    bcm_udelay(5000);

    return bcm_2708_power_on();
}
#endif

/* Set the clock dividers to generate a target value */
static uint32_t sd_get_clock_divider(uint32_t base_clock, uint32_t target_rate)
{
    /* TODO: implement use of preset value registers */

    uint32_t targetted_divisor = 0;
    if (target_rate > base_clock) {
        targetted_divisor = 1;
    } else {
        targetted_divisor = base_clock / target_rate;
        uint32_t mod = base_clock % target_rate;
        if (mod)
            targetted_divisor--;
    }

    /*
     * Decide on the clock mode to use
     * Currently only 10-bit divided clock mode is supported
     */

    if (hci_ver >= 2) {
        /*
         * HCI version 3 or greater supports 10-bit divided clock mode
         * This requires a power-of-two divider
         */

        /* Find the first bit set */
        int divisor = -1;
        for (int first_bit = 31; first_bit >= 0; first_bit--) {
            uint32_t bit_test = (1 << first_bit);
            if (targetted_divisor & bit_test) {
                divisor = first_bit;
                targetted_divisor &= ~bit_test;
                if (targetted_divisor) {
                    /* The divisor is not a power-of-two, increase it */
                    divisor++;
                }
                break;
            }
        }

        if (divisor == -1)
            divisor = 31;
        if (divisor >= 32)
            divisor = 31;

        if (divisor != 0)
            divisor = (1 << (divisor - 1));

        if (divisor >= 0x400)
            divisor = 0x3ff;

        uint32_t freq_select = divisor & 0xff;
        uint32_t upper_bits = (divisor >> 8) & 0x3;
        uint32_t ret = (freq_select << 8) | (upper_bits << 6) | (0 << 5);

#ifdef EMMC_DEBUG
        int denominator = 1;
        if (divisor != 0)
            denominator = divisor * 2;
        int actual_clock = base_clock / denominator;
        {
            char buf[80];
            ksprintf(buf, sizeof(buf),
                    "EMMC: base_clock: %i, target_rate: %i, divisor: %08x, "
                    "actual_clock: %i, ret: %08x\n", base_clock, target_rate,
                    divisor, actual_clock, ret);
            KERROR(KERROR_DEBUG, buf);
        }
#endif

        return ret;
    } else {
        KERROR(KERROR_ERR, "EMMC: unsupported host version\n");
        return SD_GET_CLOCK_DIVIDER_FAIL;
    }

}

/* Switch the clock rate whilst running */
static int sd_switch_clock_rate(uint32_t base_clock, uint32_t target_rate)
{
    /* Decide on an appropriate divider */
    uint32_t divider = sd_get_clock_divider(base_clock, target_rate);
    if (divider == SD_GET_CLOCK_DIVIDER_FAIL) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "EMMC: couldn't get a valid divider for target rate %i Hz\n",
                target_rate);
        KERROR(KERROR_DEBUG, buf);
        return -1;
    }

    /* Wait for the command inhibit (CMD and DAT) bits to clear */
    while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x3) {
        bcm_udelay(1000);
    }

    /* Set the SD clock off */
    uint32_t control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 &= ~(1 << 2);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bcm_udelay(2000);

    /* Write the new divider */
    control1 &= ~0xffe0;        /* Clear old setting + clock generator select */
    control1 |= divider;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bcm_udelay(2000);

    /* Enable the SD clock */
    control1 |= (1 << 2);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bcm_udelay(2000);

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                 "EMMC: successfully set clock rate to %i Hz\n",
                 target_rate);
        KERROR(KERROR_DEBUG, buf);
    }
#endif
    return 0;
}

/* Reset the CMD line */
static int sd_reset_cmd()
{
    uint32_t control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= SD_RESET_CMD;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    TIMEOUT_WAIT((mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_CMD) == 0,
                 1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_CMD) != 0) {
        KERROR(KERROR_ERR, "EMMC: CMD line did not reset properly\n");

        return -1;
    }
    return 0;
}

/* Reset the CMD line */
static int sd_reset_dat()
{
    uint32_t control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);

    control1 |= SD_RESET_DAT;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    TIMEOUT_WAIT((mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_DAT) == 0,
            1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_DAT) != 0) {
        KERROR(KERROR_ERR, "EMMC: DAT line did not reset properly\n");
        return -1;
    }

    return 0;
}

static void sd_issue_command_int(struct emmc_block_dev *dev, uint32_t cmd_reg,
        uint32_t argument, useconds_t timeout)
{
    dev->last_cmd_reg = cmd_reg;
    dev->last_cmd_success = 0;

    /* This is as per HCSS 3.7.1.1/3.7.2.2 */

    /* Check Command Inhibit */
    while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x1) {
        bcm_udelay(1000);
    }

    /* Is the command with busy? */
    if ((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) {
        /* With busy */

        /* Is is an abort command? */
        if ((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT) {
            /* Not an abort command */

            /* Wait for the data line to be free */
            while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) {
                bcm_udelay(1000);
            }
        }
    }

    /* Is this a DMA transfer? */
    int is_sdma = 0;
    if ((cmd_reg & SD_CMD_ISDATA) && (dev->use_sdma)) {
#ifdef EMMC_DEBUG
        {
            char buf[80];
            ksprintf(buf, sizeof(buf),
                    "SD: performing SDMA transfer, current INTERRUPT: %08x\n",
                    mmio_read(EMMC_BASE + EMMC_INTERRUPT));
            KERROR(KERROR_DEBUG, buf);
        }
#endif
        is_sdma = 1;
    }

    if (is_sdma) {
        /* Set system address register (ARGUMENT2 in RPi) */

        /* We need to define a 4 kiB aligned buffer to use here */
        /* Then convert its virtual address to a bus address */
        /* TODO set dma buffer */
        panic("no dma support");
        /*mmio_write(EMMC_BASE + EMMC_ARG2, SDMA_BUFFER_PA); */
    }

    /* Set block size and block count */
    /* For now, block size = 512 bytes, block count = 1, */
    /*  host SDMA buffer boundary = 4 kiB */
    if (dev->blocks_to_transfer > 0xffff) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: blocks_to_transfer too great (%i)\n",
               dev->blocks_to_transfer);
        KERROR(KERROR_ERR, buf);

        dev->last_cmd_success = 0;
        return;
    }
    uint32_t blksizecnt = dev->block_size | (dev->blocks_to_transfer << 16);
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, blksizecnt);

    /* Set argument 1 reg */
    mmio_write(EMMC_BASE + EMMC_ARG1, argument);

    if (is_sdma) {
        /* Set Transfer mode register */
        cmd_reg |= SD_CMD_DMA;
    }

    /* Set command reg */
    mmio_write(EMMC_BASE + EMMC_CMDTM, cmd_reg);

    bcm_udelay(2000);

    /* Wait for command complete interrupt */
    TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8001, timeout);
    uint32_t irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);

    /* Clear command complete status */
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0001);

    /* Test for errors */
    if ((irpts & 0xffff0001) != 0x1) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_ERR,
                "SD: error occured whilst waiting for command "
                "complete interrupt\n");
#endif
        dev->last_error = irpts & 0xffff0000;
        dev->last_interrupt = irpts;
        return;
    }

    bcm_udelay(2000);

    /* Get response data */
    switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
    case SD_CMD_RSPNS_TYPE_48:
    case SD_CMD_RSPNS_TYPE_48B:
        dev->last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
        break;

    case SD_CMD_RSPNS_TYPE_136:
        dev->last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
        dev->last_r1 = mmio_read(EMMC_BASE + EMMC_RESP1);
        dev->last_r2 = mmio_read(EMMC_BASE + EMMC_RESP2);
        dev->last_r3 = mmio_read(EMMC_BASE + EMMC_RESP3);
        break;
    }

    /* If with data, wait for the appropriate interrupt */
    if ((cmd_reg & SD_CMD_ISDATA) && (is_sdma == 0)) {
        uint32_t wr_irpt;
        int is_write = 0;

        if (cmd_reg & SD_CMD_DAT_DIR_CH) {
            wr_irpt = (1 << 5);     /* read */
        } else {
            is_write = 1;
            wr_irpt = (1 << 4);     /* write */
        }

        int cur_block = 0;
        uint32_t *cur_buf_addr = (uint32_t *)dev->buf;
        while (cur_block < dev->blocks_to_transfer) {
#ifdef EMMC_DEBUG
            if (dev->blocks_to_transfer > 1) {
                char buf[80];

                ksprintf(buf, sizeof(buf),
                        "SD: multi block transfer, awaiting block %i ready\n",
                        cur_block);
                KERROR(KERROR_DEBUG, buf);
            }
#endif
            TIMEOUT_WAIT(
                    mmio_read(EMMC_BASE + EMMC_INTERRUPT) & (wr_irpt | 0x8000),
                    timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0000 | wr_irpt);

            if ((irpts & (0xffff0000 | wr_irpt)) != wr_irpt) {
#ifdef EMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for "
                       "data ready interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;

                return;
            }

            /* Transfer the block */
            size_t cur_byte_no = 0;
            while (cur_byte_no < dev->block_size) {
                if (is_write) {
                    uint32_t data = read_word((uint8_t *)cur_buf_addr, 0);
                    mmio_write(EMMC_BASE + EMMC_DATA, data);
                } else {
                    uint32_t data = mmio_read(EMMC_BASE + EMMC_DATA);
                    write_word(data, (uint8_t *)cur_buf_addr, 0);
                }
                cur_byte_no += 4;
                cur_buf_addr++;
            }

#ifdef EMMC_DEBUG
            {
                char buf[80];
                ksprintf(buf, sizeof(buf), "SD: block %i transfer complete\n",
                        cur_block);
                KERROR(KERROR_DEBUG, buf);
            }
#endif

            cur_block++;
        }
    }

    /* Wait for transfer complete (set if read/write transfer or with busy) */
    if ((((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||
       (cmd_reg & SD_CMD_ISDATA)) && (is_sdma == 0)) {
        /* First check command inhibit (DAT) is not already 0 */
        if ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) == 0) {
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
        } else {
            TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8002,
                         timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);

            /* Handle the case where both data timeout and transfer complete
             * are set - transfer complete overrides data timeout: HCSS 2.2.17
             */
            if (((irpts & 0xffff0002) != 0x2) &&
                    ((irpts & 0xffff0002) != 0x100002)) {
#ifdef EMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for "
                       "transfer complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
        }
    } else if (is_sdma) {
        /* For SDMA transfers, we have to wait for either transfer complete, */
        /*  DMA int or an error */

        /* First check command inhibit (DAT) is not already 0 */
        if ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) == 0) {
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff000a);
        } else {
            TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x800a,
                         timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff000a);

            /* Detect errors */
            if ((irpts & 0x8000) && ((irpts & 0x2) != 0x2)) {
#ifdef EMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for transfer "
                       "complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            /* Detect DMA interrupt without transfer complete */
            /* Currently not supported - all block sizes should fit in the */
            /*  buffer */
            if ((irpts & 0x8) && ((irpts & 0x2) != 0x2)) {
#ifdef EMMC_DEBUG
                KERROR(KERROR_ERR,
                        "SD: error: DMA interrupt occured"
                        "without transfer complete\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;

                return;
            }

            /* Detect transfer complete */
            if (irpts & 0x2) {
#ifdef EMMC_DEBUG
                KERROR(KERROR_DEBUG, "SD: SDMA transfer complete");
#endif
                panic("NO DMA support");
                /* TODO */
                /* Transfer the data to the user buffer */
                /*memcpy(dev->buf, (const void *)SDMA_BUFFER,
                  dev->block_size); */
            } else {
                /* Unknown error */
#ifdef EMMC_DEBUG
                char buf[80];

                if (irpts == 0) {
                    KERROR(KERROR_DEBUG,
                            "SD: timeout waiting for SDMA transfer to complete\n");
                } else {
                    KERROR(KERROR_ERR, "SD: unknown SDMA transfer error\n");
                }

                ksprintf(buf, sizeof(buf), "SD: INTERRUPT: %08x, STATUS %08x\n",
                         irpts, mmio_read(EMMC_BASE + EMMC_STATUS));
                KERROR(KERROR_DEBUG, buf);
#endif

                if ((irpts == 0) &&
                        ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x3) == 0x2)) {
                    /*
                     * The data transfer is ongoing, we should attempt to stop
                     * it.
                     */
#ifdef EMMC_DEBUG
                    KERROR(KERROR_DEBUG, "SD: aborting transfer\n");
#endif
                    mmio_write(EMMC_BASE + EMMC_CMDTM,
                            sd_commands[STOP_TRANSMISSION]);

#ifdef EMMC_DEBUG
                    /* pause to let us read the screen */
                    bcm_udelay(2000000);
#endif
                }
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }
        }
    }

    /* Return success */
    dev->last_cmd_success = 1;
}

static void sd_handle_card_interrupt(struct emmc_block_dev *dev)
{
    /* Handle a card interrupt */

#ifdef EMMC_DEBUG
    uint32_t status = mmio_read(EMMC_BASE + EMMC_STATUS);

    {
        char buf[80];

        KERROR(KERROR_DEBUG, "SD: card interrupt\n");
        ksprintf(buf, sizeof(buf), "SD: controller status: %08x\n", status);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Get the card status */
    if (dev->card_rca) {
        sd_issue_command_int(dev, sd_commands[SEND_STATUS], dev->card_rca << 16,
                         500000);
        if (FAIL(dev)) {
#ifdef EMMC_DEBUG
            KERROR(KERROR_ERR, "SD: unable to get card status\n");
#endif
        } else {
#ifdef EMMC_DEBUG
            char buf[80];

            ksprintf(buf, sizeof(buf), "SD: card status: %08x\n", dev->last_r0);
            KERROR(KERROR_DEBUG, buf);
#endif
        }
    } else {
#ifdef EMMC_DEBUG
        KERROR(KERROR_ERR, "SD: no card currently selected\n");
#endif
    }
}

static void sd_handle_interrupts(struct emmc_block_dev *dev)
{
    uint32_t irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
    uint32_t reset_mask = 0;

    if (irpts & SD_COMMAND_COMPLETE) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious command complete interrupt\n");
#endif
        reset_mask |= SD_COMMAND_COMPLETE;
    }

    if (irpts & SD_TRANSFER_COMPLETE) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious transfer complete interrupt\n");
#endif
        reset_mask |= SD_TRANSFER_COMPLETE;
    }

    if (irpts & SD_BLOCK_GAP_EVENT) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious block gap event interrupt\n");
#endif
        reset_mask |= SD_BLOCK_GAP_EVENT;
    }

    if (irpts & SD_DMA_INTERRUPT) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious DMA interrupt\n");
#endif
        reset_mask |= SD_DMA_INTERRUPT;
    }

    if (irpts & SD_BUFFER_WRITE_READY) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious buffer write ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_WRITE_READY;
        sd_reset_dat();
    }

    if (irpts & SD_BUFFER_READ_READY) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious buffer read ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_READ_READY;
        sd_reset_dat();
    }

    if (irpts & SD_CARD_INSERTION) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card insertion detected\n");
#endif
        reset_mask |= SD_CARD_INSERTION;
    }

    if (irpts & SD_CARD_REMOVAL) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card removal detected\n");
#endif
        reset_mask |= SD_CARD_REMOVAL;
        dev->card_removal = 1;
    }

    if (irpts & SD_CARD_INTERRUPT) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card interrupt detected\n");
#endif
        sd_handle_card_interrupt(dev);
        reset_mask |= SD_CARD_INTERRUPT;
    }

    if (irpts & 0x8000) {
#ifdef EMMC_DEBUG
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: spurious error interrupt: %08x\n",
                irpts);
        KERROR(KERROR_ERR, buf);
#endif
        reset_mask |= 0xffff0000;
    }

    mmio_write(EMMC_BASE + EMMC_INTERRUPT, reset_mask);
}

static void sd_issue_command(struct emmc_block_dev *dev, uint32_t command,
        uint32_t argument, useconds_t timeout)
{
    /* First, handle any pending interrupts */
    sd_handle_interrupts(dev);

    /*
     * Stop the command issue if it was the card remove interrupt that was
     * handled
     */
    if (dev->card_removal) {
        dev->last_cmd_success = 0;
        return;
    }

    /* Now run the appropriate commands by calling sd_issue_command_int() */
    if (command & IS_APP_CMD) {
        command &= 0xff;
#ifdef EMMC_DEBUG
        {
            char buf[80];
            ksprintf(buf, sizeof(buf), "SD: issuing command ACMD%i\n", command);
            KERROR(KERROR_DEBUG, buf);
        }
#endif

        if (sd_acommands[command] == SD_CMD_RESERVED(0)) {
            char buf[80];

            ksprintf(buf, sizeof(buf), "SD: invalid command ACMD%i\n", command);
            KERROR(KERROR_ERR, buf);

            dev->last_cmd_success = 0;
            return;
        }
        dev->last_cmd = APP_CMD;

        uint32_t rca = 0;
        if (dev->card_rca)
            rca = dev->card_rca << 16;
        sd_issue_command_int(dev, sd_commands[APP_CMD], rca, timeout);
        if (dev->last_cmd_success) {
            dev->last_cmd = command | IS_APP_CMD;
            sd_issue_command_int(dev, sd_acommands[command], argument, timeout);
        }
    } else {
#ifdef EMMC_DEBUG
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: issuing command CMD%i\n", command);
        KERROR(KERROR_DEBUG, buf);
#endif

        if (sd_commands[command] == SD_CMD_RESERVED(0)) {
            char buf[80];

            ksprintf(buf, sizeof(buf), "SD: invalid command CMD%i\n", command);
            KERROR(KERROR_ERR, buf);

            dev->last_cmd_success = 0;
            return;
        }

        dev->last_cmd = command;
        sd_issue_command_int(dev, sd_commands[command], argument, timeout);
    }

#ifdef EMMC_DEBUG
    if (FAIL(dev)) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                 "SD: error issuing command: interrupts %08x: ",
                 dev->last_interrupt);
        KERROR(KERROR_DEBUG, buf);

        if (dev->last_error == 0) {
            KERROR(KERROR_DEBUG, "TIMEOUT");
        }
#if 0
        else {
            for (int i = 0; i < SD_ERR_RSVD; i++) {
                if (dev->last_error & (1 << (i + 16))) {
                    printf(err_irpts[i]);
                    printf(" ");
                }
            }
        }
        printf("\n");
#endif
    } else {
        KERROR(KERROR_DEBUG, "SD: command completed successfully\n");
    }
#endif
}

int rpi_emmc_card_init(struct block_dev **dev)
{
#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
    /* Power cycle the card to ensure its in its startup state */
    if (bcm_2708_power_cycle() != 0) {
        KERROR(KERROR_ERR,
                "EMMC: BCM2708 controller did not power cycle successfully\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: BCM2708 controller power-cycled\n");
#endif
#endif

    /* Read the controller version */
    uint32_t ver = mmio_read(EMMC_BASE + EMMC_SLOTISR_VER);
    uint32_t vendor = ver >> 24;
    uint32_t sdversion = (ver >> 16) & 0xff;
    uint32_t slot_status = ver & 0xff;
    {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "EMMC: vendor %x, sdversion %x, slot_status %x\n",
                vendor, sdversion, slot_status);
        KERROR(KERROR_INFO, buf);
    }
    hci_ver = sdversion;

    if (hci_ver < 2) {
        KERROR(KERROR_ERR, "EMMC: only SDHCI versions >= 3.0 are supported\n");

        return -1;
    }

    /* Reset the controller */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: resetting controller\n");
#endif
    uint32_t control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= (1 << 24);
    /* Disable clock */
    control1 &= ~(1 << 2);
    control1 &= ~(1 << 0);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    TIMEOUT_WAIT((mmio_read(EMMC_BASE + EMMC_CONTROL1) & (0x7 << 24)) == 0,
                 1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & (0x7 << 24)) != 0) {
        KERROR(KERROR_ERR, "EMMC: controller did not reset properly\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "EMMC: control0: %08x, control1: %08x, control2: %08x\n",
                mmio_read(EMMC_BASE + EMMC_CONTROL0),
                mmio_read(EMMC_BASE + EMMC_CONTROL1),
                mmio_read(EMMC_BASE + EMMC_CONTROL2));
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Read the capabilities registers */
    capabilities_0 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_0);
    capabilities_1 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_1);
#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "EMMC: capabilities: %x, %x\n",
                capabilities_1, capabilities_0);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Check for a valid card */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: checking for an inserted card\n");
#endif
    TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_STATUS) & (1 << 16), 500000);
    uint32_t status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
    if ((status_reg & (1 << 16)) == 0) {
        KERROR(KERROR_ERR, "EMMC: no card inserted\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "EMMC: status: %08x\n", status_reg);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Clear control2 */
    mmio_write(EMMC_BASE + EMMC_CONTROL2, 0);

    /* Get the base clock rate */
    uint32_t base_clock = sd_get_base_clock_hz();
    if (base_clock == 0) {
        KERROR(KERROR_INFO, "EMMC: assuming clock rate to be 100MHz\n");
        base_clock = 100000000;
    }

    /* Set clock rate to something slow */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: setting clock rate\n");
#endif
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= 1;            /* enable clock */

    /* Set to identification frequency (400 kHz) */
    uint32_t f_id = sd_get_clock_divider(base_clock, SD_CLOCK_ID);
    if (f_id == SD_GET_CLOCK_DIVIDER_FAIL) {
        KERROR(KERROR_ERR,
                "EMMC: unable to get a valid clock divider for ID frequency\n");

        return -1;
    }
    control1 |= f_id;

    control1 |= (7 << 16);        /* data timeout = TMCLK * 2^10 */
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2, 0x1000000);
    if ((mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2) == 0) {
        KERROR(KERROR_ERR,
                "EMMC: controller's clock did not stabilise within 1 second\n");
        return -1;
    }
#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "EMMC: control0: %08x, control1: %08x\n",
                mmio_read(EMMC_BASE + EMMC_CONTROL0),
                mmio_read(EMMC_BASE + EMMC_CONTROL1));
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Enable the SD clock */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: enabling SD clock\n");
#endif
    bcm_udelay(2000);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    control1 |= 4;
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    bcm_udelay(2000);
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: SD clock enabled\n");
#endif

    /* Mask off sending interrupts to the ARM */
    mmio_write(EMMC_BASE + EMMC_IRPT_EN, 0);
    /* Reset interrupts */
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);
    /* Have all interrupts sent to the INTERRUPT register */
    uint32_t irpt_mask = 0xffffffff & (~SD_CARD_INTERRUPT);
#ifdef SD_CARD_INTERRUPTS
    irpt_mask |= SD_CARD_INTERRUPT;
#endif
    mmio_write(EMMC_BASE + EMMC_IRPT_MASK, irpt_mask);

#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: interrupts disabled\n");
#endif
    bcm_udelay(2000);

    /* Prepare the device structure */
    struct emmc_block_dev *ret;
    if (*dev == NULL)
        ret = kmalloc(sizeof(struct emmc_block_dev));
    else
        ret = (struct emmc_block_dev *)*dev;

    memset(ret, 0, sizeof(struct emmc_block_dev));
    ret->bd.drv_name = driver_name;
    ret->bd.dev_name = device_name;
    ret->bd.block_size = 512;
    ret->bd.read = sd_read;
#ifdef SD_WRITE_SUPPORT
    ret->bd.write = sd_write;
#endif
    ret->bd.supports_multiple_block_read = 1;
    ret->bd.supports_multiple_block_write = 1;
    ret->base_clock = base_clock;

#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: device structure created\n");
#endif

    /* Send CMD0 to the card (reset to idle state) */
    sd_issue_command(ret, GO_IDLE_STATE, 0, 500000);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: no CMD0 response\n");

        return -1;
    }

    /* Send CMD8 to the card */
    /* Voltage supplied = 0x1 = 2.7-3.6V (standard) */
    /* Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG,
            "SD: note a timeout error on the following command (CMD8) is normal "
            "and expected if the SD card version is less than 2.0\n");
#endif
    sd_issue_command(ret, SEND_IF_COND, 0x1aa, 500000);
    int v2_later = 0;

    if (TIMEOUT(ret)) {
        v2_later = 0;
    } else if (CMD_TIMEOUT(ret)) {
        if (sd_reset_cmd() == -1)
            return -1;
        mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        v2_later = 0;
    } else if (FAIL(ret)) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: failure sending CMD8 (%08x)\n",
                ret->last_interrupt);
        KERROR(KERROR_ERR, buf);

        return -1;
    } else {
        if ((ret->last_r0 & 0xfff) != 0x1aa) {
            KERROR(KERROR_ERR, "SD: unusable card\n");
#ifdef EMMC_DEBUG
            char buf[80];

            ksprintf(buf, sizeof(buf), "SD: CMD8 response %08x\n",
                    ret->last_r0);
            KERROR(KERROR_DEBUG, buf);
#endif
            return -1;
        } else {
            v2_later = 1;
        }
    }

    /* Here we are supposed to check the response to CMD5 (HCSS 3.6) */
    /* It only returns if the card is a SDIO card */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG,
            "SD: note that a timeout error on the following command (CMD5) is "
            "normal and expected if the card is not a SDIO card.\n");
#endif
    sd_issue_command(ret, IO_SET_OP_COND, 0, 10000);
    if (!TIMEOUT(ret)) {
        if (CMD_TIMEOUT(ret)) {
            if (sd_reset_cmd() == -1) {
                return -1;
            }
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        } else {
            KERROR(KERROR_ERR,
                    "SD: SDIO card detected - not currently supported\n");
#ifdef EMMC_DEBUG
            char buf[80];
            ksprintf(buf, sizeof(buf), "SD: CMD5 returned %08x\n",
                     ret->last_r0);
            KERROR(KERROR_DEBUG, buf);
#endif
            return -1;
        }
    }

    /* Call an inquiry ACMD41 (voltage window = 0) to get the OCR */
#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: sending inquiry ACMD41\n");
#endif
    sd_issue_command(ret, ACMD(41), 0, 500000);

    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: inquiry ACMD41 failed\n");

        return -1;
    }
#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: inquiry ACMD41 returned %08x\n",
                ret->last_r0);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Call initialization ACMD41 */
    int card_is_busy = 1;
    while (card_is_busy) {
        uint32_t v2_flags = 0;
        if (v2_later) {
            /* Set SDHC support */
            v2_flags |= (1 << 30);

            /* Set 1.8v support */
#ifdef SD_1_8V_SUPPORT
            if (!ret->failed_voltage_switch)
                v2_flags |= (1 << 24);
#endif

            /* Enable SDXC maximum performance */
#ifdef SDXC_MAXIMUM_PERFORMANCE
            v2_flags |= (1 << 28);
#endif
        }

        sd_issue_command(ret, ACMD(41), 0x00ff8000 | v2_flags, 500000);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: error issuing ACMD41\n");

            return -1;
        }

        if ((ret->last_r0 >> 31) & 0x1) {
            /* Initialization is complete */
            ret->card_ocr = (ret->last_r0 >> 8) & 0xffff;
            ret->card_supports_sdhc = (ret->last_r0 >> 30) & 0x1;

#ifdef SD_1_8V_SUPPORT
            if (!ret->failed_voltage_switch)
                ret->card_supports_18v = (ret->last_r0 >> 24) & 0x1;
#endif

            card_is_busy = 0;
        } else {
            /* Card is still busy */
#ifdef EMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: card is busy, retrying\n");
#endif
            bcm_udelay(500000);
        }
    }

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "SD: card identified: OCR: %04x, "
                "1.8v support: %i, SDHC support: %i\n",
                ret->card_ocr, ret->card_supports_18v, ret->card_supports_sdhc);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* At this point, we know the card is definitely an SD card,
     * so will definitely  support SDR12 mode which runs at 25 MHz */
    sd_switch_clock_rate(base_clock, SD_CLOCK_NORMAL);

    /* A small wait before the voltage switch */
    bcm_udelay(5000);

    /* Switch to 1.8V mode if possible */
    if (ret->card_supports_18v) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_ERR, "SD: switching to 1.8V mode\n");
#endif
        /* As per HCSS 3.6.1 */

        /* Send VOLTAGE_SWITCH */
        sd_issue_command(ret, VOLTAGE_SWITCH, 0, 500000);
        if (FAIL(ret)) {
#ifdef EMMC_DEBUG
            KERROR(KERROR_ERR, "SD: error issuing VOLTAGE_SWITCH\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return rpi_emmc_card_init((struct block_dev **)&ret);
        }

        /* Disable SD clock */
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        control1 &= ~(1 << 2);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);

        /* Check DAT[3:0] */
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        uint32_t dat30 = (status_reg >> 20) & 0xf;
        if (dat30 != 0) {
#ifdef EMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: DAT[3:0] did not settle to 0\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return rpi_emmc_card_init((struct block_dev **)&ret);
        }

        /* Set 1.8V signal enable to 1 */
        uint32_t control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        control0 |= (1 << 8);
        mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);

        /* Wait 5 ms */
        bcm_udelay(5000);

        /* Check the 1.8V signal enable is set */
        control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        if (((control0 >> 8) & 0x1) == 0) {
#ifdef EMMC_DEBUG
            KERROR(KERROR_DEBUG,
                    "SD: controller did not keep 1.8V signal enable high\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return rpi_emmc_card_init((struct block_dev **)&ret);
        }

        /* Re-enable the SD clock */
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        control1 |= (1 << 2);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);

        /* Wait 1 ms */
        bcm_udelay(10000);

        /* Check DAT[3:0] */
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        dat30 = (status_reg >> 20) & 0xf;
        if (dat30 != 0xf) {
#ifdef EMMC_DEBUG
            {
                char buf[80];

                ksprintf(buf, sizeof(buf),
                        "SD: DAT[3:0] did not settle to 1111b (%01x)\n", dat30);
                KERROR(KERROR_DEBUG, buf);
            }
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return rpi_emmc_card_init((struct block_dev **)&ret);
        }

#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: voltage switch complete\n");
#endif
    }

    /* Send CMD2 to get the cards CID */
    sd_issue_command(ret, ALL_SEND_CID, 0, 500000);
    if (FAIL(ret)) {
        KERROR(KERROR_DEBUG, "SD: error sending ALL_SEND_CID\n");
        return -1;
    }
    uint32_t card_cid_0 = ret->last_r0;
    uint32_t card_cid_1 = ret->last_r1;
    uint32_t card_cid_2 = ret->last_r2;
    uint32_t card_cid_3 = ret->last_r3;

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "SD: card CID: %08x%08x%08x%08x\n",
                card_cid_3, card_cid_2, card_cid_1, card_cid_0);
        KERROR(KERROR_DEBUG, buf);
    }
#endif
    uint32_t *dev_id = kmalloc(4 * sizeof(uint32_t));
    dev_id[0] = card_cid_0;
    dev_id[1] = card_cid_1;
    dev_id[2] = card_cid_2;
    dev_id[3] = card_cid_3;
    ret->cid = (uint8_t *)dev_id;
    ret->cid_len = 4 * sizeof(uint32_t);

    /* Send CMD3 to enter the data state */
    sd_issue_command(ret, SEND_RELATIVE_ADDR, 0, 500000);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending SEND_RELATIVE_ADDR\n");

        kfree(ret);
        return -1;
    }

    uint32_t cmd3_resp = ret->last_r0;
#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "SD: CMD3 response: %08x\n", cmd3_resp);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    ret->card_rca = (cmd3_resp >> 16) & 0xffff;
    uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
    uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
    uint32_t error = (cmd3_resp >> 13) & 0x1;
    uint32_t status = (cmd3_resp >> 9) & 0xf;
    uint32_t ready = (cmd3_resp >> 8) & 0x1;

    if (crc_error) {
        KERROR(KERROR_ERR, "SD: CRC error\n");

        kfree(ret);
        kfree(dev_id);

        return -1;
    }

    if (illegal_cmd) {
        KERROR(KERROR_ERR, "SD: illegal command\n");

        kfree(ret);
        kfree(dev_id);

        return -1;
    }

    if (error) {
        KERROR(KERROR_ERR, "SD: generic error\n");

        kfree(ret);
        kfree(dev_id);

        return -1;
    }

    if (!ready) {
        KERROR(KERROR_ERR, "SD: not ready for data\n");

        kfree(ret);
        kfree(dev_id);

        return -1;
    }

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "SD: RCA: %04x\n", ret->card_rca);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    /* Now select the card (toggles it to transfer state) */
    sd_issue_command(ret, SELECT_CARD, ret->card_rca << 16, 500000);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending CMD7\n");

        kfree(ret);

        return -1;
    }

    uint32_t cmd7_resp = ret->last_r0;
    status = (cmd7_resp >> 9) & 0xf;

    if ((status != 3) && (status != 4)) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: invalid status (%i)\n", status);
        KERROR(KERROR_ERR, buf);

        kfree(ret);
        kfree(dev_id);

        return -1;
    }

    /* If not an SDHC card, ensure BLOCKLEN is 512 bytes */
    if (!ret->card_supports_sdhc) {
        sd_issue_command(ret, SET_BLOCKLEN, 512, 500000);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: error sending SET_BLOCKLEN\n");

            kfree(ret);

            return -1;
        }
    }
    ret->block_size = 512;
    uint32_t controller_block_size = mmio_read(EMMC_BASE + EMMC_BLKSIZECNT);
    controller_block_size &= (~0xfff);
    controller_block_size |= 0x200;
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, controller_block_size);

    /* Get the cards SCR register */
    ret->scr = kmalloc(sizeof(struct sd_scr));
    ret->buf = &ret->scr->scr[0];
    ret->block_size = 8;
    ret->blocks_to_transfer = 1;
    sd_issue_command(ret, SEND_SCR, 0, 500000);
    ret->block_size = 512;
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending SEND_SCR\n");

        kfree(ret->scr);
        kfree(ret);

        return -1;
    }

    /* Determine card version */
    /* Note that the SCR is big-endian */
    uint32_t scr0 = __builtin_bswap32(ret->scr->scr[0]);
    ret->scr->sd_version = SD_VER_UNKNOWN;
    uint32_t sd_spec = (scr0 >> (56 - 32)) & 0xf;
    uint32_t sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
    uint32_t sd_spec4 = (scr0 >> (42 - 32)) & 0x1;
    ret->scr->sd_bus_widths = (scr0 >> (48 - 32)) & 0xf;
    if (sd_spec == 0) {
        ret->scr->sd_version = SD_VER_1;
    } else if (sd_spec == 1) {
        ret->scr->sd_version = SD_VER_1_1;
    } else if (sd_spec == 2) {
        if (sd_spec3 == 0) {
            ret->scr->sd_version = SD_VER_2;
        } else if (sd_spec3 == 1) {
            if (sd_spec4 == 0) {
                ret->scr->sd_version = SD_VER_3;
            } else if (sd_spec4 == 1) {
                ret->scr->sd_version = SD_VER_4;
            }
        }
    }

#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: &scr: %p\n", &ret->scr->scr[0]);
        KERROR(KERROR_DEBUG, buf);

        ksprintf(buf, sizeof(buf), "SD: SCR[0]: %08x, SCR[1]: %08x\n",
                ret->scr->scr[0], ret->scr->scr[1]);
        KERROR(KERROR_DEBUG, buf);

        ksprintf(buf, sizeof(buf), "SD: SCR: %08x%08x\n",
                __builtin_bswap32(ret->scr->scr[0]),
                __builtin_bswap32(ret->scr->scr[1]));
        KERROR(KERROR_DEBUG, buf);

        ksprintf(buf, sizeof(buf), "SD: SCR: version %s, bus_widths %01x\n",
                sd_versions[ret->scr->sd_version], ret->scr->sd_bus_widths);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    if (ret->scr->sd_bus_widths & 0x4) {
        /* Set 4-bit transfer mode (ACMD6) */
        /* See HCSS 3.4 for the algorithm */
#ifdef SD_4BIT_DATA
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: switching to 4-bit data mode\n");
#endif

        /* Disable card interrupt in host */
        uint32_t old_irpt_mask = mmio_read(EMMC_BASE + EMMC_IRPT_MASK);
        uint32_t new_iprt_mask = old_irpt_mask & ~(1 << 8);
        mmio_write(EMMC_BASE + EMMC_IRPT_MASK, new_iprt_mask);

        /* Send ACMD6 to change the card's bit mode */
        sd_issue_command(ret, SET_BUS_WIDTH, 0x2, 500000);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: switch to 4-bit data mode failed\n");
        } else {
            /* Change bit mode for Host */
            uint32_t control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
            control0 |= 0x2;
            mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);

            /* Re-enable card interrupt in host */
            mmio_write(EMMC_BASE + EMMC_IRPT_MASK, old_irpt_mask);

#ifdef EMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: switch to 4-bit complete\n");
#endif
        }
#endif
    }

    {
        char buf[80];

        ksprintf(buf, sizeof(buf), "SD: found a valid version %s SD card\n",
                sd_versions[ret->scr->sd_version]);
        KERROR(KERROR_INFO, buf);

#ifdef EMMC_DEBUG
        ksprintf(buf, sizeof(buf), "SD: setup successful (status %i)\n",
                status);
        KERROR(KERROR_DEBUG, buf);
#endif
    }

    /* Reset interrupt register */
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);

    *dev = (struct block_dev *)ret;

    return 0;
}

static int sd_ensure_data_mode(struct emmc_block_dev *edev)
{
    if (edev->card_rca == 0) {
        /* Try again to initialise the card */
        int ret = rpi_emmc_card_init((struct block_dev **)&edev);
        if (ret != 0)
            return ret;
    }

#ifdef EMMC_DEBUG
    {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "SD: ensure_data_mode() obtaining status register for "
                "card_rca %08x: ",
                edev->card_rca);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16, 500000);
    if (FAIL(edev)) {
        KERROR(KERROR_ERR, "SD: ensure_data_mode() error sending CMD13\n");

        edev->card_rca = 0;

        return -1;
    }

    uint32_t status = edev->last_r0;
    uint32_t cur_state = (status >> 9) & 0xf;

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "status %i\n", cur_state);
        KERROR(KERROR_DEBUG, buf);
    }
#endif
    if (cur_state == 3) {
        /* Currently in the stand-by state - select it */
        sd_issue_command(edev, SELECT_CARD, edev->card_rca << 16, 500000);
        if (FAIL(edev)) {
            KERROR(KERROR_ERR,
                    "SD: ensure_data_mode() no response from CMD17\n");

            edev->card_rca = 0;

            return -1;
        }
    } else if (cur_state == 5) {
        /* In the data transfer state - cancel the transmission */
        sd_issue_command(edev, STOP_TRANSMISSION, 0, 500000);
        if (FAIL(edev)) {

            KERROR(KERROR_ERR,
                    "SD: ensure_data_mode() no response from CMD12\n");
            edev->card_rca = 0;
            return -1;
        }

        /* Reset the data circuit */
        sd_reset_dat();
    } else if (cur_state != 4) {
        /* Not in the transfer state - re-initialise */
        int ret = rpi_emmc_card_init((struct block_dev **)&edev);
        if (ret != 0)
            return ret;
    }

    /* Check again that we're now in the correct mode */
    if (cur_state != 4) {
#ifdef EMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: ensure_data_mode() rechecking status: ");
#endif
        sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16, 500000);
        if (FAIL(edev)) {
            KERROR(KERROR_ERR,
                    "SD: ensure_data_mode() no response from CMD13\n");
            edev->card_rca = 0;
            return -1;
        }
        status = edev->last_r0;
        cur_state = (status >> 9) & 0xf;

#ifdef EMMC_DEBUG
        {
            char buf[80];
            ksprintf(buf, sizeof(buf), "cur_state: %i\n", cur_state);
            KERROR(KERROR_DEBUG, buf);
        }
#endif

        if (cur_state != 4) {
            char buf[80];

            ksprintf(buf, sizeof(buf),
                    "SD: unable to initialise SD card to data mode (state %i)\n",
                    cur_state);
            KERROR(KERROR_ERR, buf);

            edev->card_rca = 0;
            return -1;
        }
    }

    return 0;
}

#ifdef SDMA_SUPPORT
/* We only support DMA transfers to buffers aligned on a 4 kiB boundary */
static int sd_suitable_for_dma(void *buf)
{
    if ((uintptr_t)buf & 0xfff)
        return 0;
    else
        return 1;
}
#endif

static int sd_do_data_command(struct emmc_block_dev *edev, int is_write,
        uint8_t *buf, size_t buf_size, uint32_t block_no)
{
    /* PLSS table 4.20 - SDSC cards use byte addresses rather than
     * block addresses */
    if (!edev->card_supports_sdhc)
        block_no *= 512;

    /* This is as per HCSS 3.7.2.1 */
    if (buf_size < edev->block_size) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "SD: do_data_command() called with buffer size (%i) less than "
                "block size (%i)\n", buf_size, edev->block_size);
        KERROR(KERROR_ERR, buf);

        return -1;
    }

    edev->blocks_to_transfer = buf_size / edev->block_size;
    if (buf_size % edev->block_size) {
        char buf[80];

        ksprintf(buf, sizeof(buf),
                "SD: do_data_command() called with buffer size (%i) not an "
                "exact multiple of block size (%i)\n",
                buf_size, edev->block_size);
        KERROR(KERROR_ERR, buf);

        return -1;
    }
    edev->buf = buf;

    /* Decide on the command to use */
    int command;
    if (is_write) {
        if (edev->blocks_to_transfer > 1)
            command = WRITE_MULTIPLE_BLOCK;
        else
            command = WRITE_BLOCK;
    } else {
        if (edev->blocks_to_transfer > 1)
            command = READ_MULTIPLE_BLOCK;
        else
            command = READ_SINGLE_BLOCK;
    }

    int retry_count = 0;
    int max_retries = 3;
    while (retry_count < max_retries) {
#ifdef SDMA_SUPPORT
        /* use SDMA for the first try only */
        if ((retry_count == 0) && sd_suitable_for_dma(buf)) {
            edev->use_sdma = 1;
        } else {
#ifdef EMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: retrying without SDMA\n");
#endif
            edev->use_sdma = 0;
        }
#else
        edev->use_sdma = 0;
#endif

        sd_issue_command(edev, command, block_no, 5000000);

        if (SUCCESS(edev)) {
            break;
        } else {
            char buf[80];

            ksprintf(buf, sizeof(buf), "SD: error sending CMD%i, ", command);
            KERROR(KERROR_ERR, buf);

            ksprintf(buf, sizeof(buf), "error = %08x.  ", edev->last_error);
            KERROR(KERROR_ERR, buf);

            retry_count++;
            if (retry_count < max_retries) {
                KERROR(KERROR_INFO, "Retrying...\n");
            } else {
                KERROR(KERROR_INFO, "Giving up.\n");
            }
        }
    }
    if (retry_count == max_retries) {
        edev->card_rca = 0;
        return -1;
    }

    return 0;
}

int sd_read(struct block_dev *dev, off_t block_no,
        uint8_t *buf, size_t buf_size)
{
    /* Check the status of the card */
    struct emmc_block_dev *edev = (struct emmc_block_dev *)dev;
    if (sd_ensure_data_mode(edev) != 0)
        return -1;

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "SD: read() card ready, reading from block %u\n", block_no);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    if (sd_do_data_command(edev, 0, buf, buf_size, block_no) < 0)
        return -1;

#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: data read successful\n");
#endif

    return buf_size;
}

#ifdef SD_WRITE_SUPPORT
int sd_write(struct block_dev * dev, off_t block_no, uint8_t *buf,
        size_t buf_size)
{
    /* Check the status of the card */
    struct emmc_block_dev *edev = (struct emmc_block_dev *)dev;
    if (sd_ensure_data_mode(edev) != 0)
        return -1;

#ifdef EMMC_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf),
                "SD: write() card ready, reading from block %u\n", block_no);
        KERROR(KERROR_DEBUG, buf);
    }
#endif

    if (sd_do_data_command(edev, 1, buf, buf_size, block_no) < 0)
        return -1;

#ifdef EMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: write read successful\n");
#endif

    return buf_size;
}
#endif

