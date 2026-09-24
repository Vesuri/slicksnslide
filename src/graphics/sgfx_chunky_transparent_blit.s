	section	code
	xdef	sgfx_chunky_transparent_blit
	xdef	slicks_draw_chunky_icon
	xref	mult320

; 3aa7c transparent blit, with VGA exits lowered to authoritative chunky
; stores. Source is the decoded, unpadded row-major icon, not a screen dump.
; a0=320-wide surface, a1=pixels; d0.w=x, d1.w=y, d2.w=width, d3.w=height.
; Caller validates bounds; zero-sized sprites are no-ops. All registers kept.
sgfx_chunky_transparent_blit:
	movem.l	d0-d5/a0-a2,-(sp)
	tst.w	d2
	beq.s	.done
	tst.w	d3
	beq.s	.done
	moveq	#0,d4
	move.w	d1,d4
	lsl.w	#2,d4
	lea	mult320,a2
	adda.l	(a2,d4.w),a0
	moveq	#0,d4
	move.w	d0,d4
	adda.l	d4,a0
	subq.w	#1,d3
.row:
	movea.l	a0,a2
	move.w	d2,d4
	subq.w	#1,d4
.pixel:
	move.b	(a1)+,d5
	beq.s	.transparent
	move.b	d5,(a2)
.transparent:
	addq.l	#1,a2
	dbf	d4,.pixel
	adda.w	#320,a0
	dbf	d3,.row
.done:
	movem.l	(sp)+,d0-d5/a0-a2
	rts

; GCC bridge: (surface, pixels, x, y, width, height), 32-bit argument slots.
slicks_draw_chunky_icon:
	movem.l	d2-d3,-(sp)
	movea.l	12(sp),a0
	movea.l	16(sp),a1
	move.w	22(sp),d0
	move.w	26(sp),d1
	move.w	30(sp),d2
	move.w	34(sp),d3
	bsr	sgfx_chunky_transparent_blit
	movem.l	(sp)+,d2-d3
	rts
