/**
 *******************************************************************************
 * @file    dyndebug.c
 * @author  Olli Vanhoja
 * @brief   Dynamic kerror debug messages.
 * @section LICENSE
 * Copyright (c) 2016, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <buf.h>
#include <fs/procfs.h>
#include <fs/procfs_dbgfile.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>

#define DD_MAX_LINE 40

__GLOBL(__start_set_debug_msg_sect);
__GLOBL(__stop_set_debug_msg_sect);
extern struct _kerror_dyndebug_msg __start_set_debug_msg_sect;
extern struct _kerror_dyndebug_msg __stop_set_debug_msg_sect;

static int toggle_dbgmsg(char * cfg)
{
    struct _kerror_dyndebug_msg * msg_opt = &__start_set_debug_msg_sect;
    struct _kerror_dyndebug_msg * stop = &__stop_set_debug_msg_sect;
    char strbuf[DD_MAX_LINE];
    char * file;
    char * line;

    if (msg_opt == stop)
        return -EINVAL;

    if (cfg[0] == '*') { /* Match all */
        file = NULL;
        line = NULL;
    } else { /* Match specfic file */
        strlcpy(strbuf, cfg, sizeof(strbuf));
        file = strbuf;
        line = kstrchr(strbuf, ':');

        if (line) { /* Match line number */
            line[0] = '\0';
            line++;
        }
    }

    while (msg_opt < stop) {
        if (file) {
            if (strcmp(file, msg_opt->file) != 0)
                goto next;

            if (line && *line != '\0') {
                char msgline[12];

                uitoa32(msgline, msg_opt->line);
                if (strcmp(line, msgline) != 0)
                    goto next;
            }
        }

        /* Toggle */
        msg_opt->flags ^= 1;

next:
        msg_opt++;
    }

    return 0;
}

/**
 * Enable dyndbg messages configured in Kconfig.
 */
void dyndebug_early_boot_init(void)
{
    static char cfgs_str[] = configDYNDEBUG_BOOTPARMS;
    char * cfgs = cfgs_str;
    char * cfg;

    while ((cfg = strsep(&cfgs, ";, "))) {
        toggle_dbgmsg(cfg);
    }
}

static int read_dyndebug(void * buf, size_t max, void * elem)
{
    struct _kerror_dyndebug_msg * msg_opt = elem;

    return ksprintf(buf, max, "%u:%s:%d\n",
                    msg_opt->flags, msg_opt->file, msg_opt->line);
}

static ssize_t write_dyndebug(const void * buf, size_t bufsize)
{
    int err;

    if (!strvalid((char *)buf, bufsize))
        return -EINVAL;

    err = toggle_dbgmsg((char *)buf);
    if (err)
        return err;

    return bufsize;
}

PROCFS_DBGFILE(dyndebug,
               &__start_set_debug_msg_sect, &__stop_set_debug_msg_sect,
               read_dyndebug, write_dyndebug);
