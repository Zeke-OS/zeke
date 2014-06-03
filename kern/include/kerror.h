/**
 *******************************************************************************
 * @file    kerror.h
 * @author  Olli Vanhoja
 * @brief   Kernel error logging.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <stddef.h>

#define KERROR_NOLOG    0
#define KERROR_BUF      1
#define KERROR_UARTLOG  2
#define KERROR_FB       3

/* Line number as a string */
#define _KERROR_S(x) #x
#define _KERROR_S2(x) _KERROR_S(x)
#define S__LINE__ _KERROR_S2(__LINE__)

#if configKLOGGER == 0
#define kerror_print_macro(level, where, msg)
#endif

#define _KERROR_FN(level, where, msg) kerror_print_macro(level, where, msg)
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

const char _kernel_panic_msg[19];

/**
 * Kernel panic with message.
 * @param msg is a message to be logged before halt.
 */
#define panic(msg) do {                     \
    disable_interrupt();                    \
    KERROR(KERROR_CRIT, _kernel_panic_msg); \
    KERROR(KERROR_CRIT, msg);               \
    panic_halt();                           \
} while(1)

/* Log levels */
#define KERROR_CRIT     '0' /*!< Critical error system is halted. */
#define KERROR_ERR      '1' /*!< Fatal error. */
#define KERROR_WARN     '2' /*!< Unexpected condition. */
#define KERROR_INFO     '3' /*!< Normal informational message. */
#define KERROR_DEBUG    '4' /*!< Debug message. */

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

#if configKLOGGER != 0
void kerror_print_macro(char level, const char * where, const char * msg);
void (*kputs)(const char *);
#endif

#endif /* KERROR_H */
