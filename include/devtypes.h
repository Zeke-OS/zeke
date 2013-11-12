/**
 *******************************************************************************
 * @file    devtypes.h
 * @author  Olli Vanhoja
 * @brief   Device driver types header file.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/** @addtogroup Dev
  * @{
  */

#pragma once
#ifndef DEVTYPES_H
#define DEVTYPES_H

#include <stdint.h>

typedef uint32_t dev_t; /*!< Device identifier */

#define DEV_MAJORDEVS   16 /*!< Number of major devs 2^nbr_of_marjor_bits */
#define DEV_MINORBITS   28 /*!< Number of minor bits */
#define DEV_MINORMASK   ((1u << DEV_MINORBITS) - 1) /*!< Minor bits mask */

/**
 * Get major number from osDev_t.
 */
#define DEV_MAJOR(dev)  ((unsigned int)((dev) >> DEV_MINORBITS))

/**
 * Get minor number from osDev_t.
 */
#define DEV_MINOR(dev)  ((unsigned int)((dev) & DEV_MINORMASK))

/**
 * Convert major, minor pair into osDev_t.
 */
#define DEV_MMTODEV(ma, mi) (((ma) << DEV_MINORBITS) | (mi))

#endif /* DEVTYPES_H */

/**
  * @}
  */
