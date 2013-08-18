/* Runtime ABI for the ARM Cortex-M0  
 * ldivmod.S: signed 64 bit division (only quotient)
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


@ long long __divdi3(long long numerator, long long denominator)
@
@ Divide r1:r0 by r3:r2 and return the quotient in r1:r0.
@ Same as _aeabi_ldivmod(), but ignore remainder.
@
	.thumb_func
        .global __divdi3
__divdi3:

	cmp	r1, #0
	bge	L_num_pos
	
	push	{r4, lr}
	movs	r4, #0			@ num = -num
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4

	cmp	r3, #0
	bge	L_neg_result
	
	movs	r4, #0			@ den = -den
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4
	bl	__aeabi_uldivmod
	pop	{r4, pc}
	
L_num_pos:
	cmp	r3, #0
	bge	__aeabi_uldivmod

	push	{r4, lr}
	movs	r4, #0			@ den = -den
	rsbs	r2, r2, #0
	sbcs	r4, r3
	mov	r3, r4

L_neg_result:
	bl	__aeabi_uldivmod
	movs	r4, #0			@ quot = -quot
	rsbs	r0, r0, #0
	sbcs	r4, r1
	mov	r1, r4
	pop	{r4, pc}
