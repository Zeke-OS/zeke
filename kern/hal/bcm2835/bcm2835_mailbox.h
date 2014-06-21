/**
 *******************************************************************************
 * @file    bcm2835_mailbox.h
 * @author  Olli Vanhoja
 * @brief   Access to BCM2835 mailboxes.
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

#pragma once
#ifndef BCM2835_MAILBOX_H
#define BCM2835_MAILBOX_H

#include <stdint.h>

/* Mailbox channels */
#define BCM2835_MBCH_PM         0   /*!< Power management interface */
#define BCM2835_MBCH_FB         1   /*!< Frame Buffer */
#define BCM2835_MBCH_VUART      2   /*!< Virtual UART */
#define BCM2835_MBCH_VCHIQ      3   /*!< VCHIQ interface */
#define BCM2835_MBCH_LEDS       4   /*!< LEDs interface */
#define BCM2835_MBCH_BUTTONS    5   /*!< Buttons interface */
#define BCM2835_MBCH_TOUCH      6   /*!< Touch screen interface */
#define BCM2835_MBCH_COUNT      7
#define BCM2835_MBCH_PROP       8

/**
 * Read from BCM2835 mailbox.
 * @param channel is a channel number.
 * @return  Returns received mailbox value;
 *          If no data is received 0xffffffff is returned.
 */
uint32_t bcm2835_readmailbox(unsigned int channel);

/**
 * Write to BCM2835 mailbox.
 * @param channel   is a channel number.
 * @param data      is the data to be written.
 */
void bcm2835_writemailbox(unsigned int channel, uint32_t data);

#endif /* BCM2835_MAILBOX_H */
