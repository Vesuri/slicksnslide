	section	code
	xdef	sui_font_measure
	xref	sui_font_cache
SUI_GLYPH_CACHE_MAP	set	8+2*256

; Native translation of 300e5..301aa (including its glyph-zero and tab rules).
; a1 = runtime font, a2 = NUL-terminated bytes, d0.w = initial tab position,
; d3.b = signed character adjustment (DS:1604), d4.w = tab width (DS:15fe).
; Returns d0.w = measured width. Preserves every other register.
sui_font_measure:
	movem.l	d1-d7/a0-a6,-(sp)
	bsr.l	sui_font_cache
	lea	SUI_GLYPH_CACHE_MAP(a4),a5	; first matching code, or count
	move.w	d0,d7
	moveq	#0,d6
	ext.w	d3
	moveq	#0,d1
	move.b	(a1),d1
	moveq	#0,d2
	move.b	5(a1),d2
	lea	6(a1,d2.w),a3	; character codes
	movea.l	a3,a4
	adda.w	d1,a4		; widths
.character:
	moveq	#0,d2
	move.b	(a2)+,d2
	beq.s	.done
	cmpi.b	#13,d2
	beq.s	.done
	cmpi.b	#10,d2
	beq.s	.done
	moveq	#0,d5
	move.b	(a5,d2.w),d5
	cmp.w	d1,d5
	bge.s	.missing
.found:
	tst.w	d5		; original tests index > 0, not >= 0
	beq.s	.missing
	moveq	#0,d0
	move.b	(a4,d5.w),d0
	moveq	#0,d5
	move.b	3(a1),d5
	add.w	d5,d0
	add.w	d3,d0
	subq.w	#1,d0
	bra.s	.advance
.missing:
	cmpi.b	#$cf,d2
	beq.s	.tab
	cmpi.b	#8,d2
	beq.s	.tab
	moveq	#0,d0
	move.b	1(a1),d0
	bra.s	.advance
.tab:
	move.w	d7,d0
	ext.l	d0
	divs.w	d4,d0
	swap	d0
	move.w	d4,d5
	sub.w	d0,d5
	move.w	d5,d0
.advance:
	add.w	d0,d6
	add.w	d6,d7		; original adds accumulated width, not this advance
	bra.s	.character
.done:
	moveq	#0,d0
	move.w	d6,d0
	movem.l	(sp)+,d1-d7/a0-a6
	rts
