	section	code
	xdef	sui_draw_text
	xref	sgfx_plot_plane

; Compact native 5x7 text renderer for the title-menu vocabulary.
;
; In: a0 = logical planes, a1 = zero-terminated ASCII
;     d0.w = horizontal centre, d1.w = top y, d2.b = colour, d3.w = page
; Preserves: d0-d7/a0-a6
sui_draw_text:
	movem.l	d0-d7/a0-a6,-(sp)
	movea.l	a1,a3
	movea.w	d1,a5
	moveq	#0,d4
.length:
	tst.b	(a1)+
	beq.s	.length_done
	addq.w	#1,d4
	bra.s	.length
.length_done:
	moveq	#6,d6
	mulu.w	d6,d4
	subq.w	#1,d4
	lsr.w	#1,d4
	sub.w	d4,d0
	movea.w	d0,a2
	moveq	#100,d4

.character:
	moveq	#0,d0
	move.b	(a3)+,d0
	beq.s	.done
	cmpi.b	#' ',d0
	beq.s	.advance
	lea	.font,a4
.find:
	tst.b	(a4)
	beq.s	.advance
	cmp.b	(a4),d0
	beq.s	.glyph
	adda.w	#8,a4
	bra.s	.find
.glyph:
	moveq	#0,d7
.row:
	moveq	#0,d5
	move.b	1(a4,d7.w),d5
	moveq	#0,d6
.column:
	moveq	#4,d0
	sub.w	d6,d0
	btst	d0,d5
	beq.s	.next_column
	move.w	a2,d0
	add.w	d6,d0
	move.w	a5,d1
	add.w	d7,d1
	movem.l	d5-d6,-(sp)
	jsr	sgfx_plot_plane
	movem.l	(sp)+,d5-d6
.next_column:
	addq.w	#1,d6
	cmpi.w	#5,d6
	blt.s	.column
	addq.w	#1,d7
	cmpi.w	#7,d7
	blt.s	.row
.advance:
	adda.w	#6,a2
	bra.s	.character
.done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts

	section	data,data
; Character followed by seven five-bit rows, high displayed bit first.
.font:
	dc.b	'A',$0e,$11,$11,$1f,$11,$11,$11
	dc.b	'C',$0e,$11,$10,$10,$10,$11,$0e
	dc.b	'D',$1e,$11,$11,$11,$11,$11,$1e
	dc.b	'E',$1f,$10,$10,$1e,$10,$10,$1f
	dc.b	'G',$0e,$11,$10,$17,$11,$11,$0f
	dc.b	'H',$11,$11,$11,$1f,$11,$11,$11
	dc.b	'I',$1f,$04,$04,$04,$04,$04,$1f
	dc.b	'K',$11,$12,$14,$18,$14,$12,$11
	dc.b	'L',$10,$10,$10,$10,$10,$10,$1f
	dc.b	'N',$11,$19,$19,$15,$13,$13,$11
	dc.b	'O',$0e,$11,$11,$11,$11,$11,$0e
	dc.b	'P',$1e,$11,$11,$1e,$10,$10,$10
	dc.b	'Q',$0e,$11,$11,$11,$15,$12,$0d
	dc.b	'R',$1e,$11,$11,$1e,$14,$12,$11
	dc.b	'S',$0f,$10,$10,$0e,$01,$01,$1e
	dc.b	'T',$1f,$04,$04,$04,$04,$04,$04
	dc.b	'U',$11,$11,$11,$11,$11,$11,$0e
	dc.b	'Y',$11,$11,$0a,$04,$04,$04,$04
	dc.b	'!',$04,$04,$04,$04,$04,$00,$04
	dc.b	0
