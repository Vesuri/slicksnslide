; Stable sparse-pixel compaction; identical to the C reference. The union
; bounds reject unrelated pixels before visiting any individual rectangle.
	section	.text,code
	xdef	slicks_race_prune_dirty_pixels
	include	"race_offsets.i"
slicks_race_prune_dirty_pixels:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a4
	moveq	#0,d6
	move.w	RACE_DIRTY_PIXEL_COUNT(a4),d6
	beq.w	.done
	moveq	#0,d7
	move.b	RACE_DIRTY_ROW_COUNT(a4),d7
	beq.w	.done
	lea	RACE_DIRTY_ROWS(a4),a2
	lea	(a2,d7.w*8),a6
	movea.l	a2,a3
	move.w	(a3)+,d2
	move.w	(a3)+,d3
	move.w	(a3)+,d4
	move.w	(a3)+,d5
.bounds:
	cmpa.l	a6,a3
	beq.s	.pixels
	cmp.w	(a3),d2
	bls.s	.top
	move.w	(a3),d2
.top:
	cmp.w	2(a3),d3
	bls.s	.right
	move.w	2(a3),d3
.right:
	cmp.w	4(a3),d4
	bcc.s	.bottom
	move.w	4(a3),d4
.bottom:
	cmp.w	6(a3),d5
	bcc.s	.bounds_next
	move.w	6(a3),d5
.bounds_next:
	addq.l	#8,a3
	bra.s	.bounds
.pixels:
	lea	RACE_DIRTY_PIXELS(a4),a0
	movea.l	a0,a1
	subq.w	#1,d6
.pixel:
	move.l	(a0)+,d0
	movea.l	d0,a5
	move.w	d0,d1
	lsr.w	#8,d1			; y; preserve the unused byte in a5
	swap	d0				; x (unsigned word)
	cmp.w	d2,d0
	bcs.s	.keep
	cmp.w	d4,d0
	bcc.s	.keep
	cmp.w	d3,d1
	bcs.s	.keep
	cmp.w	d5,d1
	bcc.s	.keep
	movea.l	a2,a3
.rect:
	cmp.w	(a3),d0
	bcs.s	.rect_next
	cmp.w	4(a3),d0
	bcc.s	.rect_next
	cmp.w	2(a3),d1
	bcs.s	.rect_next
	cmp.w	6(a3),d1
	bcs.s	.next			; covered: omit from sparse list
.rect_next:
	addq.l	#8,a3
	cmpa.l	a6,a3
	bne.s	.rect
.keep:
	move.l	a5,(a1)+
.next:
	dbf	d6,.pixel
	lea	RACE_DIRTY_PIXELS(a4),a0
	move.l	a1,d0
	sub.l	a0,d0
	lsr.l	#2,d0
	move.w	d0,RACE_DIRTY_PIXEL_COUNT(a4)
.done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts
