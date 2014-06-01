	.syntax unified
	.eabi_attribute 6, 6
	.eabi_attribute 10, 2
	.fpu vfpv2
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.file	"soft_fp.c"
	.text
	.globl	__aeabi_i2d
	.align	2
	.type	__aeabi_i2d,%function
__aeabi_i2d:
	push	{r4, lr}
	mov	r1, r0
	rsbs	r2, r1, #0
	mov	r0, #0
	sbc	r12, r0, r1, asr #31
	cmp	r1, #0
	mov	r3, #-2147483648
	movge	r2, r1
	asrge	r12, r1, #31
	and	r1, r3, r1, asr #31
	orrs	r3, r2, r12
	beq	.LBB0_2
	clz	r3, r2
	rsb	r4, r3, #32
	lsl	lr, r2, r3
	lsr	r4, r2, r4
	orr	r12, r4, r12, lsl r3
	mov	r4, #1
	and	lr, r4, lr, lsr #11
	sub	r4, r3, #32
	cmp	r4, #0
	lslge	r12, r2, r4
	adds	r2, lr, r2, lsl r3
	bic	r4, r12, #-2147483648
	adc	r12, r4, #0
	mov	r4, #62
	orr	r4, r4, #1024
	sub	r3, r4, r3
	mov	r4, #255
	orr	r4, r4, #768
	adds	r2, r2, r4
	orr	r1, r1, r3, lsl #20
	adc	r3, r12, #0
	lsr	r2, r2, #11
	orr	r2, r2, r3, lsl #21
	orr	r1, r1, r3, lsr #11
	orr	r0, r0, r2
.LBB0_2:
	pop	{r4, pc}
.Ltmp0:
	.size	__aeabi_i2d, .Ltmp0-__aeabi_i2d

	.globl	__aeabi_l2d
	.align	2
	.type	__aeabi_l2d,%function
__aeabi_l2d:
	push	{r4, lr}
	adds	r0, r0, r1, asr #31
	adc	r3, r1, r1, asr #31
	eor	r2, r0, r1, asr #31
	eor	r12, r3, r1, asr #31
	orrs	r0, r2, r12
	and	r1, r1, #-2147483648
	mov	r0, #0
	beq	.LBB1_2
	clz	r3, r2
	rsb	r4, r3, #32
	lsl	lr, r2, r3
	lsr	r4, r2, r4
	orr	r12, r4, r12, lsl r3
	mov	r4, #1
	and	lr, r4, lr, lsr #11
	sub	r4, r3, #32
	cmp	r4, #0
	lslge	r12, r2, r4
	adds	r2, lr, r2, lsl r3
	bic	r4, r12, #-2147483648
	adc	r12, r4, #0
	mov	r4, #62
	orr	r4, r4, #1024
	sub	r3, r4, r3
	mov	r4, #255
	orr	r4, r4, #768
	adds	r2, r2, r4
	orr	r1, r1, r3, lsl #20
	adc	r3, r12, #0
	lsr	r2, r2, #11
	orr	r2, r2, r3, lsl #21
	orr	r1, r1, r3, lsr #11
	orr	r0, r0, r2
.LBB1_2:
	pop	{r4, pc}
.Ltmp1:
	.size	__aeabi_l2d, .Ltmp1-__aeabi_l2d

	.globl	__aeabi_ui2d
	.align	3
	.type	__aeabi_ui2d,%function
__aeabi_ui2d:
	cmp	r0, #0
	vldreq	d0, .LCPI2_0
	vmoveq	r0, r1, d0
	bxeq	lr
	clz	r1, r0
	rsb	r2, r1, #32
	lsl	r12, r0, r1
	mov	r3, #1
	and	r12, r3, r12, lsr #11
	sub	r3, r1, #32
	lsr	r2, r0, r2
	cmp	r3, #0
	lslge	r2, r0, r3
	mov	r3, #255
	adds	r0, r12, r0, lsl r1
	bic	r2, r2, #-2147483648
	orr	r3, r3, #768
	adc	r2, r2, #0
	adds	r0, r0, r3
	lsr	r3, r0, #11
	adc	r2, r2, #0
	lsr	r0, r2, #11
	orr	r2, r3, r2, lsl #21
	mov	r3, #62
	orr	r3, r3, #1024
	sub	r1, r3, r1
	orr	r0, r0, r1, lsl #20
	vmov	d0, r2, r0
	vmov	r0, r1, d0
	bx	lr
	.align	3
.LCPI2_0:
	.long	0
	.long	0
.Ltmp2:
	.size	__aeabi_ui2d, .Ltmp2-__aeabi_ui2d

	.globl	__aeabi_ul2d
	.align	2
	.type	__aeabi_ul2d,%function
__aeabi_ul2d:
	push	{r4, lr}
	adds	r0, r0, r1, asr #31
	adc	r3, r1, r1, asr #31
	eor	r2, r0, r1, asr #31
	eor	r12, r3, r1, asr #31
	orrs	r0, r2, r12
	and	r1, r1, #-2147483648
	mov	r0, #0
	beq	.LBB3_2
	clz	r3, r2
	rsb	r4, r3, #32
	lsl	lr, r2, r3
	lsr	r4, r2, r4
	orr	r12, r4, r12, lsl r3
	mov	r4, #1
	and	lr, r4, lr, lsr #11
	sub	r4, r3, #32
	cmp	r4, #0
	lslge	r12, r2, r4
	adds	r2, lr, r2, lsl r3
	bic	r4, r12, #-2147483648
	adc	r12, r4, #0
	mov	r4, #62
	orr	r4, r4, #1024
	sub	r3, r4, r3
	mov	r4, #255
	orr	r4, r4, #768
	adds	r2, r2, r4
	orr	r1, r1, r3, lsl #20
	adc	r3, r12, #0
	lsr	r2, r2, #11
	orr	r2, r2, r3, lsl #21
	orr	r1, r1, r3, lsr #11
	orr	r0, r0, r2
.LBB3_2:
	pop	{r4, pc}
.Ltmp3:
	.size	__aeabi_ul2d, .Ltmp3-__aeabi_ul2d

	.globl	__aeabi_i2f
	.align	2
	.type	__aeabi_i2f,%function
__aeabi_i2f:
	push	{r11, lr}
	rsbs	r1, r0, #0
	mov	r2, #0
	sbc	r12, r2, r0, asr #31
	cmp	r0, #0
	asrge	r12, r0, #31
	movge	r1, r0
	and	r0, r0, #-2147483648
	orrs	r2, r1, r12
	beq	.LBB4_2
	clz	lr, r1
	rsb	r3, lr, #32
	mov	r2, #1
	lsr	r3, r1, r3
	orr	r3, r3, r12, lsl lr
	sub	r12, lr, #32
	cmp	r12, #0
	lslge	r3, r1, r12
	and	r2, r2, r3, lsr #8
	adds	r1, r2, r1, lsl lr
	bic	r2, r3, #-2147483648
	adc	r3, r2, #0
	subs	r1, r1, #1
	sbc	r1, r3, #0
	asr	r1, r1, #8
	rsb	r2, lr, #190
	add	r1, r1, r2, lsl #23
	orr	r0, r1, r0
.LBB4_2:
	pop	{r11, pc}
.Ltmp4:
	.size	__aeabi_i2f, .Ltmp4-__aeabi_i2f

	.globl	__aeabi_l2f
	.align	2
	.type	__aeabi_l2f,%function
__aeabi_l2f:
	push	{r11, lr}
	adds	r0, r0, r1, asr #31
	adc	r3, r1, r1, asr #31
	eor	r2, r0, r1, asr #31
	eor	r12, r3, r1, asr #31
	orrs	r0, r2, r12
	and	r0, r1, #-2147483648
	beq	.LBB5_2
	clz	lr, r2
	rsb	r3, lr, #32
	mov	r1, #1
	lsr	r3, r2, r3
	orr	r3, r3, r12, lsl lr
	sub	r12, lr, #32
	cmp	r12, #0
	lslge	r3, r2, r12
	and	r1, r1, r3, lsr #8
	adds	r2, r1, r2, lsl lr
	bic	r1, r3, #-2147483648
	adc	r3, r1, #0
	subs	r1, r2, #1
	sbc	r1, r3, #0
	asr	r2, r1, #8
	rsb	r1, lr, #190
	add	r1, r2, r1, lsl #23
	orr	r0, r1, r0
.LBB5_2:
	pop	{r11, pc}
.Ltmp5:
	.size	__aeabi_l2f, .Ltmp5-__aeabi_l2f

	.globl	__aeabi_ui2f
	.align	2
	.type	__aeabi_ui2f,%function
__aeabi_ui2f:
	cmp	r0, #0
	vldreq	s0, .LCPI6_0
	vmoveq	r0, s0
	bxeq	lr
	clz	r1, r0
	rsb	r2, r1, #32
	sub	r3, r1, #32
	lsr	r2, r0, r2
	cmp	r3, #0
	lslge	r2, r0, r3
	mov	r3, #1
	and	r3, r3, r2, lsr #8
	adds	r0, r3, r0, lsl r1
	bic	r2, r2, #-2147483648
	adc	r2, r2, #0
	subs	r0, r0, #1
	sbc	r0, r2, #0
	asr	r0, r0, #8
	rsb	r1, r1, #190
	add	r0, r0, r1, lsl #23
	vmov	s0, r0
	vmov	r0, s0
	bx	lr
	.align	2
.LCPI6_0:
	.long	0
.Ltmp6:
	.size	__aeabi_ui2f, .Ltmp6-__aeabi_ui2f


