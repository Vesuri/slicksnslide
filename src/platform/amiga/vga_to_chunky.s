	section	code,code
	xdef	slicks_convert_to_amiga
	xdef	slicks_chunky_to_amiga
	xdef	slicks_chunky_rows_to_amiga
	xref	c2p1x1_8_c5_bm

; Convert the visible 320x200 pixels from the original four-bank VGA
; byte layout into a conventional chunky buffer, then let Kalms' CPU5
; converter write the eight Amiga BitMap planes.
;
; C ABI:
;   slicks_convert_to_amiga(logical, chunky, bitmap)
slicks_convert_to_amiga:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1

	movea.l	a0,a2
	adda.l	#$10000,a2
	movea.l	a2,a3
	adda.l	#$10000,a3
	movea.l	a3,a4
	adda.l	#$10000,a4

	move.w	#199,d4
.row:
	move.w	#79,d1
.group:
	moveq	#0,d0
	move.b	(a0)+,d0
	lsl.l	#8,d0
	move.b	(a2)+,d0
	lsl.l	#8,d0
	move.b	(a3)+,d0
	lsl.l	#8,d0
	move.b	(a4)+,d0
	move.l	d0,(a1)+
	dbf	d1,.group

	lea	20(a0),a0
	lea	20(a2),a2
	lea	20(a3),a3
	lea	20(a4),a4
	dbf	d4,.row

	move.w	#320,d0
	move.w	#200,d1
	moveq	#0,d2
	moveq	#0,d3
	movea.l	52(sp),a0
	movea.l	56(sp),a1
	jsr	c2p1x1_8_c5_bm

	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Convert an already synchronized 320x200 chunky surface.  The live race
; updates both its VGA-compatible logical store and this surface per pixel,
; avoiding a redundant full-screen VGA deinterleave every frame.
; C ABI: slicks_chunky_to_amiga(chunky, bitmap)
slicks_chunky_to_amiga:
	movem.l	d2-d7/a2-a6,-(sp)
	move.w	#320,d0
	move.w	#200,d1
	moveq	#0,d2
	moveq	#0,d3
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	jsr	c2p1x1_8_c5_bm
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Convert one half-open full-width row interval from an already synchronized
; chunky surface. C ABI arguments are chunky, bitmap, top, bottom; bounds are
; passed as 32-bit unsigned longs to keep their stack layout unambiguous.
slicks_chunky_rows_to_amiga:
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	56(sp),d3
	move.l	60(sp),d1
	sub.w	d3,d1
	ble.s	.rows_done
	moveq	#0,d4
	move.w	d3,d4
	mulu.w	#320,d4
	movea.l	48(sp),a0
	adda.l	d4,a0
	movea.l	52(sp),a1
	move.w	#320,d0
	moveq	#0,d2
	jsr	c2p1x1_8_c5_bm
.rows_done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts
