/* Runtime ABI for the ARM Cortex-M0  
 * memset.S: set memory region
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


@ void __aeabi_memclr(void *r0, size_t r1)
@
@ Set the r1 bytes beginning with *r0 to 0.
@
    .global __aeabi_memclr
__aeabi_memclr:


	eors	r2, r2		@ fallthrough to memset


@ void __aeabi_memset(void *r0, size_t r1, int r2)
@
@ Set the r1 bytes beginning with *r0 to r2
@
    .global __aeabi_memset
__aeabi_memset:

	@ check if length=0
	cmp	r1, #0
	beq	L_return1
	
	movs	r3, #1		@ set one byte if odd address
	tst	r0, r3
	beq	L_align2
	strb	r2, [r0]
	adds	r0, #1
	subs	r1, #1
	beq	L_return1
	
L_align2:
	movs	r3, #2		@ set one halfword if address is not 32 bit aligned
	tst	r0, r3
	beq	__aeabi_memset4
	strb	r2, [r0]
	cmp	r1, #1		@ if length is 1 copy only 1 byte
	beq	L_return1
	strb	r2, [r0, #1]
	adds	r0, #2
	subs	r1, #2
	bne	__aeabi_memset4

L_return1:
	bx	lr


@ void __aeabi_memclr4(void *r0, size_t r1)
@
@ Set the r1 bytes beginning with *r0 to 0.
@ r0 must be 4-byte-aligned
@
    .global __aeabi_memclr4
__aeabi_memclr4:


@ void __aeabi_memclr8(void *r0, size_t r1)
@
@ Set the r1 bytes beginning with *r0 to 0.
@ r0 must be 8-byte-aligned
@
    .global __aeabi_memclr8
__aeabi_memclr8:


	eors	r2, r2		@ fallthrough to memset4


@ void __aeabi_memset4(void *r0, size_t r1, int r2)
@
@ Set the r1 bytes beginning with *r0 to r2.
@ r0 must be 4-byte-aligned
@
    .global __aeabi_memset4
__aeabi_memset4:


@ void __aeabi_memset8(void *r0, size_t r1, int r2)
@
@ Set the r1 bytes beginning with *r0 to r2.
@ r0 must be 8-byte-aligned
@
    .global __aeabi_memset8
__aeabi_memset8:


	subs	r1, #4
	blo	L_last_3bytes
	
	lsls	r2, r2, #24	@ copy lowest byte of r2 to all other bytes in r2
	lsrs	r3, r2, #8
	orrs	r2, r3
	lsrs	r3, r2, #16
	orrs	r2, r3
	
L_loop:
	str	r2, [r0]
	adds	r0, #4
	subs	r1, #4
	bhs	L_loop
	
L_last_3bytes:			@ r1 = remaining len - 4
	adds	r1, #2
	blo	L_one_left	@ branch if r1 was -4 or -3
	strh	r2, [r0]
	beq	L_return2	@ finished if r1 was -2
	strb	r2, [r0, #2]

L_return2:
	bx	lr

L_one_left:
	adds	r1, #1
	bne	L_return3
	strb	r2, [r0]

L_return3:
	bx	lr
