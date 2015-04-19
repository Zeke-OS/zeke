/**
 *******************************************************************************
 * @file    kerror.h
 * @author  Olli Vanhoja
 * @brief   Kernel error logging.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stddef.h>
#include <kstring.h>

#define KERROR_NOLOG    0
#define KERROR_BUF      1
#define KERROR_UARTLOG  2

/* Log levels */
#define KERROR_CRIT     '0' /*!< Critical error system is halted. */
#define KERROR_ERR      '1' /*!< Fatal error. */
#define KERROR_WARN     '2' /*!< Unexpected condition. */
#define KERROR_INFO     '3' /*!< Normal informational message. */
#define KERROR_DEBUG    '4' /*!< Debug message. */

#if defined(configDEBUG_CRIT)
#define configDEBUG KERROR_CRIT
#elif defined(configDEBUG_ERR)
#define configDEBUG KERROR_ERR
#elif defined(configDEBUG_WARN)
#define configDEBUG KERROR_WARN
#elif defined(configDEBUG_INFO)
#define configDEBUG KERROR_INFO
#elif defined(configDEBUG_DEBUG)
#define configDEBUG KERROR_DEBUG
#endif

/* Line number as a string */
#define _KERROR_S(x) #x
#define _KERROR_S2(x) _KERROR_S(x)
#define S__LINE__ _KERROR_S2(__LINE__)

#if configKLOGGER == 0
#define _KERROR_FN(level, where, fmt, ...) ((void)0)
#else
#define _KERROR_FN(level, where, fmt, ...) do {                             \
    char _kerror_buf[configKERROR_MAXLEN];                                  \
    size_t _kerror_i = ksprintf(_kerror_buf, sizeof(_kerror_buf), "%c:%s",  \
                                level, where);                              \
    ksprintf(_kerror_buf + _kerror_i - 1, sizeof(_kerror_buf) - _kerror_i,  \
            fmt, ##__VA_ARGS__);                                            \
    kputs(_kerror_buf);                                                     \
} while (0)
#endif

#define _KERROR_WHERESTR (__FILE__ ":" S__LINE__ ": ")
#define _KERROR2(level, where, fmt, ...) _KERROR_FN(level, where, fmt, ##__VA_ARGS__)

/**
 * KERROR macro for logging kernel errors.
 * Expect that storage space for messages may vary depending on selected logging
 * method and additional data stored like file and line number.
 * @param level Log level: KERROR_LOG, KERROR_WARN or KERROR_ERR.
 * @param msg Message to be logged.
 */
#define KERROR(level, fmt, ...) _KERROR2(level, _KERROR_WHERESTR, fmt, ##__VA_ARGS__)

#if configKLOGGER != 0
extern const char * const _kernel_panic_msg;
#endif

/**
 * Kernel panic with message.
 * @param msg is a message to be logged before halt.
 */
#define panic(msg) do {                             \
    disable_interrupt();                            \
    KERROR(KERROR_CRIT, "%s", _kernel_panic_msg);   \
    KERROR(KERROR_CRIT, "%s", msg);                 \
    panic_halt();                                   \
} while(0)

#ifdef configKASSERT
#define KASSERT(invariant, msg) do {    \
    if (!(invariant)) { panic(msg); }   \
} while(0)
#else
#define KASSERT(invariant, msg)
#endif

struct kerror_klogger {
    size_t id;

    /**
     * Initialize the klogger.
     * @remarks Can be called multiple times.
     */
    void (*init)(void);

    /**
     * Insert a line to the logger.
     * @param str is the line.
     */
    void (*puts)(const char * str);

    void (*read)(char * str, size_t len);

    /**
     * Flush contents of the logger to the current kputs.
     * This can be used to flush old logger to the new logger
     * when changing klogger.
     */
    void (*flush)(void);
};

void (*kputs)(const char *);

#endif /* KERROR_H */
