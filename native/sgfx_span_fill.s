	section	code
	xdef	sgfx_span_fill

; Fill a half-open rectangle in the four-plane logical VGA store.
;
; In:  a0 = base of four consecutive 64 KiB planes
;      d0.w = signed x0
;      d1.w = signed y0
;      d2.w = signed x1 (exclusive)
;      d3.w = signed y1 (exclusive)
;      d4.b = byte value
;      d5.w = guest page base
;      d6.w = guest byte stride
; Out: logical pixels in [x0,x1) x [y0,y1) replaced
; Preserves: d0-d7/a0-a6
sgfx_span_fill:
	movem.l	d0-d7/a0-a6,-(sp)

	movea.w	d0,a2			; retain x0 across rows
	movea.w	d3,a3			; retain signed y1
	move.w	d1,d7			; current y
	mulu.w	d6,d1
	add.w	d5,d1			; wrapped row base

.row:
	cmp.w	a3,d7
	bge.s	.done
	move.w	a2,d5			; current x
.pixel:
	cmp.w	d2,d5
	bge.s	.next_row

	moveq	#0,d0
	move.w	d5,d0
	asr.w	#2,d0
	add.w	d1,d0			; wrapped byte offset

	moveq	#0,d3
	move.w	d5,d3
	and.w	#3,d3
	swap	d3			; plane * 65536
	movea.l	a0,a1
	adda.l	d3,a1
	adda.l	d0,a1
	move.b	d4,(a1)

	addq.w	#1,d5
	bra.s	.pixel
.next_row:
	add.w	d6,d1
	addq.w	#1,d7
	bra.s	.row
.done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts
