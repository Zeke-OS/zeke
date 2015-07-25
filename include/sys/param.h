/**
 *******************************************************************************
 * @file    param.h
 * @author  Olli Vanhoja
 * @brief   Definitions.
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

#ifndef _SYS_PARAM_H_
#define _SYS_PARAM_H_

#include <limits.h>

#define MAXCOMLEN       19      /*!< Max command name remembered. */
#define MAXHOSTNAMELEN  HOST_NAME_MAX /*!< Max hostname size. */
#define MAXLOGNAME      33      /*!< max login name length (incl. NUL). */
#define MAXPATHLEN      PATH_MAX
#define SPECNAMELEN     20      /*!< Max length of devicename. */


#define DEV_MAJORDEVS   256     /*!< Number of major devs 2^nr_marjor_bits */
#define DEV_MINORBITS   24      /*!< Number of minor bits */
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

#define CMASK 022 /*!< File creation mask: S_IWGRP|S_IWOTH */

#endif /* _SYS_PARAM_H_ */
