	section	code,code
	xdef	slicks_convert_to_amiga
	xdef	slicks_chunky_to_amiga
	xdef	slicks_chunky_rows_to_amiga
	xdef	slicks_chunky_rect_to_amiga
	xdef	slicks_chunky_pixels_to_amiga
	xref	c2p1x1_8_c5_bm
	xref	mult320

	include	graphics/gfx.i

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
	lea	mult320,a0
	move.l	(a0,d4.w*4),d4
	movea.l	48(sp),a0
	adda.l	d4,a0
	movea.l	52(sp),a1
	move.w	#320,d0
	moveq	#0,d2
	jsr	c2p1x1_8_c5_bm
.rows_done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Convert one half-open rectangle from the 320-byte-stride chunky surface.
; Horizontal bounds must be aligned to 32 pixels by the caller. C ABI:
; slicks_chunky_rect_to_amiga(chunky, bitmap, left, top, right, bottom,
;                             packed_scratch)
slicks_chunky_rect_to_amiga:
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	56(sp),d5
	move.l	60(sp),d6
	move.l	64(sp),d7
	sub.w	d5,d7
	ble.s	.rect_done
	move.l	68(sp),d4
	sub.w	d6,d4
	ble.s	.rect_done
	moveq	#0,d3
	move.w	d6,d3
	lea	mult320,a4
	move.l	(a4,d3.w*4),d3
	add.w	d5,d3
	movea.l	48(sp),a4
	adda.l	d3,a4
	movea.l	72(sp),a5
	movea.l	a5,a6
	move.w	d4,d3
	subq.w	#1,d3
.pack_row:
	move.w	d7,d2
	lsr.w	#2,d2
	subq.w	#1,d2
.pack_long:
	move.l	(a4)+,(a5)+
	dbf	d2,.pack_long
	move.w	#320,d2
	sub.w	d7,d2
	adda.w	d2,a4
	dbf	d3,.pack_row
	move.w	d7,d0
	move.w	d4,d1
	move.w	d5,d2
	move.w	d6,d3
	movea.l	a6,a0
	movea.l	52(sp),a1
	jsr	c2p1x1_8_c5_bm
.rect_done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Synchronize a list of final chunky pixels directly into all eight planes.
; Each list entry is {word x, byte y, byte padding}.
; C ABI: slicks_chunky_pixels_to_amiga(chunky, bitmap, pixels, count)
slicks_chunky_pixels_to_amiga:
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	60(sp),d7
	beq.w	.pixels_done
	subq.w	#1,d7
	movea.l	56(sp),a6
	movea.l	52(sp),a5
	move.l	bm_Planes+24(a5),d4
	move.l	bm_Planes+28(a5),d5
	; The BitMap facade stores the full interleaved row stride here (320),
	; matching the layout expected by the partial Kalms converter below.
	move.w	bm_BytesPerRow(a5),-(sp)
	move.l	50(sp),-(sp)
	movea.l	bm_Planes+0(a5),a0
	movea.l	bm_Planes+4(a5),a1
	movea.l	bm_Planes+8(a5),a2
	movea.l	bm_Planes+12(a5),a3
	movea.l	bm_Planes+16(a5),a4
	movea.l	bm_Planes+20(a5),a5
.pixel:
	moveq	#0,d0
	move.w	(a6)+,d0
	moveq	#0,d6
	move.b	(a6)+,d6
	addq.l	#1,a6
	move.l	a6,d1
	lea	mult320,a6
	move.l	(a6,d6.w*4),d3
	; The interleaved target uses the same 320-byte row offset for chunky
	; and bitplanes. Retain the old generic-stride behavior as a fallback.
	cmpi.w	#320,4(sp)
	bne.s	.generic_pixel_stride
	move.l	d3,d6
	bra.s	.pixel_stride_ready
.generic_pixel_stride:
	mulu.w	4(sp),d6
.pixel_stride_ready:
	add.l	d0,d3
	add.l	(sp),d3
	movea.l	d3,a6
	moveq	#0,d3
	move.b	(a6),d3
	move.l	d0,d2
	lsr.w	#3,d0
	add.l	d0,d6
	moveq	#7,d0
	and.w	#7,d2
	sub.w	d2,d0
	move.l	d0,d2

	btst	#0,d3
	beq.s	.p0_clear
	bset	d2,0(a0,d6.l)
	bra.s	.p1
.p0_clear:
	bclr	d2,0(a0,d6.l)
.p1:
	btst	#1,d3
	beq.s	.p1_clear
	bset	d2,0(a1,d6.l)
	bra.s	.p2
.p1_clear:
	bclr	d2,0(a1,d6.l)
.p2:
	btst	#2,d3
	beq.s	.p2_clear
	bset	d2,0(a2,d6.l)
	bra.s	.p3
.p2_clear:
	bclr	d2,0(a2,d6.l)
.p3:
	btst	#3,d3
	beq.s	.p3_clear
	bset	d2,0(a3,d6.l)
	bra.s	.p4
.p3_clear:
	bclr	d2,0(a3,d6.l)
.p4:
	btst	#4,d3
	beq.s	.p4_clear
	bset	d2,0(a4,d6.l)
	bra.s	.p5
.p4_clear:
	bclr	d2,0(a4,d6.l)
.p5:
	btst	#5,d3
	beq.s	.p5_clear
	bset	d2,0(a5,d6.l)
	bra.s	.p6
.p5_clear:
	bclr	d2,0(a5,d6.l)
.p6:
	movea.l	d4,a6
	btst	#6,d3
	beq.s	.p6_clear
	bset	d2,0(a6,d6.l)
	bra.s	.p7
.p6_clear:
	bclr	d2,0(a6,d6.l)
.p7:
	movea.l	d5,a6
	btst	#7,d3
	beq.s	.p7_clear
	bset	d2,0(a6,d6.l)
	bra.s	.pixel_next
.p7_clear:
	bclr	d2,0(a6,d6.l)
.pixel_next:
	movea.l	d1,a6
	dbf	d7,.pixel
	addq.l	#6,sp
	bra.w	.pixels_done

.pixels_done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts
