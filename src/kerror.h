/**
 *******************************************************************************
 * @file    kerror.h
 * @author  Olli Vanhoja
 * @brief   Kernel error logging.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#ifndef KERROR_H
#define KERROR_H

#include <autoconf.h>
#include <hal/hal_core.h>

/* Store few last kernel error messages in memory */
#if configKERROR_LAST != 0
#include <kerror_lastlog/kerror_last.h>
#elif configKERROR_TTYS != 0
#include <kerror_ttys/kerror_ttys.h>
#else /* No kerner error logging */
#define _KERROR_FN(level, where, msg) do {} while (0)
#endif

/* Line number as a string */
#define _KERROR_S(x) #x
#define _KERROR_S2(x) _KERROR_S(x)
#define S__LINE__ _KERROR_S2(__LINE__)

#define _KERROR_WHERESTR __FILE__ ":" S__LINE__ ": "
#define _KERROR2(level, where, msg) _KERROR_FN(level, where, msg)
/**
 * KERROR macro for logging kernel errors.
 * Expect that storage space for messages may vary depending on selected logging
 * method and additional data stored like file and line number.
 * @param level Log level: KERROR_LOG, KERROR_WARN or KERROR_ERR.
 * @param msg Message to be logged.
 */
#define KERROR(level, msg) _KERROR2(level, _KERROR_WHERESTR, msg)

/**
 * Kernel panic with message.
 * @param msg is a message to be logged before halt.
 */
#define panic(msg)                              \
    KERROR(KERROR_CRIT, "Oops, Kernel panic");  \
    KERROR(KERROR_CRIT, msg);                   \
    do {                                        \
         panic_halt();                          \
    } while(1)

/* Log levels */
#define KERROR_CRIT     '0' /*!< Critical error system is halted. */
#define KERROR_ERR      '1' /*!< Fatal error. */
#define KERROR_WARN     '2' /*!< Unexpected condition. */
#define KERROR_LOG      '3' /*!< Normal informational message. */
#define KERROR_DEBUG    '4' /*!< Debug message. */

#endif /* KERROR_H */
