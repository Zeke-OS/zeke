/**
 *******************************************************************************
 * @file    kerror.c
 * @author  Olli Vanhoja
 * @brief   Kernel error logging.
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

#define KERNEL_INTERNAL
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>
#include <fs/fs.h>
#include <sys/sysctl.h>
#include <errno.h>
#include <hal/uart.h>
#include <kerror.h>

const char _kernel_panic_msg[19] = "Oops, Kernel panic";

static size_t kerror_fdwrite(vnode_t * file, const off_t * offset,
        const void * buf, size_t count);

vnode_ops_t kerror_vops = {
    .write = kerror_fdwrite
};

vnode_t kerror_vnode = {
    .vnode_num = 0,
    .refcount = 0,
    .len = SIZE_MAX,
    .vnode_ops = &kerror_vops
};

struct kerror_logger {
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

/* klogger init functions */
extern void kerror_lastlog_init(void);
extern void kerror_uart_init(void);

/* klogger puts functions */
static void nolog_puts(const char * str);
extern void kerror_lastlog_puts(const char * str);
extern void kerror_uart_puts(const char * str);

/* klogger flush functions */
extern void kerror_lastlog_flush(void);

void (*kputs)(const char *) = &kerror_lastlog_puts; /* Boot value */
static size_t curr_klogger = KERROR_LASTLOG; /* Boot value */

/** Array of kloggers */
static struct kerror_logger klogger_arr[] = {
    [KERROR_NOLOG] = {
        .init   = 0,
        .puts   = &nolog_puts,
        .read   = 0,
        .flush  = 0},
    [KERROR_LASTLOG] = {
        .init   = &kerror_lastlog_init,
        .puts   = &kerror_lastlog_puts,
        .read   = 0,
        .flush = &kerror_lastlog_flush},
#if configKERROR_UART != 0
    [KERROR_UARTLOG] = {
        .init   = &kerror_uart_init,
        .puts   = &kerror_uart_puts,
        .read   = 0,
        .flush  = 0}
#endif
};

static int klogger_change(size_t new_klogger, size_t old_klogger);

void kerror_init(void) __attribute__((constructor));
void kerror_init(void)
{
    SUBSYS_INIT();

    klogger_change(configDEF_KLOGGER, curr_klogger);

    SUBSYS_INITFINI("Kerror logger OK");
}

/**
 * Kernel fake fd write function to print kerror messages from usr mode threads.
 */
static size_t kerror_fdwrite(vnode_t * file, const off_t * offset,
        const void * buf, size_t count)
{
    kputs(buf);
    return count;
}

void kerror_print_macro(char level, const char * where, const char * msg)
{
    char buf[configKERROR_MAXLEN];

    ksprintf(buf, sizeof(buf), "%c:%s%s\n", level, where, msg);
    kputs(buf);
}

static void nolog_puts(const char * str)
{
}

/**
 * sysctl function to read current klogger and change it.
 */
static int sysctl_kern_klogger(SYSCTL_HANDLER_ARGS)
{
    int error;
    size_t new_klogger = curr_klogger;
    size_t old_klogger = curr_klogger;

    error = sysctl_handle_int(oidp, &new_klogger, sizeof(new_klogger), req);
    if (!error && req->newptr) {
        error = klogger_change(new_klogger, old_klogger);
    }

    return error;
}

static int klogger_change(size_t new_klogger, size_t old_klogger)
{
    if (new_klogger > num_elem(klogger_arr))
        return EINVAL;

    if (klogger_arr[new_klogger].init)
        klogger_arr[new_klogger].init();

    kputs = klogger_arr[new_klogger].puts;

    if (klogger_arr[old_klogger].flush)
        klogger_arr[old_klogger].flush();

    curr_klogger = new_klogger;

    return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, klogger, CTLTYPE_INT | CTLFLAG_RW,
        NULL, 0, sysctl_kern_klogger, "I", "Kernel logger type.");

