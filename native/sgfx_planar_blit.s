	section	code
	xdef	sgfx_planar_blit

; Opaque four-plane sprite copy.
;
; In:  a0 = four consecutive 64 KiB destination planes
;      a1 = sprite [byte width, height, four consecutive plane payloads]
;      d0.w = x, d1.w = y, d3.w = screen base, d4.w = byte stride
; Out: destination rectangle replaced
; Clobbers: d0-d3/d5-d7, a1-a5/cc.  d4 and a0 are preserved.
; A zero width or height is outside the proved source contract and is ignored.
sgfx_planar_blit:
	moveq	#0,d5
	move.b	(a1)+,d5
	moveq	#0,d6
	move.b	(a1)+,d6
	tst.w	d5
	beq.s	.done
	tst.w	d6
	beq.s	.done

	moveq	#0,d7
	move.w	d1,d7
	mulu.w	d4,d7
	moveq	#0,d2
	move.w	d0,d2
	lsr.w	#2,d2
	add.w	d2,d7
	add.w	d3,d7
	andi.l	#$ffff,d7
	move.l	d7,a5

	moveq	#0,d3
	move.w	d0,d3
	andi.w	#3,d3
	moveq	#4,d2
	move.l	d2,a4
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

	move.w	d6,d1
	subq.w	#1,d1
.row:
	move.w	d5,d2
	subq.w	#1,d2
.byte:
	moveq	#0,d7
	move.w	d0,d7
	lea	(a2,d7.l),a3
	move.b	(a1)+,d7
	move.b	d7,(a3)
	addq.w	#1,d0
	dbf	d2,.byte
	add.w	d4,d0
	sub.w	d5,d0
	dbf	d1,.row

	addq.w	#1,d3
	subq.l	#1,a4
	move.l	a4,d2
	bne.s	.plane
.done:
	rts
