	section	code
	xdef	sgfx_readback

; Copy a pixel-width rectangle from the logical VGA planes into sprite format.
;
; In:  a0 = four consecutive 64 KiB source planes
;      a1 = destination buffer
;      d0.w = x, d1.w = y, d2.w = pixel width
;      d3.w = screen base, d4.w = byte stride, d5.b = height
; Out: [byte ceil(width/4), byte height, four plane payloads, byte padding]
;      where padding is (-width) & 3
; Clobbers: d0-d3/d5-d7, a1-a5/cc.  d4 and a0 are preserved.
; A zero height is outside the proved source contract and is ignored.
sgfx_readback:
	moveq	#0,d6
	move.w	d2,d6
	neg.w	d6
	andi.w	#3,d6
	move.l	d6,a4

	addq.w	#3,d2
	lsr.w	#2,d2
	move.b	d2,(a1)+
	move.b	d5,(a1)+
	andi.l	#$ff,d5
	beq.s	.done
	ori.l	#$00040000,d5

	moveq	#0,d7
	move.w	d1,d7
	mulu.w	d4,d7
	moveq	#0,d6
	move.w	d0,d6
	lsr.w	#2,d6
	add.w	d6,d7
	add.w	d3,d7
	andi.l	#$ffff,d7
	move.l	d7,a5

	moveq	#0,d3
	move.w	d0,d3
	andi.w	#3,d3
.plane:
	move.l	a5,d0
	move.w	d3,d1
	lsr.w	#2,d1
	add.w	d1,d0
	moveq	#0,d1
	move.w	d3,d1
	andi.w	#3,d1
	swap	d1
	lea	(a0,d1.l),a2

	moveq	#0,d1
	move.b	d5,d1
	subq.w	#1,d1
.row:
	tst.w	d2
	beq.s	.row_advance
	move.w	d2,d6
	subq.w	#1,d6
.byte:
	moveq	#0,d7
	move.w	d0,d7
	lea	(a2,d7.l),a3
	move.b	(a3),(a1)+
	addq.w	#1,d0
	dbf	d6,.byte
.row_advance:
	add.w	d4,d0
	sub.w	d2,d0
	dbf	d1,.row

	addq.w	#1,d3
	swap	d5
	subq.w	#1,d5
	beq.s	.finish
	swap	d5
	bra.s	.plane
.finish:
	move.l	a4,d7
	move.b	d7,(a1)+
.done:
	rts
