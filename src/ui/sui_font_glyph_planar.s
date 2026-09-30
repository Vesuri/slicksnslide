	section code
	xdef sui_font_glyph_planar
	xref slicks_title_text_page
	xref slicks_title_cached_font
	xref slicks_title_cached_offsets
; Original glyph decoding, with VGA four-bank stores for the title surface.
; Same register contract as sui_font_glyph; source zero is transparent.
sui_font_glyph_planar:
	movem.l d0-d7/a0-a6,-(sp)
	moveq #0,d3
	move.b (a1),d3
	moveq #0,d4
	move.b 5(a1),d4
	lea 6(a1),a2
	adda.w d4,a2
	adda.w d3,a2
	movea.l a2,a3
	adda.w d3,a3
	moveq #0,d4
	move.b 2(a1),d4
	moveq #0,d6
	moveq #0,d7
	cmpa.l slicks_title_cached_font,a1
	bne.s .preceding
	cmpi.w #256,d2
	bhs.s .preceding
	lea slicks_title_cached_offsets,a4
	move.w (a4,d2.w*2),d7
	bra.s .selected
.preceding:
	cmp.w d2,d6
	bge.s .selected
	moveq #0,d5
	move.b (a2,d6.w),d5
	addq.w #3,d5
	andi.w #$fffc,d5
	mulu.w d4,d5
	add.w d5,d7
	addq.w #1,d6
	bra.s .preceding
.selected:
	adda.l d7,a3
	moveq #0,d3
	move.b (a2,d2.w),d3
	move.w d3,d5
	addq.w #3,d5
	andi.w #$fffc,d5
	moveq #0,d6
.column:
	cmp.w d3,d6
	bge.s .done
	; X and VGA bank stay fixed down a glyph column. Build its destination
	; once, then advance by the original 100-byte VGA row stride. Signed Y
	; permits top clipping without forming an address from a wrapped word.
	tst.w d0
	bmi.s .next_column
	cmpi.w #320,d0
	bge.s .next_column
	move.w d1,d2
	muls.w #100,d2
	movea.l d2,a5
	moveq #0,d2
	move.w d0,d2
	lsr.w #2,d2
	adda.l d2,a5
	moveq #0,d2
	move.w slicks_title_text_page,d2
	adda.l d2,a5
	moveq #0,d2
	move.w d0,d2
	andi.w #3,d2
	swap d2
	adda.l d2,a5
	adda.l a0,a5
	movea.l a3,a4
	moveq #0,d7
.row:
	move.w d1,d2
	add.w d7,d2
	bmi.s .skip
	cmpi.w #200,d2
	bge.s .skip
	moveq #0,d2
	move.b (a4),d2
	beq.s .skip
	move.b 5(a1,d2.w),(a5)
.skip:
	adda.w #100,a5
	adda.w d5,a4
	addq.w #1,d7
	cmp.w d4,d7
	blt.s .row
.next_column:
	addq.l #1,a3
	addq.w #1,d0
	addq.w #1,d6
	bra.s .column
.done:
	movem.l (sp)+,d0-d7/a0-a6
	rts
