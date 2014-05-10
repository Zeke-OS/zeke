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
#include <fs/fs.h>
#include <sys/sysctl.h>
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

static uart_port_t * kerror_uart;

/* kputs functions */
static void kputs_nolog(const char * str);
static void kputs_uart(const char * str);

void (*kputs)(const char *) = &kputs_uart; /* Boot value */
static int curr_klogger = KERROR_UARTLOG;
/** Array of klogger functions */
static void (*kputs_arr[])(const char *) = {
    [KERROR_NOLOG] = &kputs_nolog,
    [KERROR_LASTLOG] = &kputs_nolog, /* TODO */
    [KERROR_UARTLOG] = &kputs_uart
};

void kerror_init(void)
{
    uart_port_init_t uart_conf = {
        .baud_rate  = UART_BAUDRATE_115200,
        .data_bits  = UART_DATABITS_8,
        .stop_bits  = UART_STOPBITS_ONE,
        .parity     = UART_PARITY_NO,
    };

    kerror_uart = uart_getport(0);
    kerror_uart->init(&uart_conf);

    KERROR(KERROR_INFO, "Kerror logger initialized");
}
//HW_PREINIT_ENTRY(kerror_init);


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
    char buf[200];

    ksprintf(buf, sizeof(buf), "%c:%s%s\n", level, where, msg);
    kputs(buf);
}

static void kputs_nolog(const char * str)
{
}

static void kputs_uart(const char * str)
{
    size_t i = 0;
    static char kbuf[1024] = "";

    /* TODO static buffering, lock etc. */
    strnncat(kbuf, sizeof(kbuf), str, sizeof(kbuf));
    if (!kerror_uart) {
        return;
    }

    while (kbuf[i] != '\0') {
        if (kbuf[i] == '\n')
            kerror_uart->uputc('\r');
        kerror_uart->uputc(kbuf[i++]);
    }
    kbuf[0] = '\0';
}

/**
 * sysctl function to read current klogger and change it.
 */
static int sysctl_kern_klogger(SYSCTL_HANDLER_ARGS)
{
    int error;

    error = sysctl_handle_int(oidp, &curr_klogger, sizeof(curr_klogger), req);
    if (!error && req->newptr) {
        kputs = kputs_arr[curr_klogger];
    }

    return error;
}

SYSCTL_PROC(_kern, OID_AUTO, klogger, CTLTYPE_INT | CTLFLAG_RW,
        NULL, 0, sysctl_kern_klogger, "I", "Kernel logger type.");

