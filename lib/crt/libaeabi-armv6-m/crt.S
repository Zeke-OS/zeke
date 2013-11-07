/* Runtime ABI for the ARM Cortex-M0  
 * crt.S: C runtime environment
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


@ _start
@
@ Program entry point: call main(), then exit()
@
	.thumb_func
        .global _start
_start:
	bl main
	@ fallthrough to exit()	


	
@ void exit(int status)
@
@ Exit from program: breakpoint 0
@
	.thumb_func
        .global exit
exit:
	bkpt	#0



@ void abort(void)
@
@ Abnormal program termination: breakpoint 1
@
	.thumb_func
        .global abort
abort:
	bkpt	#1



@ int __aeabi_idiv0(int r)
@
@ Handler for 32 bit division by zero
@
	.thumb_func
        .global __aeabi_idiv0
__aeabi_idiv0:



@ long long __aeabi_ldiv0(long long r)
@
@ Handler for 64 bit division by zero
@
	.thumb_func
        .global __aeabi_ldiv0
__aeabi_ldiv0:
	bx	lr
