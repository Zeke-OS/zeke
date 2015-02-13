/* Runtime ABI for the ARMv6k
 * moddi3.S: signed 64 bit division (only remainder)
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
    .arch armv6k


@ long long __moddi3(long long numerator, long long denominator)
@
@ Divide r1:r0 by r3:r2 and return the remainder in r1:r0 (all signed)
@ Special case of __aeabi_ldivmod() that is a lot simpler and moves the
@ remainder to r1:r0.
@
    .global __moddi3
__moddi3:

	push	{r4, lr}
	cmp	r3, #0
	bge	L_dont_neg_den
	
	movs	r4, #0			@ den = -den
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
L_dont_neg_den:

	cmp	r1, #0
	bge	L_pos_result
	
	movs	r4, #0			@ num = -num
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4

	bl	__aeabi_uldivmod
	movs	r0, #0			@ rem = -rem
	movs	r1, #0
	subs	r0, r2
	sbcs	r1, r3
	pop	{r4, pc}

L_pos_result:
	bl	__aeabi_uldivmod
	mov	r1, r3
	mov	r0, r2
	pop	{r4, pc}
