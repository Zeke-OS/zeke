/* Runtime ABI for ARM
 * llsr.S: 64 bit logical shift right
 *
 * Copyright (c) 2013, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012 Jörg Mische <bobbl@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

    .syntax unified
    .text


@ long long __lshrdi3(long long r1:r0, int r2)
@
@ libgcc wrapper: just an alias for __aeabi_llsr()
@
    .global __lshrdi3
__lshrdi3:



@ long long __aeabi_llsr(long long r1:r0, int r2)
@
@ Logical shift r1:r0 right by r2 bits
@
    .global __aeabi_llsr
__aeabi_llsr:

    cmp r2, #31
    bhi 1f

    movs    r3, r1      @ n < 32:
    lsrs    r0, r2
    lsrs    r1, r2      @ hi = hi >> n
    rsbs    r2, r2, #0
    adds    r2, #32
    lsls    r3, r2
    orrs    r0, r3      @ lo = lo >> n | hi << (32-n)
    bx  lr

1:  subs    r2, #32     @ n >= 32:
    movs    r0, r1
    lsrs    r0, r2      @ lo = hi >> (n-32)
    movs    r1, #0      @ hi = 0
    bx  lr
