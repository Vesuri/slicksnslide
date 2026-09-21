	section	code
	xdef	sutil_fill_bytes

; Native byte-fill semantics of the Borland far-memory helper at runtime
; offset 00D9Fh.  VGA and text-memory destinations remain caller-level
; platform decisions; this routine covers the ordinary bounded byte store.
;
; In: a0 = destination
;     d0.w = byte count
;     d1.b = repeated value
; Preserves: d0-d7/a0-a6
sutil_fill_bytes:
	movem.l	d0-d7/a0-a6,-(sp)
	move.w	d0,d4
	beq.s	.done

	moveq	#0,d2
	move.b	d1,d2
	move.w	d2,d3
	lsl.w	#8,d3
	or.w	d3,d2
	move.w	d2,d3
	swap	d2
	move.w	d3,d2

	move.l	a0,d3
	btst	#0,d3
	beq.s	.aligned
	move.b	d1,(a0)+
	subq.w	#1,d4
	beq.s	.done

.aligned:
	move.w	d4,d5
	andi.w	#3,d4
	lsr.w	#2,d5
	beq.s	.tail
	subq.w	#1,d5
.longs:
	move.l	d2,(a0)+
	dbra	d5,.longs

.tail:
	tst.w	d4
	beq.s	.done
	subq.w	#1,d4
.bytes:
	move.b	d1,(a0)+
	dbra	d4,.bytes

.done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts
