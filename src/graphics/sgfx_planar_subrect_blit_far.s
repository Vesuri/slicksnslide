	section	code
	xdef	sgfx_planar_subrect_blit_far

; Compatibility form of 2b8de for a complete 64 KiB source segment.
; Same registers as sgfx_planar_subrect_blit, plus a6 = source segment base.
; a1 points at the two-byte sprite header within that segment (offset <= fffe).
; Source offsets wrap after each byte/row/plane, as the original SI does.
; This cold path does not add wrapping overhead to bounded production crops.
sgfx_planar_subrect_blit_far:
	move.l	a6,-(sp)
	andi.l	#$ffff,d2
	lsr.w	#2,d2
	move.l	d2,a4
	addq.w	#3,d5
	lsr.w	#2,d5
	andi.l	#$ffff,d5
	move.l	d5,a3
	tst.w	d5
	beq.w	.done
	andi.l	#$ff,d6
	beq.w	.done
	moveq	#0,d2
	move.w	d0,d2
	andi.w	#3,d2
	ori.l	#$00040000,d2
	add.w	d3,d1
	mulu.w	d4,d1
	lsr.w	#2,d0
	add.w	d0,d1
	move.l	a4,d5
	add.w	d5,d1
	add.w	d7,d1
	moveq	#0,d0
	move.w	d1,d0
	moveq	#0,d1
	move.w	d4,d1
	move.l	a3,d5
	sub.w	d5,d1
	moveq	#0,d5
	move.b	(a1)+,d5
	move.l	d5,a2
	moveq	#0,d7
	move.b	(a1)+,d7
	sub.b	d6,d7
	mulu.w	d5,d7
	move.l	d7,a6
	move.l	d5,d7
	move.l	a3,d5
	sub.w	d5,d7
	move.l	a4,d5
	sub.w	d5,d7
	andi.l	#$ffff,d7
	move.l	d7,a5
	suba.l	(sp),a1
	move.l	a2,d5
	mulu.w	d5,d3
	adda.l	d3,a1
	move.l	d6,a2
.plane:
	moveq	#0,d7
	move.w	d2,d7
	andi.w	#3,d7
	swap	d7
	moveq	#0,d6
	move.w	d2,d6
	lsr.w	#2,d6
	add.w	d0,d6
	move.w	d6,d7
	move.l	a2,d5
	subq.w	#1,d5
.row:
	adda.l	a4,a1
	move.l	a3,d6
	subq.w	#1,d6
.byte:
	move.l	a1,d3
	andi.l	#$ffff,d3
	add.l	(sp),d3
	move.l	d3,a1
	move.b	(a1)+,d3
	suba.l	(sp),a1
	move.b	d3,(a0,d7.l)
	addq.w	#1,d7
	dbf	d6,.byte
	add.w	d1,d7
	adda.l	a5,a1
	dbf	d5,.row
	adda.l	a6,a1
	addq.w	#1,d2
	swap	d2
	subq.w	#1,d2
	beq.s	.done
	swap	d2
	bra.s	.plane
.done:
	addq.l	#4,sp
	rts
