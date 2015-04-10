/**
 *******************************************************************************
 * @file    bcm2835_pm.h
 * @author  Olli Vanhoja
 * @brief   BCM2835 PM.
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
#ifndef BCM2835_PM_H
#define BCM2835_PM_H

/*
 * BCM2835 Power Management Device IDs
 */
#define BCM2835_SD      0x00000000 /*!< SD Card device id. */
#define BCM2835_UART0   0x00000001 /*!< UART0 device id. */
#define BCM2835_UART1   0x00000002 /*!< UART1 device id. */
#define BCM2835_USB     0x00000003 /*!< USB HCD device id. */
#define BCM2835_I2C0    0x00000004 /*!< I2C0 device id. */
#define BCM2835_I2C1    0x00000005 /*!< I2C1 device id. */
#define BCM2835_I2C2    0x00000006 /*!< I2C2 device id. */
#define BCM2835_SPI     0x00000007 /*!< SPI device id. */
#define BCM2835_CCP2TX  0x00000008 /*!< CCP2TX device id. */

/**
 * Get device power state.
 */
int bcm2835_pm_get_power_state(uint32_t devid);

/**
 * Set device power state.
 */
int bcm2835_pm_set_power_state(uint32_t devid, int state);

/**
 * Get wait time required after turning on a device.
 * A wait time period is needed after a device is turned on.
 * There is generally no need to call this function as
 * bcm2835_pm_set_power_state() handles the waiting anyway.
 * @param devid is the id of the device.
 * @return Returns a wait time in usec.
 */
int bcm2835_pm_get_timing(uint32_t devid);

#endif /* BCM2835_PM_H */
