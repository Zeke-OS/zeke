/**
 *******************************************************************************
 * @file    emmc.c
 * @author  Olli Vanhoja
 * @brief   emmc driver.
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

/*
 * Provides an interface to the EMMC controller and commands for interacting
 * with an sd card
 *
 * References:
 *
 * PLSS     - SD Group Physical Layer Simplified Specification ver 3.00
 * HCSS     - SD Group Host Controller Simplified Specification ver 3.00
 *
 * Broadcom BCM2835 Peripherals Guide
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/dev_major.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fs/mbr.h>
#include <hal/hw_timers.h>
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <vm/vm.h>
#ifdef configBCM2835
#include "../bcm2835/bcm2835_mmio.h"
#endif
#include "emmc.h"


/* TODO DMA support */
/* SDMA buffer address */
#if 0
#define SDMA_BUFFER     0x6000
#define SDMA_BUFFER_PA  (SDMA_BUFFER + 0xC0000000)
#endif

static const char driver_name[] = "emmc";
/** We use a single device name as there is only one card slot in the RPi */
static const char device_name[] = "emmc0";
static struct emmc_capabilities emmccap;

#define SD_VER_UNKNOWN      0
#define SD_VER_1            1
#define SD_VER_1_1          2
#define SD_VER_2            3
#define SD_VER_3            4
#define SD_VER_4            5

static const char * sd_versions[] = {
    "unknown", "1.0 and 1.01", "1.10", "2.00", "3.0x", "4.xx"
};

#ifdef configEMMC_DEBUG
static const char * err_irpts[] = {
    "CMD_TIMEOUT", "CMD_CRC", "CMD_END_BIT", "CMD_INDEX",
    "DATA_TIMEOUT", "DATA_CRC", "DATA_END_BIT", "CURRENT_LIMIT",
    "AUTO_CMD12", "ADMA", "TUNING", "RSVD"
};
#endif

#define DEFAULT_CMD_TIMEOUT 500000

static mtx_t emmc_lock = MTX_INITIALIZER(MTX_TYPE_SPIN, MTX_OPT_DINT);

static ssize_t sd_read(struct dev_info * dev, off_t offset, uint8_t * buf,
                       size_t count, int oflags);
static ssize_t sd_write(struct dev_info * dev, off_t offset, uint8_t * buf,
                        size_t count, int oflags);
static off_t sd_lseek(file_t * file, struct dev_info * devnfo, off_t offset,
                      int whence);
static int sd_ioctl(struct dev_info * devnfo, uint32_t request,
                    void * arg, size_t arg_len);


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
#define ACMD(a)                 ((a) | IS_APP_CMD)
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

static int emmc_card_init(struct emmc_block_dev ** edev);

int __kinit__ emmc_init(void)
{
#ifdef configBCM2835
    SUBSYS_DEP(bcm2835_prop_init);
#endif
    SUBSYS_INIT("emmc");

    vnode_t * vnode;
#ifdef configMBR
    int fd;
#endif
    int err;

    struct emmc_block_dev * sd_edev = NULL;
    err = emmc_card_init(&sd_edev);
    if (err)
        return err;

    /* TODO Block cache not implemented */
#ifdef ENABLE_BLOCK_CACHE
    struct dev_info * c_dev = sd_edev->dev;
    uintptr_t cache_start = alloc_buf(BLOCK_CACHE_SIZE);

    if (cache_start != 0)
        cache_init(&sd_edev->dev, &c_dev, cache_start, BLOCK_CACHE_SIZE);
#endif

    /* Register with devfs */
    if (make_dev(&sd_edev->dev, 0, 0, 0666, &vnode)) {
        KERROR(KERROR_ERR, "Failed to register a new emmc dev\n");
    }

#ifdef configMBR
    fd = fs_fildes_create_curproc(vnode, O_RDONLY);
    if (fd < 0) {
        KERROR(KERROR_ERR, "Failed to open the device\n");

        return -ENOENT;
    }

    mbr_register(fd, NULL);
#endif

    return 0;
}

static void sd_power_off()
{
    istate_t s_entry;
    uint32_t control0;

    /* Power off the SD card */
    mmio_start(&s_entry);
    control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
    mmio_end(&s_entry);

    /* Set SD Bus Power bit off in Power Control Register */
    control0 &= ~(1 << 8);
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
    mmio_end(&s_entry);
}

/**
 * Set the clock dividers to generate a target value.
 */
static uint32_t sd_get_clock_divider(uint32_t base_clock, uint32_t target_rate)
{
    uint32_t targetted_divisor = 0;
    /* TODO: implement use of preset value registers */

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

    if (emmccap.hci_ver >= 2) {
        /*
         * HCI version 3 or greater supports 10-bit divided clock mode
         * This requires a power-of-two divider
         */

        /* Find the first bit set */
        int divisor = -1;
        for (int first_bit = 31; first_bit >= 0; first_bit--) {
            uint32_t bit_test = (1u << first_bit);
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

#ifdef configEMMC_DEBUG
        int denominator = 1;
        if (divisor != 0)
            denominator = divisor * 2;
        int actual_clock = base_clock / denominator;

        KERROR(KERROR_DEBUG,
               "EMMC: base_clock: %u, target_rate: %i, divisor: %x, "
               "actual_clock: %i, ret: %x\n", base_clock, (int)target_rate,
               (int)divisor, (int)actual_clock, ret);
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
    uint32_t divider, control1;
    istate_t s_entry;
    uint32_t ret;

    /* Decide on an appropriate divider */
    divider = sd_get_clock_divider(base_clock, target_rate);
    if (divider == SD_GET_CLOCK_DIVIDER_FAIL) {
        KERROR(KERROR_DEBUG,
               "EMMC: couldn't get a valid divider for target rate %u Hz\n",
               target_rate);

        return -EIO;
    }

    /* Wait for the command inhibit (CMD and DAT) bits to clear */
    do {
        mmio_start(&s_entry);
        ret = mmio_read(EMMC_BASE + EMMC_STATUS);
        mmio_end(&s_entry);
        udelay(1000);
    } while (ret & 0x3);

    /* Set the SD clock off */
    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);
    control1 &= ~(1 << 2);

    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);
    udelay(2000);

    /* Write the new divider */
    control1 &= ~0xffe0;        /* Clear old setting + clock generator select */
    control1 |= divider;
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);
    udelay(2000);

    /* Enable the SD clock */
    control1 |= (1 << 2);
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);
    udelay(2000);

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG,
           "EMMC: successfully set clock rate to %u Hz\n",
           target_rate);
#endif
    return 0;
}

/**
 * Reset the CMD line.
 */
static int sd_reset_cmd()
{
    uint32_t control1;
    istate_t s_entry;

    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);

    control1 |= SD_RESET_CMD;
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);

    mmio_start(&s_entry);
    TIMEOUT_WAIT((mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_CMD) == 0,
                 1000000);
    mmio_end(&s_entry);
    /* TODO That won't work. */
#if 0
    if (mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_CMD) {
        KERROR(KERROR_ERR, "EMMC: CMD line did not reset properly\n");

        return -EIO;
    }
#endif

    return 0;
}

/* Reset the CMD line */
static int sd_reset_dat()
{
    uint32_t control1;
    uint32_t d;
    istate_t s_entry;

    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);

    control1 |= SD_RESET_DAT;
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);

    mmio_start(&s_entry);
    TIMEOUT_WAIT(
            (d = (mmio_read(EMMC_BASE + EMMC_CONTROL1) & SD_RESET_DAT)) == 0,
            1000000);
    mmio_end(&s_entry);
    if (d != 0) {
        KERROR(KERROR_ERR, "EMMC: DAT line did not reset properly\n");

        return -EIO;
    }

    return 0;
}

static void sd_issue_command_int(struct emmc_block_dev *dev, uint32_t cmd_reg,
                                 uint32_t argument, useconds_t timeout)
{
    int is_sdma = 0;
    uint32_t blksizecnt, irpts;
    istate_t s_entry;

    dev->last_cmd_reg = cmd_reg;
    dev->last_cmd_success = 0;

    /* This is as per HCSS 3.7.1.1/3.7.2.2 */

    /* Check Command Inhibit */
    mmio_start(&s_entry);
    while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x1) {
        mmio_end(&s_entry);
        udelay(SD_CMD_UDELAY);
        mmio_start(&s_entry);
    }
    mmio_end(&s_entry);

    /* Is the command with busy and not and abort command? */
    if (((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) &&
        ((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT)) {

        /* Wait for the data line to be free */
        mmio_start(&s_entry);
        while (mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) {
            mmio_end(&s_entry);
            udelay(SD_CMD_UDELAY);
            mmio_start(&s_entry);
        }
        mmio_end(&s_entry);
    }

    /* Is this a DMA transfer? */
    if ((cmd_reg & SD_CMD_ISDATA) && (dev->use_sdma)) {
#ifdef configEMMC_DEBUG
        uint32_t d;

        mmio_start(&s_entry);
        d = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
        mmio_end(&s_entry);

        KERROR(KERROR_DEBUG,
               "SD: performing SDMA transfer, current INTERRUPT: %x\n", d);
#endif

        is_sdma = 1;
    }

    if (is_sdma) {
        /* Set system address register (ARGUMENT2 in RPi) */

        /*
         * We need to define a 4 kiB aligned buffer to use here
         * Then convert its virtual address to a bus address
         */
        /* TODO set dma buffer */
        panic("no dma support");
        /*mmio_write(EMMC_BASE + EMMC_ARG2, SDMA_BUFFER_PA); */
    }

    /*
     * Set block size and block count
     * For now, block size = 512 bytes, block count = 1,
     * host SDMA buffer boundary = 4 kiB
     */
    if (dev->blocks_to_transfer > 0xffff) {
        KERROR(KERROR_ERR, "SD: blocks_to_transfer too great (%i)\n",
               (int)dev->blocks_to_transfer);

        dev->last_cmd_success = 0;
        return;
    }
    blksizecnt = dev->block_size | (dev->blocks_to_transfer << 16);
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, blksizecnt);

    /* Set argument 1 reg */
    mmio_write(EMMC_BASE + EMMC_ARG1, argument);
    mmio_end(&s_entry);

    if (is_sdma) {
        /* Set Transfer mode register */
        cmd_reg |= SD_CMD_DMA;
    }

    /* Set command reg */
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CMDTM, cmd_reg);
    mmio_end(&s_entry);
    udelay(2 * SD_CMD_UDELAY);

    /* Wait for command complete interrupt */
    mmio_start(&s_entry);
    TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8001, timeout);
    irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
    /* Clear command complete status */
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0001);
    mmio_end(&s_entry);

    /* Test for errors */
    if ((irpts & 0xffff0001) != 0x1) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_ERR,
               "SD: error occured whilst waiting for command complete interrupt\n");
#endif
        dev->last_error = irpts & 0xffff0000;
        dev->last_interrupt = irpts;
        return;
    }

    udelay(2 * SD_CMD_UDELAY);

    /* Get response data */
    switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
    case SD_CMD_RSPNS_TYPE_48:
    case SD_CMD_RSPNS_TYPE_48B:
        mmio_start(&s_entry);
        dev->last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
        mmio_end(&s_entry);
        break;

    case SD_CMD_RSPNS_TYPE_136:
        mmio_start(&s_entry);
        dev->last_r0 = mmio_read(EMMC_BASE + EMMC_RESP0);
        dev->last_r1 = mmio_read(EMMC_BASE + EMMC_RESP1);
        dev->last_r2 = mmio_read(EMMC_BASE + EMMC_RESP2);
        dev->last_r3 = mmio_read(EMMC_BASE + EMMC_RESP3);
        mmio_end(&s_entry);
        break;
    }

    /* If with data, wait for the appropriate interrupt */
    if ((cmd_reg & SD_CMD_ISDATA) && !is_sdma) {
        uint32_t wr_irpt;
        int is_write = 0;
        int cur_block;
        uint32_t * cur_buf_addr;

        if (cmd_reg & SD_CMD_DAT_DIR_CH) {
            wr_irpt = (1 << 5);     /* read */
        } else {
            is_write = 1;
            wr_irpt = (1 << 4);     /* write */
        }

        cur_block = 0;
        cur_buf_addr = (uint32_t *)dev->buf;
        while (cur_block < dev->blocks_to_transfer) {
            size_t cur_byte_no;

            mmio_start(&s_entry);
            TIMEOUT_WAIT(
                    mmio_read(EMMC_BASE + EMMC_INTERRUPT) & (wr_irpt | 0x8000),
                    timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0000 | wr_irpt);
            mmio_end(&s_entry);

            if ((irpts & (0xffff0000 | wr_irpt)) != wr_irpt) {
#ifdef configEMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for data ready interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;

                return;
            }

            /* Transfer the block */
            cur_byte_no = 0;
            while (cur_byte_no < dev->block_size) {
                uint32_t data;

                if (is_write) {
                    data = read_word((uint8_t *)cur_buf_addr, 0);
                    mmio_start(&s_entry);
                    mmio_write(EMMC_BASE + EMMC_DATA, data);
                    mmio_end(&s_entry);
                } else {
                    mmio_start(&s_entry);
                    data = mmio_read(EMMC_BASE + EMMC_DATA);
                    mmio_end(&s_entry);
                    write_word(data, (uint8_t *)cur_buf_addr, 0);
                }
                cur_byte_no += 4;
                cur_buf_addr++;
            }

            cur_block++;
        }
    }

    /* Wait for transfer complete (set if read/write transfer or with busy) */
    if ((((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||
       (cmd_reg & SD_CMD_ISDATA)) && (is_sdma == 0)) {
        /* First check command inhibit (DAT) is not already 0 */
        mmio_start(&s_entry);
        if ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) == 0) {
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
            mmio_end(&s_entry);
        } else {
            TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8002,
                         timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
            mmio_end(&s_entry);

            /*
             * Handle the case where both data timeout and transfer complete
             * are set - transfer complete overrides data timeout: HCSS 2.2.17
             */
            if (((irpts & 0xffff0002) != 0x2) &&
                    ((irpts & 0xffff0002) != 0x100002)) {
#ifdef configEMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }
            mmio_start(&s_entry);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0002);
            mmio_end(&s_entry);
        }
    } else if (is_sdma) {
        /*
         * For SDMA transfers, we have to wait for either transfer complete,
         * DMA int or an error
         */

        /* First check command inhibit (DAT) is not already 0 */
        mmio_start(&s_entry);
        if ((mmio_read(EMMC_BASE + EMMC_STATUS) & 0x2) == 0) {
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff000a);
            mmio_end(&s_entry);
        } else {
            TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x800a,
                         timeout);
            irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
            mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff000a);
            mmio_end(&s_entry);

            /* Detect errors */
            if ((irpts & 0x8000) && ((irpts & 0x2) != 0x2)) {
#ifdef configEMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            /*
             * Detect DMA interrupt without transfer complete
             * Currently not supported - all block sizes should fit in the
             *  buffer
             */
            if ((irpts & 0x8) && ((irpts & 0x2) != 0x2)) {
#ifdef configEMMC_DEBUG
                KERROR(KERROR_ERR,
                       "SD: error: DMA interrupt occured without transfer complete\n");
#endif
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            /* Detect transfer complete */
            if (irpts & 0x2) {
#ifdef configEMMC_DEBUG
                KERROR(KERROR_DEBUG, "SD: SDMA transfer complete");
#endif
                panic("NO DMA support");
                /* TODO */
                /* Transfer the data to the user buffer */
                /*memcpy(dev->buf, (const void *)SDMA_BUFFER,
                  dev->block_size); */
            } else {
                /* Unknown error */
#ifdef configEMMC_DEBUG
                if (irpts == 0) {
                    KERROR(KERROR_DEBUG,
                           "SD: timeout waiting for SDMA transfer to complete\n");
                } else {
                    KERROR(KERROR_ERR, "SD: unknown SDMA transfer error\n");
                }

                mmio_start(&s_entry);
                uint32_t emmc_status = mmio_read(EMMC_BASE + EMMC_STATUS);
                mmio_end(&s_entry);
                KERROR(KERROR_DEBUG, "SD: INTERRUPT: %x, STATUS %x\n",
                       (uint32_t)irpts, emmc_status);
#endif

                mmio_start(&s_entry);
                uint32_t d = mmio_read(EMMC_BASE + EMMC_STATUS) & 0x3;
                mmio_end(&s_entry);
                if (irpts == 0 && d == 2) {
                    /*
                     * The data transfer is ongoing and we should stop it.
                     */
#ifdef configEMMC_DEBUG
                    KERROR(KERROR_DEBUG, "SD: aborting transfer\n");
#endif
                    mmio_start(&s_entry);
                    mmio_write(EMMC_BASE + EMMC_CMDTM,
                               sd_commands[STOP_TRANSMISSION]);
                    mmio_end(&s_entry);
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

static void sd_handle_card_interrupt(struct emmc_block_dev * dev)
{
    /* Handle a card interrupt */

#ifdef configEMMC_DEBUG
#if 0
    uint32_t status = mmio_read(EMMC_BASE + EMMC_STATUS);

    KERROR(KERROR_DEBUG, "SD: card interrupt; controller status: %x\n",
           (uint32_t)status);
#endif
#endif

    /* Get the card status */
    if (dev->card_rca) {
        sd_issue_command_int(dev, sd_commands[SEND_STATUS], dev->card_rca << 16,
                             DEFAULT_CMD_TIMEOUT);
        if (FAIL(dev)) {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_ERR, "SD: unable to get card status\n");
#endif
        } else {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: card status: %x\n",
                   (uint32_t)dev->last_r0);
#endif
        }
    } else {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_ERR, "SD: no card currently selected\n");
#endif
    }
}

static void sd_handle_interrupts(struct emmc_block_dev *dev)
{
    uint32_t irpts;
    uint32_t reset_mask = 0;
    istate_t s_entry;

    mmio_start(&s_entry);
    irpts = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
    mmio_end(&s_entry);
    if (irpts & SD_COMMAND_COMPLETE) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious command complete interrupt\n");
#endif
        reset_mask |= SD_COMMAND_COMPLETE;
    }

    if (irpts & SD_TRANSFER_COMPLETE) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious transfer complete interrupt\n");
#endif
        reset_mask |= SD_TRANSFER_COMPLETE;
    }

    if (irpts & SD_BLOCK_GAP_EVENT) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious block gap event interrupt\n");
#endif
        reset_mask |= SD_BLOCK_GAP_EVENT;
    }

    if (irpts & SD_DMA_INTERRUPT) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious DMA interrupt\n");
#endif
        reset_mask |= SD_DMA_INTERRUPT;
    }

    if (irpts & SD_BUFFER_WRITE_READY) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious buffer write ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_WRITE_READY;
        sd_reset_dat();
    }

    if (irpts & SD_BUFFER_READ_READY) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: spurious buffer read ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_READ_READY;
        sd_reset_dat();
    }

    if (irpts & SD_CARD_INSERTION) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card insertion detected\n");
#endif
        reset_mask |= SD_CARD_INSERTION;
    }

    if (irpts & SD_CARD_REMOVAL) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card removal detected\n");
#endif
        reset_mask |= SD_CARD_REMOVAL;
        dev->card_removal = 1;
    }

    if (irpts & SD_CARD_INTERRUPT) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: card interrupt detected\n");
#endif
        sd_handle_card_interrupt(dev);
        reset_mask |= SD_CARD_INTERRUPT;
    }

    if (irpts & 0x8000) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_ERR, "SD: spurious error interrupt: %x\n",
                irpts);
#endif
        reset_mask |= 0xffff0000;
    }

    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, reset_mask);
    mmio_end(&s_entry);
}

static void sd_issue_command(struct emmc_block_dev *dev, uint32_t command,
                             uint32_t argument, useconds_t timeout)
{
    /* First, handle any pending interrupts. */
    sd_handle_interrupts(dev);

    /*
     * Stop the command issue if it was the card remove interrupt that was
     * handled.
     */
    if (dev->card_removal) {
        dev->last_cmd_success = 0;
        return;
    }

    /* Now run the appropriate commands by calling sd_issue_command_int() */
    if (command & IS_APP_CMD) {
        uint32_t rca;

        command &= 0xff;
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: issuing command ACMD%u\n", command);
#endif

        if (sd_acommands[command] == SD_CMD_RESERVED(0)) {
            KERROR(KERROR_ERR, "SD: invalid command ACMD%u\n", command);

            dev->last_cmd_success = 0;
            return;
        }
        dev->last_cmd = APP_CMD;

        rca = 0;
        if (dev->card_rca)
            rca = dev->card_rca << 16;
        sd_issue_command_int(dev, sd_commands[APP_CMD], rca, timeout);
        if (dev->last_cmd_success) {
            dev->last_cmd = command | IS_APP_CMD;
            sd_issue_command_int(dev, sd_acommands[command], argument, timeout);
        }
    } else {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: issuing command CMD%u\n", command);
#endif

        if (sd_commands[command] == SD_CMD_RESERVED(0)) {
            KERROR(KERROR_ERR, "SD: invalid command CMD%u\n", command);

            dev->last_cmd_success = 0;
            return;
        }

        dev->last_cmd = command;
        sd_issue_command_int(dev, sd_commands[command], argument, timeout);
    }

#ifdef configEMMC_DEBUG
    if (FAIL(dev)) {
        KERROR(KERROR_DEBUG,
           "SD: error issuing command: interrupts %x%s\n",
           (uint32_t)dev->last_interrupt,
           (dev->last_error == 0) ? ", TIMEOUT" : "");
        for (int i = 0; i < SD_ERR_RSVD; i++) {
            if (dev->last_error & (1 << (i + 16))) {
                KERROR(KERROR_DEBUG, "%s\n", err_irpts[i]);
            }
        }
    }
#endif
}

static int emmc_card_init(struct emmc_block_dev ** edev)
{
    uint32_t ver, vendor, sdversion, slot_status;
    uint32_t control1, d;
    istate_t s_entry;

    /* TODO use emmc_lock */

    /* Power cycle the card to ensure its in its startup state */
    if (emmc_hw.emmc_power_cycle() != 0) {
        KERROR(KERROR_ERR,
                "EMMC: Controller did not power cycle successfully\n");

        return -EIO;
    }
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: Controller power-cycled\n");
#endif

    /* Read the controller version */
    mmio_start(&s_entry);
    ver = mmio_read(EMMC_BASE + EMMC_SLOTISR_VER);
    mmio_end(&s_entry);
    vendor = ver >> 24;
    sdversion = (ver >> 16) & 0xff;
    slot_status = ver & 0xff;

    KERROR(KERROR_INFO,
           "EMMC: vendor %x, sdversion %x, slot_status %x\n",
           (uint32_t)vendor, (uint32_t)sdversion, (uint32_t)slot_status);
    emmccap.hci_ver = sdversion;

    if (emmccap.hci_ver < 2) {
        KERROR(KERROR_ERR, "EMMC: only SDHCI versions >= 3.0 are supported\n");

        return -EIO;
    }

    /* Reset the controller */
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: resetting controller\n");
#endif
    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);
    control1 |= (1 << 24);

    /* Disable clock */
    control1 &= ~(1 << 2);
    control1 &= ~(1 << 0);
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);

    TIMEOUT_WAIT((d = (mmio_read(EMMC_BASE + EMMC_CONTROL1) &
                                 (0x7 << 24))) == 0, 1000000);
    mmio_end(&s_entry);
    if (d != 0) {
        KERROR(KERROR_ERR, "EMMC: controller did not reset properly\n");

        return -EIO;
    }

#ifdef configEMMC_DEBUG
    uint32_t c0, c1, c2;

    mmio_start(&s_entry);
    c0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
    c1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    c2 = mmio_read(EMMC_BASE + EMMC_CONTROL2);
    mmio_end(&s_entry);
    KERROR(KERROR_DEBUG,
           "EMMC: control0: %x, control1: %x, control2: %x\n", c0, c1, c2);
#endif

    /* Read the capabilities registers */
    mmio_start(&s_entry);
    emmccap.capabilities[0] = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_0);
    emmccap.capabilities[1] = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_1);
    mmio_end(&s_entry);
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: capabilities: %x, %x\n",
           emmccap.capabilities[0], emmccap.capabilities[1]);
#endif

    /* Check for a valid card */
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: checking for an inserted card\n");
#endif
    uint32_t status_reg;

    mmio_start(&s_entry);
    TIMEOUT_WAIT((status_reg = mmio_read(EMMC_BASE + EMMC_STATUS)) & (1 << 16),
                 DEFAULT_CMD_TIMEOUT);
    mmio_end(&s_entry);
    if ((status_reg & (1 << 16)) == 0) {
        KERROR(KERROR_ERR, "EMMC: no card inserted\n");

        return -EIO;
    }
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: status: %x\n", (uint32_t)status_reg);
#endif

    /* Clear control2 */
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL2, 0);
    mmio_end(&s_entry);

    /* Get the base clock rate */
    uint32_t base_clock = emmc_hw.sd_get_base_clock_hz(&emmccap);
    if (base_clock == 0) {
        KERROR(KERROR_INFO, "EMMC: assuming clock rate to be 100MHz\n");

        base_clock = 100000000;
    }

    /* Set clock rate to something slow */
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: setting clock rate\n");
#endif
    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);
    control1 |= 1;            /* enable clock */

    /* Set to identification frequency (400 kHz) */
    uint32_t f_id = sd_get_clock_divider(base_clock, SD_CLOCK_ID);
    if (f_id == SD_GET_CLOCK_DIVIDER_FAIL) {
        KERROR(KERROR_ERR,
               "EMMC: unable to get a valid clock divider for ID frequency\n");

        return -EIO;
    }
    control1 |= f_id;

    control1 |= (7 << 16);        /* data timeout = TMCLK * 2^10 */
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    TIMEOUT_WAIT(d = (mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2), 1000000);
    mmio_end(&s_entry);
    if (d == 0) {
        KERROR(KERROR_WARN,
               "EMMC: controller's clock did not stabilise within 1 second\n");
    }
#ifdef configEMMC_DEBUG
    {
        uint32_t c0, c1;

        mmio_start(&s_entry);
        c0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        c1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        mmio_end(&s_entry);

        KERROR(KERROR_DEBUG, "EMMC: control0: %x, control1: %x\n", c0, c1);
    }
#endif

    /* Enable the SD clock */
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: enabling SD clock\n");
#endif
    udelay(2000);
    mmio_start(&s_entry);
    control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
    mmio_end(&s_entry);
    control1 |= 4;
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
    mmio_end(&s_entry);
    udelay(2000);
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: SD clock enabled\n");
#endif

    /* Mask off sending interrupts to the ARM */
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_IRPT_EN, 0);
    /* Reset interrupts */
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);
    mmio_end(&s_entry);
    /* Have all interrupts sent to the INTERRUPT register */
    uint32_t irpt_mask = 0xffffffff & (~SD_CARD_INTERRUPT);
#ifdef configEMMC_SD_CARD_INTERRUPTS
    irpt_mask |= SD_CARD_INTERRUPT;
#endif
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_IRPT_MASK, irpt_mask);
    mmio_end(&s_entry);

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: interrupts disabled\n");
#endif
    udelay(2000);

    /* Prepare the device structure */
    kmalloc_autofree struct emmc_block_dev * ret;
    ret = (*edev == NULL) ? kmalloc(sizeof(struct emmc_block_dev)) : *edev;

    memset(ret, 0, sizeof(struct emmc_block_dev));
    ret->dev.dev_id = DEV_MMTODEV(VDEV_MJNR_EMMC, 0);
    ret->dev.drv_name = driver_name;
    strlcpy(ret->dev.dev_name, device_name, sizeof(ret->dev.dev_name));
    ret->dev.block_size = 512;
    ret->dev.num_blocks = 0; /* FIXME */
    ret->dev.read = sd_read;
#ifdef configEMMC_WRITE_SUPPORT
    ret->dev.write = sd_write;
#endif
    ret->dev.lseek = sd_lseek;
    ret->dev.ioctl = sd_ioctl;
    ret->dev.flags = DEV_FLAGS_MB_READ | DEV_FLAGS_MB_WRITE;
    ret->base_clock = base_clock;

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "EMMC: device structure created\n");
#endif

    /* Send CMD0 to the card (reset to idle state) */
    sd_issue_command(ret, GO_IDLE_STATE, 0, DEFAULT_CMD_TIMEOUT);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: no CMD0 response\n");

        return -EIO;
    }

    /*
     * Send CMD8 to the card.
     * Voltage supplied = 0x1 = 2.7-3.6V (standard)
     * Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
     *
     * Note: A timeout error on the following command (CMD8) is normal
     * and expected if the SD card version is less than 2.0.
     */
    sd_issue_command(ret, SEND_IF_COND, 0x1aa, DEFAULT_CMD_TIMEOUT);
    int v2_later = 0;

    if (TIMEOUT(ret)) {
        v2_later = 0;
    } else if (CMD_TIMEOUT(ret)) {
        int err;
        err = sd_reset_cmd();
        if (err)
            return err;

        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        mmio_end(&s_entry);

        v2_later = 0;
    } else if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: failure sending CMD8 (%x)\n",
               (uint32_t)ret->last_interrupt);

        return -EIO;
    } else {
        if ((ret->last_r0 & 0xfff) != 0x1aa) {
            KERROR(KERROR_ERR, "SD: unusable card\n");
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: CMD8 response %x\n",
                   (uint32_t)ret->last_r0);
#endif
            return -EIO;
        } else {
            v2_later = 1;
        }
    }

    /*
     * Here we are supposed to check the response to CMD5 (HCSS 3.6)
     * It only returns if the card is a SDIO card.
     *
     * Note that a timeout error on the following command (CMD5) is
     * normal and expected if the card is not a SDIO card.
     */
    sd_issue_command(ret, IO_SET_OP_COND, 0, 10000);
    if (!TIMEOUT(ret) &&  CMD_TIMEOUT(ret)) {
        int err;

        err = sd_reset_cmd();
        if (err)
            return err;

        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
        mmio_end(&s_entry);
    } else {
        KERROR(KERROR_ERR,
               "SD: SDIO card detected - not currently supported\n");
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: CMD5 returned %x\n", (uint32_t)ret->last_r0);
#endif
        return -EIO;
    }

    /* Call an inquiry ACMD41 (voltage window = 0) to get the OCR */
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: sending inquiry ACMD41\n");
#endif
    sd_issue_command(ret, ACMD(41), 0, DEFAULT_CMD_TIMEOUT);

    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: inquiry ACMD41 failed\n");

        return -EIO;
    }
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: inquiry ACMD41 returned %x\n",
           (uint32_t)ret->last_r0);
#endif

    /* Call initialization ACMD41 */
    int card_is_busy = 1;
    while (card_is_busy) {
        uint32_t v2_flags = 0;
        if (v2_later) {
            /* Set SDHC support */
            v2_flags |= (1 << 30);

            /* Set 1.8v support */
#ifdef configEMMC_SD_1_8V_SUPPORT
            if (!ret->failed_voltage_switch)
                v2_flags |= (1 << 24);
#endif

            /* Enable SDXC maximum performance */
#ifdef configEMMC_SDXC_MAXIMUM_PERFORMANCE
            v2_flags |= (1 << 28);
#endif
        }

        sd_issue_command(ret, ACMD(41), 0x00ff8000 | v2_flags,
                         DEFAULT_CMD_TIMEOUT);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: error issuing ACMD41\n");

            return -EIO;
        }

        if ((ret->last_r0 >> 31) & 0x1) {
            /* Initialization is complete */
            ret->card_ocr = (ret->last_r0 >> 8) & 0xffff;
            ret->card_supports_sdhc = (ret->last_r0 >> 30) & 0x1;

#ifdef configEMMC_SD_1_8V_SUPPORT
            if (!ret->failed_voltage_switch)
                ret->card_supports_18v = (ret->last_r0 >> 24) & 0x1;
#endif

            card_is_busy = 0;
        } else {
            /* Card is still busy */
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: card is busy, retrying\n");
#endif
            udelay(500000);
        }
    }

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG,
           "SD: card identified: OCR: %x, 1.8v support: %i, SDHC support: %i\n",
           (uint32_t)ret->card_ocr, (int)ret->card_supports_18v,
           (int)ret->card_supports_sdhc);
#endif

    /*
     * At this point, we know the card is definitely an SD card,
     * so will definitely support SDR12 mode which runs at 25 MHz
     */
    sd_switch_clock_rate(base_clock, SD_CLOCK_NORMAL);

    /* A small wait before the voltage switch */
    udelay(5000);

    /* Switch to 1.8V mode if possible */
    if (ret->card_supports_18v) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_ERR, "SD: switching to 1.8V mode\n");
#endif
        /* As per HCSS 3.6.1 */

        /* Send VOLTAGE_SWITCH */
        sd_issue_command(ret, VOLTAGE_SWITCH, 0, DEFAULT_CMD_TIMEOUT);
        if (FAIL(ret)) {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_ERR, "SD: error issuing VOLTAGE_SWITCH\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return emmc_card_init(&ret);
        }

        /* Disable SD clock */
        mmio_start(&s_entry);
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        mmio_end(&s_entry);
        control1 &= ~(1 << 2);
        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
        mmio_end(&s_entry);

        /* Check DAT[3:0] */
        mmio_start(&s_entry);
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        mmio_end(&s_entry);
        uint32_t dat30 = (status_reg >> 20) & 0xf;
        if (dat30 != 0) {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: DAT[3:0] did not settle to 0\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return emmc_card_init(&ret);
        }

        /* Set 1.8V signal enable to 1 */
        mmio_start(&s_entry);
        uint32_t control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        mmio_end(&s_entry);
        control0 |= (1 << 8);
        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
        mmio_end(&s_entry);

        /* Wait 5 ms */
        udelay(5000);

        /* Check the 1.8V signal enable is set */
        mmio_start(&s_entry);
        control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
        mmio_end(&s_entry);
        if (((control0 >> 8) & 0x1) == 0) {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG,
                   "SD: controller did not keep 1.8V signal enable high\n");
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return emmc_card_init(&ret);
        }

        /* Re-enable the SD clock */
        mmio_start(&s_entry);
        control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
        mmio_end(&s_entry);
        control1 |= (1 << 2);
        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
        mmio_end(&s_entry);

        /* Wait 1 ms */
        udelay(10000);

        /* Check DAT[3:0] */
        mmio_start(&s_entry);
        status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
        mmio_end(&s_entry);
        dat30 = (status_reg >> 20) & 0xf;
        if (dat30 != 0xf) {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG,
                   "SD: DAT[3:0] did not settle to 1111b (%x)\n", dat30);
#endif
            ret->failed_voltage_switch = 1;
            sd_power_off();
            return emmc_card_init(&ret);
        }

#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: voltage switch complete\n");
#endif
    }

    /* Send CMD2 to get the cards CID */
    sd_issue_command(ret, ALL_SEND_CID, 0, DEFAULT_CMD_TIMEOUT);
    if (FAIL(ret)) {
        KERROR(KERROR_DEBUG, "SD: error sending ALL_SEND_CID\n");

        return -EIO;
    }
    uint32_t card_cid_0 = ret->last_r0;
    uint32_t card_cid_1 = ret->last_r1;
    uint32_t card_cid_2 = ret->last_r2;
    uint32_t card_cid_3 = ret->last_r3;

    uint32_t *dev_id = kmalloc(4 * sizeof(uint32_t));
    dev_id[0] = card_cid_0;
    dev_id[1] = card_cid_1;
    dev_id[2] = card_cid_2;
    dev_id[3] = card_cid_3;
    ret->cid = (uint8_t *)dev_id;
    ret->cid_len = 4 * sizeof(uint32_t);

    /* Send CMD3 to enter the data state */
    sd_issue_command(ret, SEND_RELATIVE_ADDR, 0, DEFAULT_CMD_TIMEOUT);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending SEND_RELATIVE_ADDR\n");

        return -EIO;
    }

    uint32_t cmd3_resp = ret->last_r0;
#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: CMD3 response: %x\n", cmd3_resp);
#endif

    ret->card_rca = (cmd3_resp >> 16) & 0xffff;
    uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
    uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
    uint32_t error = (cmd3_resp >> 13) & 0x1;
    uint32_t ready = (cmd3_resp >> 8) & 0x1;

    if (crc_error) {
        KERROR(KERROR_ERR, "SD: CRC error\n");

        kfree(dev_id);
        return -EIO;
    }

    if (illegal_cmd) {
        KERROR(KERROR_ERR, "SD: illegal command\n");

        kfree(dev_id);
        return -EIO;
    }

    if (error) {
        KERROR(KERROR_ERR, "SD: generic error\n");

        kfree(dev_id);
        return -EIO;
    }

    if (!ready) {
        KERROR(KERROR_ERR, "SD: not ready for data\n");

        kfree(dev_id);
        return -EIO;
    }

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: RCA: %x\n", (uint32_t)ret->card_rca);
#endif

    /* Now select the card (toggles it to transfer state) */
    sd_issue_command(ret, SELECT_CARD, ret->card_rca << 16,
                     DEFAULT_CMD_TIMEOUT);
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending CMD7\n");

        return -EIO;
    }

    uint32_t cmd7_resp = ret->last_r0;
    uint32_t status = (cmd7_resp >> 9) & 0xf;

    if ((status != 3) && (status != 4)) {
        KERROR(KERROR_ERR, "SD: invalid status (%u)\n", status);

        kfree(dev_id);
        return -EIO;
    }

    /* If not an SDHC card, ensure BLOCKLEN is 512 bytes */
    if (!ret->card_supports_sdhc) {
        sd_issue_command(ret, SET_BLOCKLEN, 512, DEFAULT_CMD_TIMEOUT);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: error sending SET_BLOCKLEN\n");

            return -EIO;
        }
    }

    mmio_start(&s_entry);
    uint32_t controller_block_size = mmio_read(EMMC_BASE + EMMC_BLKSIZECNT);
    mmio_end(&s_entry);

    controller_block_size &= (~0xfff);
    controller_block_size |= 0x200;
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_BLKSIZECNT, controller_block_size);
    mmio_end(&s_entry);

    /* Get the cards SCR register */
    ret->scr = kmalloc(sizeof(struct sd_scr));
    ret->buf = &ret->scr->scr[0];
    ret->block_size = 8;
    ret->blocks_to_transfer = 1;
    sd_issue_command(ret, SEND_SCR, 0, DEFAULT_CMD_TIMEOUT);
    ret->block_size = 512;
    if (FAIL(ret)) {
        KERROR(KERROR_ERR, "SD: error sending SEND_SCR\n");

        kfree(ret->scr);
        return -EIO;
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
    } else if (sd_spec == 2 && sd_spec3 == 0) {
        ret->scr->sd_version = SD_VER_2;
    } else if (sd_spec == 2 && sd_spec3 == 1 && sd_spec4 == 0) {
        ret->scr->sd_version = SD_VER_3;
    } else if (sd_spec == 2 && sd_spec3 == 1 && sd_spec4 == 1) {
        ret->scr->sd_version = SD_VER_4;
    }

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: &scr: %p\n", &ret->scr->scr[0]);

    KERROR(KERROR_DEBUG, "SD: SCR[0]: %x, SCR[1]: %x\n",
           (uint32_t)ret->scr->scr[0], (uint32_t)ret->scr->scr[1]);

    KERROR(KERROR_DEBUG, "SD: SCR: 0: %x 1: %x\n",
           __builtin_bswap32(ret->scr->scr[0]),
           __builtin_bswap32(ret->scr->scr[1]));

    KERROR(KERROR_DEBUG, "SD: SCR: version %s, bus_widths %x\n",
           sd_versions[ret->scr->sd_version],
           (uint32_t)ret->scr->sd_bus_widths);
#endif

    if (ret->scr->sd_bus_widths & 0x4) {
        uint32_t old_irpt_mask, new_iprt_mask;

        /* Set 4-bit transfer mode (ACMD6) */
        /* See HCSS 3.4 for the algorithm */
#ifdef configEMMC_SD_4BIT_DATA
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: switching to 4-bit data mode\n");
#endif

        /* Disable card interrupt in host */
        mmio_start(&s_entry);
        old_irpt_mask = mmio_read(EMMC_BASE + EMMC_IRPT_MASK);
        mmio_end(&s_entry);
        new_iprt_mask = old_irpt_mask & ~(1 << 8);
        mmio_start(&s_entry);
        mmio_write(EMMC_BASE + EMMC_IRPT_MASK, new_iprt_mask);
        mmio_end(&s_entry);

        /* Send ACMD6 to change the card's bit mode */
        sd_issue_command(ret, SET_BUS_WIDTH, 0x2, DEFAULT_CMD_TIMEOUT);
        if (FAIL(ret)) {
            KERROR(KERROR_ERR, "SD: switch to 4-bit data mode failed\n");
        } else {
            uint32_t control0;

            /* Change bit mode for Host */
            mmio_start(&s_entry);
            control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
            mmio_end(&s_entry);

            control0 |= 0x2;

            mmio_start(&s_entry);
            mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
            /* Re-enable card interrupt in host */
            mmio_write(EMMC_BASE + EMMC_IRPT_MASK, old_irpt_mask);
            mmio_end(&s_entry);

#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: switch to 4-bit complete\n");
#endif
        }
#endif
    }

    KERROR(KERROR_INFO, "SD: found a valid version %s SD card\n",
           sd_versions[ret->scr->sd_version]);

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "SD: setup successful (status %i)\n",
           (int)status);
#endif

    /* Reset interrupt register */
    mmio_start(&s_entry);
    mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);
    mmio_end(&s_entry);

    *edev = ret;
    ret = NULL; /* To prevent autofree. */

    return 0;
}

static int sd_ensure_data_mode(struct emmc_block_dev *edev)
{
    uint32_t status, cur_state;

    if (edev->card_rca == 0) {
        int err;

        /* Try again to initialise the card */
        err = emmc_card_init(&edev);
        if (err)
            return err;
    }

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG,
           "SD: ensure_data_mode() obtaining status register for card_rca %x\n",
           (uint32_t)edev->card_rca);
#endif

    sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16,
                     DEFAULT_CMD_TIMEOUT);
    if (FAIL(edev)) {
        KERROR(KERROR_ERR, "SD: ensure_data_mode() error sending CMD13\n");

        edev->card_rca = 0;

        return -EIO;
    }

    status = edev->last_r0;
    cur_state = (status >> 9) & 0xf;

#ifdef configEMMC_DEBUG
    KERROR(KERROR_DEBUG, "\tstatus %u\n", cur_state);
#endif

    if (cur_state == 3) {
        /* Currently in the stand-by state - select it */
        sd_issue_command(edev, SELECT_CARD, edev->card_rca << 16,
                         DEFAULT_CMD_TIMEOUT);
        if (FAIL(edev)) {
            KERROR(KERROR_ERR,
                   "SD: ensure_data_mode() no response from CMD17\n");

            edev->card_rca = 0;

            return -EIO;
        }
    } else if (cur_state == 5) {
        /* In the data transfer state - cancel the transmission */
        sd_issue_command(edev, STOP_TRANSMISSION, 0, DEFAULT_CMD_TIMEOUT);
        if (FAIL(edev)) {

            KERROR(KERROR_ERR,
                   "SD: ensure_data_mode() no response from CMD12\n");
            edev->card_rca = 0;
            return -EIO;
        }

        /* Reset the data circuit */
        sd_reset_dat();
    } else if (cur_state != 4) {
        /* Not in the transfer state - re-initialise */
        int ret = emmc_card_init(&edev);
        if (ret != 0)
            return ret;
    }

    /* Check again that we're now in the correct mode */
    if (cur_state != 4) {
#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "SD: ensure_data_mode() rechecking status\n");
#endif
        sd_issue_command(edev, SEND_STATUS, edev->card_rca << 16,
                         DEFAULT_CMD_TIMEOUT);
        if (FAIL(edev)) {
            KERROR(KERROR_ERR,
                   "SD: ensure_data_mode() no response from CMD13\n");

            edev->card_rca = 0;
            return -EIO;
        }
        status = edev->last_r0;
        cur_state = (status >> 9) & 0xf;

#ifdef configEMMC_DEBUG
        KERROR(KERROR_DEBUG, "cur_state: %u\n", cur_state);
#endif

        if (cur_state != 4) {
            KERROR(KERROR_ERR,
                   "SD: unable to initialise SD card to data mode (state %u)\n",
                   cur_state);

            edev->card_rca = 0;
            return -EIO;
        }
    }

    return 0;
}

#ifdef configEMMC_SDMA_SUPPORT
/* We only support DMA transfers to buffers aligned on a 4 kiB boundary */
static inline int sd_suitable_for_dma(void *buf)
{
    return ((uintptr_t)buf & 0xfff) ? 0 : 1;
}
#endif

static int sd_do_data_command(struct emmc_block_dev * edev, int is_write,
                              uint8_t * buf, size_t buf_size, uint32_t block_no)
{
    uint32_t command;
    const int max_retries = 3;
    int retry_count;

    /* PLSS table 4.20 - SDSC cards use byte addresses rather than
     * block addresses */
    if (!edev->card_supports_sdhc)
        block_no *= edev->dev.block_size;

    /* This is as per HCSS 3.7.2.1 */
    if (buf_size < edev->block_size) {
        KERROR(KERROR_ERR,
               "SD: do_data_command() called with buffer size (%u) less than "
               "block size (%u)\n", buf_size, (int)edev->block_size);

        return -EIO;
    }

    edev->blocks_to_transfer = buf_size / edev->block_size;
    if (buf_size % edev->block_size) {
        KERROR(KERROR_ERR,
               "SD: do_data_command() called with buffer size (%u) not an "
               "exact multiple of block size (%u)\n",
               buf_size, (int)edev->block_size);

        return -EIO;
    }
    edev->buf = buf;

    /*
     * Select command.
     */
    if (edev->blocks_to_transfer > 1) {
        command = (is_write) ? WRITE_MULTIPLE_BLOCK : READ_MULTIPLE_BLOCK;
    } else {
        command = (is_write) ? WRITE_BLOCK : READ_SINGLE_BLOCK;
    }

    retry_count = max_retries;
    do {
#ifdef configEMMC_SDMA_SUPPORT
        /* use SDMA for the first try only */
        if ((retry_count == max_retries) && sd_suitable_for_dma(buf)) {
            edev->use_sdma = 1;
        } else {
#ifdef configEMMC_DEBUG
            KERROR(KERROR_DEBUG, "SD: retrying without SDMA\n");
#endif
            edev->use_sdma = 0;
        }
#else
        edev->use_sdma = 0;
#endif

        sd_issue_command(edev, command, block_no, DEFAULT_CMD_TIMEOUT);
        if (FAIL(edev)) {
            KERROR(KERROR_ERR, "SD: error sending CMD%u, error = %d\n",
                   command, edev->last_error);
            if (--retry_count >= 0) {
                kputs("\tRetrying...\n");
            } else {
                kputs("\tGiving up.\n");
                edev->card_rca = 0;
                return -EIO;
            }
        }
    } while (FAIL(edev));

    return 0;
}

static ssize_t sd_read(struct dev_info * dev, off_t offset, uint8_t * buf,
                       size_t bcount, int oflags)
{
    struct emmc_block_dev * edev = containerof(dev, struct emmc_block_dev, dev);
    const uint32_t block_no = (uint32_t)offset;
    int err;
    ssize_t retval;

    mtx_lock(&emmc_lock);

    /* Check the status of the card */
    err = sd_ensure_data_mode(edev);
    if (err) {
        retval = -EIO;
        goto out;
    }

    err = sd_do_data_command(edev, 0, buf, bcount, block_no);
    if (err) {
        retval = err;
        goto out;
    }
    retval = bcount;

out:
    mtx_unlock(&emmc_lock);
    return retval;
}

#ifdef configEMMC_WRITE_SUPPORT
static ssize_t sd_write(struct dev_info * dev, off_t offset, uint8_t * buf,
                        size_t bcount, int oflags)
{
    struct emmc_block_dev * edev = containerof(dev, struct emmc_block_dev, dev);
    const uint32_t block_no = (uint32_t)offset;
    int err;
    ssize_t retval;

    mtx_lock(&emmc_lock);

    /* Check the status of the card */
    err = sd_ensure_data_mode(edev);
    if (err) {
        retval = -EIO;
        goto out;
    }

    err = sd_do_data_command(edev, 1, buf, bcount, block_no);
    if (err) {
        retval = err;
        goto out;
    }
    retval = bcount;

out:
    mtx_unlock(&emmc_lock);
    return retval;
}
#endif

static off_t sd_lseek(file_t * file, struct dev_info * dev, off_t offset,
                   int whence)
{
    struct emmc_block_dev * edev = containerof(dev, struct emmc_block_dev, dev);
    uint32_t block_no;
    off_t retval;

    mtx_lock(&emmc_lock);

    if (sd_ensure_data_mode(edev)) {
        retval = -EIO;
        goto out;
    }

    if (whence == SEEK_SET) {
        block_no = offset;
    } else if (whence == SEEK_CUR) {
        block_no = file->seek_pos + offset;
    } else {
        retval = -EINVAL;
        goto out;
    }

    if (block_no >= (uint32_t)dev->num_blocks) {
        retval = -EINVAL;
        goto out;
    }

    file->seek_pos = block_no;
    retval = block_no;
out:
    mtx_unlock(&emmc_lock);
    return retval;
}

static int sd_ioctl(struct dev_info * devnfo, uint32_t request,
        void * arg, size_t arg_len)
{
    switch (request) {
    default:
        return -EINVAL;
    }

    return 0;
}
