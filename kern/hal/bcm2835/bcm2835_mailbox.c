/**
 *******************************************************************************
 * @file    bcm2835_mailbox.c
 * @author  Olli Vanhoja
 * @brief   Access to BCM2835 mailboxes.
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
#include <hal/hw_timers.h>
#include "bcm2835_mmio.h"
#include "bcm2835_mailbox.h"

/* Mailbox mmio addresses */
#define MAILBOX0_BASE   0x2000b880
#define MAILBOX0_READ   (MAILBOX0_BASE + 0x0)   /* Read and remove. */
#define MAILBOX0_PEAK   (MAILBOX0_BASE + 0x10)  /* Read without removing. */
#define MAILBOX0_SENDER (MAILBOX0_BASE + 0x14)  /* 2 bits */
#define MAILBOX0_STATUS (MAILBOX0_BASE + 0x18)
#define MAILBOX0_CONFIG (MAILBOX0_BASE + 0x1c)
#define MAILBOX0_WRITE  (MAILBOX0_BASE + 0x20)  /* Read register of mb1 */

/* Read/Write bit masks */
#define MBWR_CHANNEL    0xf
#define MBWR_DATA       0xfffffff0

/* Status bit masks */
#define MBSTAT_FULL     0x80000000 /*!< Write mailbox full status mask. */
#define MBSTAT_EMPTY    0x40000000 /*!< Read mailbox is empty stataus mask. */

int bcm2835_readmailbox(unsigned int channel, uint32_t * data)
{
    unsigned int count = 0;
    uint32_t d;
    uint32_t mb_status;
    istate_t s_entry;

    do {
        /* Wait for incoming data */
        do {
            mmio_start(&s_entry);
            mb_status = mmio_read(MAILBOX0_STATUS);
            mmio_end(&s_entry);

            if (count++ > (1 << 25)) {
                return -EIO; /* no luck */
            }
        } while (mb_status & MBSTAT_EMPTY);

        /* Read data. */
        mmio_start(&s_entry);
        d = mmio_read(MAILBOX0_READ);
        mmio_end(&s_entry);
    } while ((d & MBWR_CHANNEL) != channel);

    *data = (d & MBWR_DATA);
    return 0;
}

int bcm2835_writemailbox(unsigned int channel, uint32_t data)
{
    uint32_t mb_status;
    istate_t s_entry;

    mmio_start(&s_entry);
    /* Wait until mailbox is not full */
    TIMEOUT_WAIT((mb_status = mmio_read(MAILBOX0_STATUS)), 2000);
    if (mb_status & MBSTAT_FULL) {
        mmio_end(&s_entry);
        return -EIO;
    }

    mmio_write(MAILBOX0_WRITE, (data | channel));
    mmio_end(&s_entry);

    return 0;
}
