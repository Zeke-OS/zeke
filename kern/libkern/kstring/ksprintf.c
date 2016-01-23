/**
 *******************************************************************************
 * @file    ksprintf.c
 * @author  Olli Vanhoja
 * @brief   Function to compose a string by using printf style formatter.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdarg.h>
#include <kactype.h>
#include <kstring.h>

union value_buffer {
    char value_char;
    short value_short;
    int value_int;
    long value_long;
    long long value_2long;
    size_t value_size;
    void * value_p;
};

int ksprintf(char * str, size_t maxlen, const char * format, ...)
{
    va_list args;
    size_t fmt_i = 0, str_i = 0;
    char c;

    maxlen--;
    va_start(args, format);
    while ((c = format[fmt_i++]) != '\0' && str_i <= maxlen) {
        uint16_t flags;
        char p_specifier = 0;
        size_t value_size;
        union value_buffer value;
        struct ksprintf_formatter ** formatter;

        switch (c) {
        case '%':
            c = format[fmt_i++];

            /* Length modifier */
            switch (c) {
            case '\0':
                goto out;
            case '%':
                flags = 0;
                value_size = 0;
                break;
            case 'h':
                if (format[fmt_i] == 'h') {
                    flags = KSPRINTF_FMTFLAG_hh;
                    value_size = sizeof(int);
                    value.value_char = (char)va_arg(args, int);
                    fmt_i++;
                    c = format[fmt_i++];
                } else {
                    flags = KSPRINTF_FMTFLAG_h;
                    value_size = sizeof(int);
                    value.value_short = (short)va_arg(args, int);
                }
                break;
            case 'l':
                if (format[fmt_i] == 'l') {
                    flags = KSPRINTF_FMTFLAG_ll;
                    value_size = sizeof(uint64_t);
                    value.value_2long = (long long)va_arg(args, uint64_t);
                    fmt_i++;
                    c = format[fmt_i++];
                } else {
                    flags = KSPRINTF_FMTFLAG_ll;
                    value_size = sizeof(long);
                    value.value_long = (long)va_arg(args, long);
                    c = format[fmt_i++];
                }
                break;
            case 'z':
                flags = KSPRINTF_FMTFLAG_z;
                value_size = sizeof(size_t);
                value.value_size = (size_t)va_arg(args, size_t);
                c = format[fmt_i++];
                break;
            case 'p': /* width and conversion specifier + p_specifier */
                if (ka_isupper(format[fmt_i])) {
                    /* Pointer format specifiers are always upper case */
                    p_specifier = format[fmt_i++];
                }
                /* fall through */
            case 's': /* width and conversion specifier */
                flags = KSPRINTF_FMTFLAG_p;
                value_size = sizeof(void *);
                value.value_p = (void *)va_arg(args, void *);
                break;
            default:
                flags = KSPRINTF_FMTFLAG_i;
                value_size = sizeof(int);
                value.value_int = (int)va_arg(args, int);
                break;
            }

            /* Conversion specifier */
            switch (c) {
            case '\0':
                goto out;
            case 'c':
                str[str_i++] = (char)value.value_int;
                break;
            case '%':
                str[str_i++] = c;
                break;
            default:
                SET_FOREACH(formatter, ksprintf_formatters) {
                    struct ksprintf_formatter * fmt = *formatter;

                    if ((c == fmt->specifier || c == fmt->alt_specifier) &&
                        p_specifier == fmt->p_specifier &&
                        !!(fmt->flags & flags)) {
                        str_i += fmt->func(str + str_i, &value, value_size,
                                           maxlen - str_i);
                        break;
                    }
                }
                break;
            }
            break;
        default:
            str[str_i++] = c;
            break;
        }
    }

out:
    str[str_i] = '\0';
    va_end(args);

    return str_i + 1;
}


static int ksprintf_fmt_sdecimal(KSPRINTF_FMTFUN_ARGS)
{
    int64_t value;
    int i = 0;

    switch (value_size) {
    case sizeof(int64_t):
        value = *((int64_t *)value_p);
        break;
    case sizeof(int32_t):
    default:
        value = *((int32_t *)value_p);
    }

    if (value < 0) {
        value *= -1;
        str[i++] = '-';
    }

    return i + uitoa64(str + i, (uint64_t)(value));
}

static const struct ksprintf_formatter ksprintf_fmt_sdecimal_st = {
    .flags = KSPRINTF_FMTFLAG_hh | KSPRINTF_FMTFLAG_h | KSPRINTF_FMTFLAG_i |
             KSPRINTF_FMTFLAG_l | KSPRINTF_FMTFLAG_ll,
    .specifier = 'd',
    .alt_specifier = 'i',
    .func = ksprintf_fmt_sdecimal,
};
KSPRINTF_FORMATTER(ksprintf_fmt_sdecimal_st);


static int ksprintf_fmt_udecimal(KSPRINTF_FMTFUN_ARGS)
{
    uint64_t value;

    switch (value_size) {
    case sizeof(uint64_t):
        value = *((uint64_t *)value_p);
        break;
    case sizeof(uint32_t):
    default:
        value = *((uint32_t *)value_p);
    }

    return uitoa64(str, value);
}

static const struct ksprintf_formatter ksprintf_fmt_udecimal_st = {
    .flags = KSPRINTF_FMTFLAG_hh | KSPRINTF_FMTFLAG_h | KSPRINTF_FMTFLAG_i |
             KSPRINTF_FMTFLAG_l | KSPRINTF_FMTFLAG_ll,
    .specifier = 'u',
    .func = ksprintf_fmt_udecimal,
};
KSPRINTF_FORMATTER(ksprintf_fmt_udecimal_st);


static int ksprintf_fmt_octal(KSPRINTF_FMTFUN_ARGS)
{
    switch (value_size) {
    case sizeof(uint64_t):
        return uitoa64base(str, *((uint64_t *)value_p), 8);
    case sizeof(uint32_t):
    default:
        return uitoa32base(str, *((uint32_t *)value_p), 8);
    }
}

static const struct ksprintf_formatter ksprintf_fmt_octal_st = {
    .flags = KSPRINTF_FMTFLAG_hh | KSPRINTF_FMTFLAG_h | KSPRINTF_FMTFLAG_i |
             KSPRINTF_FMTFLAG_l | KSPRINTF_FMTFLAG_ll,
    .specifier = 'o',
    .func = ksprintf_fmt_octal,
};
KSPRINTF_FORMATTER(ksprintf_fmt_octal_st);


static int ksprintf_fmt_hex(KSPRINTF_FMTFUN_ARGS)
{
    str[0] = '0';
    str[1] = 'x';

    switch (value_size) {
    case sizeof(uint64_t):
        if (maxlen < 18)
            return 0;
        return  2 + uitoah64(str + 2, *((uint64_t *)value_p));
    case sizeof(uint32_t):
    default:
        if (maxlen < 10)
            return 0;
        return 2 + uitoah32(str + 2, *((uint32_t *)value_p));
    }
}

static const struct ksprintf_formatter ksprintf_fmt_hex_st = {
    .flags = KSPRINTF_FMTFLAG_hh | KSPRINTF_FMTFLAG_h | KSPRINTF_FMTFLAG_i |
             KSPRINTF_FMTFLAG_l | KSPRINTF_FMTFLAG_ll,
    .specifier = 'x',
    .func = ksprintf_fmt_hex,
};
KSPRINTF_FORMATTER(ksprintf_fmt_hex_st);


static int ksprintf_fmt_paddr(KSPRINTF_FMTFUN_ARGS)
{
    str[0] = 'b';
    return 1 + ksprintf_fmt_hex(str + 1, value_p, value_size, maxlen - 1);
}

static const struct ksprintf_formatter ksprintf_fmt_paddr_st = {
    .flags = KSPRINTF_FMTFLAG_p,
    .specifier = 'p',
    .func = ksprintf_fmt_paddr,
};
KSPRINTF_FORMATTER(ksprintf_fmt_paddr_st);


static int ksprintf_fmt_cstring(KSPRINTF_FMTFUN_ARGS)
{
    int j;
    char * buf = *(char **)value_p;

    for (j = 0; j < maxlen;) {
        if (buf[j] == '\0')
            break;
        str[j] = buf[j];
        j++;
    }

    return j;
}

static const struct ksprintf_formatter ksprintf_fmt_cstring_st = {
    .flags = KSPRINTF_FMTFLAG_p,
    .specifier = 's',
    .func = ksprintf_fmt_cstring,
};
KSPRINTF_FORMATTER(ksprintf_fmt_cstring_st);
