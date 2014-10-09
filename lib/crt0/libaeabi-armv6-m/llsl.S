/* Runtime ABI for the ARM Cortex-M0  
 * llsl.S: 64 bit shift left
 *
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
	.thumb
	.arch armv6-m


@ long long __ashldi3(long long r1:r0, int r2)
@
@ libgcc wrapper: just an alias for __aeabi_llsl()
@
	.thumb_func
        .global __ashldi3
__ashldi3:



@ long long __aeabi_llsl(long long r1:r0, int r2)
@
@ Sshift r1:r0 left by r2 bits
@
	.thumb_func
        .global __aeabi_llsl
__aeabi_llsl:
	
	cmp	r2, #31
	bhi	1f
	
	movs	r3, r1		@ n < 32:
	lsls	r0, r2		@ lo = lo << n
	lsls	r1, r2
	rsbs	r2, r2, #0
	adds	r2, #32
	lsrs	r3, r2
	orrs	r1, r3		@ hi = hi << n | lo >> (32-n)
	bx	lr

1:	subs	r2, #32		@ n >= 32:
	movs	r1, r0
	lsls	r1, r2		@ hi = lo << (n-32)
	movs	r0, #0		@ lo = 0
	bx	lr
