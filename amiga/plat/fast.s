| fast.s - 68020/030 hot paths: block copy, 32x32 tile blit, whole-frame chunky-to-planar with inline change detection.
|   void *memcpy(void *d, const void *s, unsigned n)   (replaces the C version)
|   void  tile_blit(unsigned char *dst, const unsigned char *src, int rows)   32 bytes per row, dst stride 320
|   void  c2p_frame(const unsigned char *chunky, unsigned *shadow, unsigned char *planes)   320x200, plane stride 40, row stride 320
	.text
	.even
	.globl memcpy, tile_blit, c2p_frame
memcpy:
	movem.l	%d2-%d7/%a2/%a3,-(%sp)
	move.l	36(%sp),%a0		| dst
	move.l	40(%sp),%a1		| src
	move.l	44(%sp),%d0		| n
	move.l	%a0,%a3			| return value
.Lblk:	cmp.l	#32,%d0
	bcs.s	.Llongs
	movem.l	(%a1)+,%d1-%d7/%a2
	movem.l	%d1-%d7/%a2,(%a0)
	lea	32(%a0),%a0
	sub.l	#32,%d0
	bra.s	.Lblk
.Llongs: cmp.l	#4,%d0
	bcs.s	.Lbytes
	move.l	(%a1)+,(%a0)+
	subq.l	#4,%d0
	bra.s	.Llongs
.Lbytes: tst.l	%d0
	beq.s	.Ldone
	move.b	(%a1)+,(%a0)+
	subq.l	#1,%d0
	bra.s	.Lbytes
.Ldone:	move.l	%a3,%d0
	movem.l	(%sp)+,%d2-%d7/%a2/%a3
	rts

tile_blit:
	movem.l	%d2-%d7/%a2,-(%sp)
	move.l	32(%sp),%a0		| dst
	move.l	36(%sp),%a1		| src
	move.l	40(%sp),%d0		| rows
	subq.l	#1,%d0
	bmi.s	.Ltb2
.Ltb:	movem.l	(%a1)+,%d1-%d7/%a2
	movem.l	%d1-%d7/%a2,(%a0)
	lea	320(%a0),%a0
	dbra	%d0,.Ltb
.Ltb2:	movem.l	(%sp)+,%d2-%d7/%a2
	rts

| c2p kernel: a0 = src (16 chunky bytes, 4-aligned), a1 = dst (plane 0 word, plane stride 40); clobbers d0-d4
.Lkern:
	move.l 12(%a0),%d0
	move.l 8(%a0),%d1
	move.l 4(%a0),%d2
	move.l (%a0),%d3
	move.l %d0,%d4
	lsr.l #8,%d4
	eor.l %d2,%d4
	and.l #0x00FF00FF,%d4
	eor.l %d4,%d2
	lsl.l #8,%d4
	eor.l %d4,%d0
	move.l %d1,%d4
	lsr.l #8,%d4
	eor.l %d3,%d4
	and.l #0x00FF00FF,%d4
	eor.l %d4,%d3
	lsl.l #8,%d4
	eor.l %d4,%d1
	move.l %d0,%d4
	lsr.l #4,%d4
	eor.l %d1,%d4
	and.l #0x0F0F0F0F,%d4
	eor.l %d4,%d1
	lsl.l #4,%d4
	eor.l %d4,%d0
	move.l %d2,%d4
	lsr.l #4,%d4
	eor.l %d3,%d4
	and.l #0x0F0F0F0F,%d4
	eor.l %d4,%d3
	lsl.l #4,%d4
	eor.l %d4,%d2
	swap %d0
	move.w %d0,%d4
	move.w %d1,%d0
	swap %d0
	move.w %d4,%d1
	swap %d2
	move.w %d2,%d4
	move.w %d3,%d2
	swap %d2
	move.w %d4,%d3
	move.l %d0,%d4
	lsr.l #2,%d4
	eor.l %d1,%d4
	and.l #0x33333333,%d4
	eor.l %d4,%d1
	lsl.l #2,%d4
	eor.l %d4,%d0
	move.l %d2,%d4
	lsr.l #2,%d4
	eor.l %d3,%d4
	and.l #0x33333333,%d4
	eor.l %d4,%d3
	lsl.l #2,%d4
	eor.l %d4,%d2
	move.l %d0,%d4
	lsr.l #1,%d4
	eor.l %d2,%d4
	and.l #0x55555555,%d4
	eor.l %d4,%d2
	lsl.l #1,%d4
	eor.l %d4,%d0
	move.l %d1,%d4
	lsr.l #1,%d4
	eor.l %d3,%d4
	and.l #0x55555555,%d4
	eor.l %d4,%d3
	lsl.l #1,%d4
	eor.l %d4,%d1
	move.w %d0,0(%a1)
	swap %d0
	move.w %d0,160(%a1)
	move.w %d1,80(%a1)
	swap %d1
	move.w %d1,240(%a1)
	move.w %d2,40(%a1)
	swap %d2
	move.w %d2,200(%a1)
	move.w %d3,120(%a1)
	swap %d3
	move.w %d3,280(%a1)
	rts

c2p_frame:
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
	move.l	48(%sp),%a2		| chunky
	move.l	52(%sp),%a3		| shadow
	move.l	56(%sp),%a4		| planes
	move.w	#199,%d7
.Lrow:	move.l	%a4,%a5			| dst row
	moveq	#19,%d6
.Lpair:	move.l	(%a2),%d0
	cmp.l	(%a3),%d0
	bne.s	.Ldirty
	move.l	4(%a2),%d0
	cmp.l	4(%a3),%d0
	bne.s	.Ldirty
	move.l	8(%a2),%d0
	cmp.l	8(%a3),%d0
	bne.s	.Ldirty
	move.l	12(%a2),%d0
	cmp.l	12(%a3),%d0
	beq.s	.Lnext
.Ldirty:
	move.l	(%a2),(%a3)
	move.l	4(%a2),4(%a3)
	move.l	8(%a2),8(%a3)
	move.l	12(%a2),12(%a3)
	move.l	%a2,%a0
	move.l	%a5,%a1
	bsr	.Lkern
.Lnext:	lea	16(%a2),%a2
	lea	16(%a3),%a3
	addq.l	#2,%a5
	dbra	%d6,.Lpair
	lea	320(%a4),%a4
	dbra	%d7,.Lrow
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
	rts
