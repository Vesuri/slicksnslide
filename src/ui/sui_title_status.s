	section	code
	xdef	sui_title_status
	xref	sgfx_transparent_blit
	xref	sui_draw_text
	xref	slicks_status_negative
	xref	slicks_status_positive

; Native translation of the observed post-menu title status slice at
; 19828h..199f9h.  The bounded BASIC path contains four active status slots
; (-,+,+,+), two counters whose measured value is one, and no optional badges.
;
; In: a0 = logical planes, d7.w = guest page base
; Preserves: d0-d7/a0-a6
sui_title_status:
	movem.l	d0-d7/a0-a6,-(sp)
	suba.w	#2,sp
	movea.l	sp,a6
	move.w	d7,(a6)
	moveq	#100,d4

	lea	slicks_status_negative,a1
	move.w	#205,d0
	moveq	#99,d1
	move.w	(a6),d3
	jsr	sgfx_transparent_blit

	lea	slicks_status_positive,a1
	move.w	#213,d0
	moveq	#100,d1
	move.w	(a6),d3
	jsr	sgfx_transparent_blit

	lea	slicks_status_positive,a1
	move.w	#221,d0
	moveq	#99,d1
	move.w	(a6),d3
	jsr	sgfx_transparent_blit

	lea	slicks_status_positive,a1
	move.w	#229,d0
	moveq	#100,d1
	move.w	(a6),d3
	jsr	sgfx_transparent_blit

	; Original style 6 centres the first value at x=219.  Style 4 starts the
	; second at x=222; the compact renderer is centred, so x=224 gives that
	; five-pixel left edge.
	lea	.value_one,a1
	move.w	#219,d0
	moveq	#112,d1
	moveq	#0,d2
	move.b	#$b9,d2
	moveq	#0,d3
	move.w	(a6),d3
	jsr	sui_draw_text

	lea	.value_one,a1
	move.w	#224,d0
	moveq	#114,d1
	moveq	#0,d2
	move.b	#$b9,d2
	moveq	#0,d3
	move.w	(a6),d3
	jsr	sui_draw_text

	adda.w	#2,sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

	section	data,data
.value_one:	dc.b	"1",0
	even
