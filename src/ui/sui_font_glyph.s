	section	code
	xdef	sui_font_glyph
	xref	sui_font_cache
	xref	mult320

; Glyph offsets come from sui_font_cache (identical to 2ff0c..2ff2b).
SUI_GLYPH_CACHE_OFFSETS	set	8

; Native translation of 2fef2..3000e (original font glyph rasterizer).
; a0 = authoritative 320-wide chunky surface, a1 = original runtime font
; d0.w = x, d1.w = y, d2.w = glyph index. All registers preserved.
; Caller supplies a valid glyph. Visible glyphs use the original fast path;
; edge glyphs are clipped so VGA's invisible right margin cannot wrap onto
; the next row of the narrower native chunky surface.
; Only the hardware exit changes: selected VGA plane + plot becomes a chunky
; byte store. Zero source pixels are transparent; other bytes index the font
; palette at +5+pixel. Do not collapse this to a boolean foreground mask.
sui_font_glyph:
	movem.l	d0-d7/a0-a6,-(sp)
	moveq	#0,d3
	move.b	(a1),d3
	moveq	#0,d4
	move.b	5(a1),d4
	lea	6(a1),a2
	adda.w	d4,a2
	adda.w	d3,a2		; widths = font + 6 + colours + glyphs
	movea.l	a2,a3
	adda.w	d3,a3		; first padded glyph
	moveq	#0,d4
	move.b	2(a1),d4		; height
	moveq	#0,d6
	moveq	#0,d7
	bsr.l	sui_font_cache
.cached:
	moveq	#0,d7
	move.w	SUI_GLYPH_CACHE_OFFSETS(a4,d2.w*2),d7
.selected:
	adda.l	d7,a3
	moveq	#0,d3
	move.b	(a2,d2.w),d3	; actual glyph width
	move.w	d3,d5
	addq.w	#3,d5
	and.w	#$fffc,d5	; padded source row stride
	tst.w	d0
	bmi	.clipped
	tst.w	d1
	bmi	.clipped
	move.w	d0,d6
	add.w	d3,d6
	cmpi.w	#320,d6
	bgt	.clipped
	move.w	d1,d6
	add.w	d4,d6
	cmpi.w	#200,d6
	bgt	.clipped
	moveq	#0,d7
	move.w	d1,d7
	lsl.w	#2,d7
	lea	mult320,a4
	move.l	(a4,d7.w),d7
	moveq	#0,d6
	move.w	d0,d6
	add.l	d6,d7
	lea	(a0,d7.l),a5
	moveq	#0,d6
.column:
	cmp.w	d3,d6
	bge.s	.done
	movea.l	a3,a4
	movea.l	a5,a6
	moveq	#0,d7
.row:
	cmp.w	d4,d7
	bge.s	.next_column
	moveq	#0,d2
	move.b	(a4),d2
	beq.s	.transparent
	move.b	5(a1,d2.w),(a6)
.transparent:
	adda.w	d5,a4
	adda.w	#320,a6
	addq.w	#1,d7
	bra.s	.row
.next_column:
	addq.l	#1,a3
	addq.l	#1,a5
	addq.w	#1,d6
	bra.s	.column
.done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts

.clipped:
	lea	mult320,a5
	moveq	#0,d6
.clip_column:
	cmp.w	d3,d6
	bge.s	.done
	tst.w	d0
	bmi.s	.clip_next
	cmpi.w	#320,d0
	bge.s	.clip_next
	movea.l	a3,a4
	moveq	#0,d7
.clip_row:
	cmp.w	d4,d7
	bge.s	.clip_next
	move.w	d1,d2
	add.w	d7,d2
	bmi.s	.clip_skip
	cmpi.w	#200,d2
	bge.s	.clip_skip
	lsl.w	#2,d2
	movea.l	(a5,d2.w),a6
	adda.l	a0,a6
	adda.w	d0,a6
	moveq	#0,d2
	move.b	(a4),d2
	beq.s	.clip_skip
	move.b	5(a1,d2.w),(a6)
.clip_skip:
	adda.w	d5,a4
	addq.w	#1,d7
	bra.s	.clip_row
.clip_next:
	addq.l	#1,a3
	addq.w	#1,d0
	addq.w	#1,d6
	bra.s	.clip_column

