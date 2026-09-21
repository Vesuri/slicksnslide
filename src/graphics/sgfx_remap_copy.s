	section	code
	xdef	sgfx_remap_copy

; Apply a 256-byte colour translation table to a half-open rectangle in the
; four-plane logical VGA store.  This replaces runtime 24499h..24553h.
;
; In:  a0 = base of four consecutive 64 KiB planes
;      a1 = 256-byte translation table
;      d0.w = nonnegative x0, d1.w = nonnegative y0
;      d2.w = x1 (exclusive), d3.w = y1 (exclusive)
;      d4.w = guest byte stride, d5.w = guest page base
; Out: every logical pixel in [x0,x1) x [y0,y1) is table-remapped
; Preserves: d0-d7/a0-a6
sgfx_remap_copy:
	movem.l	d0-d7/a0-a6,-(sp)

	movea.w	d3,a2			; retain y1 while d3 forms addresses
	move.w	d1,d6			; current y
.row:
	cmp.w	a2,d6
	bge.s	.done
	move.w	d0,d7			; current x
.pixel:
	cmp.w	d2,d7
	bge.s	.next_row

	moveq	#0,d1
	move.w	d6,d1
	mulu.w	d4,d1
	moveq	#0,d3
	move.w	d7,d3
	lsr.w	#2,d3
	add.w	d3,d1
	add.w	d5,d1
	andi.l	#$ffff,d1		; guest byte offset wraps at 16 bits

	moveq	#0,d3
	move.w	d7,d3
	andi.w	#3,d3
	swap	d3			; plane * 65536
	add.l	d1,d3

	moveq	#0,d1
	move.b	(a0,d3.l),d1
	move.b	(a1,d1.w),(a0,d3.l)
	addq.w	#1,d7
	bra.s	.pixel

.next_row:
	addq.w	#1,d6
	bra.s	.row
.done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts
