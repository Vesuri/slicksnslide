	section	code
	xdef	sgfx_planar_subrect_blit

; Copy a rectangular byte-aligned portion of a planar sprite.
;
; In:  a0 = four consecutive 64 KiB destination planes
;      a1 = sprite [byte width, height, four consecutive plane payloads]
;      d0.w = destination x, d1.w = destination y
;      d2.w = source x, d3.w = source y
;      d4.w = destination byte stride
;      d5.w = pixel width, d6.b = height, d7.w = screen base
; Out: destination rectangle replaced
; Clobbers: d0-d3/d5-d7, a1-a6/cc.  d4 and a0 are preserved.
; Source bounds and nonzero dimensions are caller contracts.
sgfx_planar_subrect_blit:
	andi.l	#$ffff,d2
	lsr.w	#2,d2
	move.l	d2,a4		; source x in bytes

	addq.w	#3,d5
	lsr.w	#2,d5
	andi.l	#$ffff,d5
	move.l	d5,a3		; copied bytes per row
	tst.w	d5
	beq.w	.done
	andi.l	#$ff,d6
	beq.w	.done

	moveq	#0,d2
	move.w	d0,d2
	andi.w	#3,d2
	ori.l	#$00040000,d2	; phase in low word, plane count in high

	add.w	d3,d1
	mulu.w	d4,d1
	lsr.w	#2,d0
	add.w	d0,d1
	move.l	a4,d5
	add.w	d5,d1
	add.w	d7,d1
	moveq	#0,d0
	move.w	d1,d0		; wrapped destination base offset

	moveq	#0,d1
	move.w	d4,d1
	move.l	a3,d5
	sub.w	d5,d1		; destination row skip

	moveq	#0,d5
	move.b	(a1)+,d5		; source byte width
	move.l	d5,a2
	moveq	#0,d7
	move.b	(a1)+,d7		; source height
	sub.w	d6,d7
	mulu.w	d5,d7
	move.l	d7,a6		; tail from crop bottom to next plane

	move.l	d5,d7
	move.l	a3,d5
	sub.w	d5,d7
	move.l	a4,d5
	sub.w	d5,d7
	andi.l	#$ffff,d7
	move.l	d7,a5		; source row skip

	move.l	a2,d5
	mulu.w	d5,d3
	adda.l	d3,a1		; source-y offset into first plane
	move.l	d6,a2		; stable row count

.plane:
	moveq	#0,d7
	move.w	d2,d7
	andi.w	#3,d7
	swap	d7
	moveq	#0,d6
	move.w	d2,d6
	lsr.w	#2,d6
	add.w	d0,d6
	move.w	d6,d7		; plane in high word, offset in low

	move.l	a2,d5
	subq.w	#1,d5
.row:
	adda.l	a4,a1
	move.l	a3,d6
	subq.w	#1,d6
.byte:
	move.b	(a1)+,d3
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
	rts
