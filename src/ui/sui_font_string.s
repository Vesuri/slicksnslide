	section	code
	xdef	sui_font_string
	xref	sui_font_measure
	xref	sui_font_glyph

; 301ab..302b5 string routine plus 3000f's character/highlight wrapper.
; a0 = chunky, a1 = runtime font, a2 = string
; d0.w/d1.w = anchor x/y, d2.b = original flags, d3.b = signed spacing,
; d4.w = tab width, d5.b = fallback/highlight colour (DS:1600).
; d6.w = signed shadow offsets packed x:y (DS:1602, DS:1603).
; Returns d0.w = final x - original anchor; other registers preserved.
sui_font_string:
	movem.l	d1-d7/a0-a6,-(sp)
	link	a6,#-32
	move.w	d0,-2(a6)
	move.w	d0,-4(a6)
	move.w	d1,-6(a6)
	move.w	d2,-8(a6)
	clr.w	-10(a6)
	move.w	d6,-28(a6)
	movea.l	a2,a5
	andi.w	#3,d2
	cmpi.w	#1,d2
	beq.s	.align
	cmpi.w	#2,d2
	bne.s	.character
.align:
	moveq	#0,d0
	jsr	sui_font_measure
	cmpi.w	#1,d2
	bne.s	.subtract
	tst.w	d0
	bpl.s	.half
	addq.w	#1,d0
.half:
	asr.w	#1,d0
.subtract:
	sub.w	d0,-4(a6)
.character:
	tst.b	(a5)
	beq	.done
	cmpi.w	#1000,-10(a6)
	beq	.done
	moveq	#0,d6
	move.b	(a5),d6
	cmpi.b	#13,d6
	beq	.newline
	cmpi.b	#10,d6
	beq	.newline
	moveq	#0,d7
	move.b	5(a1),d7
	lea	6(a1,d7.w),a3
	moveq	#0,d2
	moveq	#0,d7
	move.b	(a1),d7
.lookup:
	cmp.w	d7,d2
	bge	.advance
	cmp.b	(a3,d2.w),d6
	beq.s	.glyph
	addq.w	#1,d2
	bra.s	.lookup
.glyph:
	btst	#2,-7(a6)
	beq.s	.normal_draw
	moveq	#0,d6
	moveq	#0,d7
	move.b	5(a1),d7
	cmpi.w	#10,d7
	bls.s	.save_palette
	moveq	#10,d7
.save_palette:
	cmp.w	d7,d6
	bge.s	.draw
	move.b	6(a1,d6.w),-24(a6,d6.w)
	move.b	d5,6(a1,d6.w)
	addq.w	#1,d6
	bra.s	.save_palette
.draw:
	move.w	-4(a6),d0
	move.w	-6(a6),d1
	move.b	-28(a6),d6
	ext.w	d6
	add.w	d6,d0
	move.b	-27(a6),d6
	ext.w	d6
	add.w	d6,d1
	jsr	sui_font_glyph
	moveq	#0,d6
.restore_palette:
	cmp.w	d7,d6
	bge.s	.normal_draw
	move.b	-24(a6,d6.w),6(a1,d6.w)
	addq.w	#1,d6
	bra.s	.restore_palette
.normal_draw:
	move.w	-4(a6),d0
	move.w	-6(a6),d1
	jsr	sui_font_glyph
.advance:
	move.b	(a5),-12(a6)
	clr.b	-11(a6)
	lea	-12(a6),a2
	move.w	-4(a6),d0
	sub.w	-2(a6),d0
	jsr	sui_font_measure
	add.w	d0,-4(a6)
	bra.s	.next
.newline:
	moveq	#0,d0
	move.b	2(a1),d0
	addq.w	#1,d0
	add.w	d0,-6(a6)
	move.w	-2(a6),-4(a6)
.next:
	addq.l	#1,a5
	addq.w	#1,-10(a6)
	bra	.character
.done:
	moveq	#0,d0
	move.w	-4(a6),d0
	sub.w	-2(a6),d0
	unlk	a6
	movem.l	(sp)+,d1-d7/a0-a6
	rts
