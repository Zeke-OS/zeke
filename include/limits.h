/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Implementation-defined constants.
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

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup limits.h
 * @{
 */

#ifndef _LIMITS_H_
#define _LIMITS_H_

#include <sys/_limits.h>

/* Runtime Invariant Values */
#define HOST_NAME_MAX   255

/* Pathname Variable Values */
#define FILESIZEBITS    32
#define LINK_MAX        16
#define LINE_MAX        4096
#define MAX_INPUT       255
#define NAME_MAX        255         /*!< Maximum file name length. */
#define PATH_MAX        4096        /*!< Maximum path length. */
#define NGROUPS_MAX     16

/* TODO PIPE_BUF */


/* Runtime Increasable Values */
/* Maximum Values */
/* Minimum Values */
#define _POSIX2_LINE_MAX    LINE_MAX
#define _POSIX_ARG_MAX      ARG_MAX
#define _POSIX_LINK_MAX     LINK_MAX
#define _XOPEN_PATH_MAX     PATH_MAX

/* Other Invariant Values */
#define NZERO           0       /*!< Default process priority. */

#define ARG_MAX         4096    /*!< Maximum size of argv and env combined. */

#endif /* _LIMITS_H_ */

/**
 * @}
 */

/**
 * @}
 */
