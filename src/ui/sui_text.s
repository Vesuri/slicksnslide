	section code
	xdef sui_draw_text
	xdef sui_draw_small_text
	xref sui_font_string_planar
	xref slicks_title_font
	xref slicks_title_small_font
	xref slicks_title_third_color
	xref slicks_title_text_page
; a0=logical planes, a1=text, d0/d1=centre/top, d2=colour, d3=page.
; Original main-menu font at DS:0688 is iso.@f.
sui_draw_text:
	movem.l d0-d7/a0-a6,-(sp)
	move.w d3,slicks_title_text_page
	movea.l a1,a2
	movea.l slicks_title_font,a1
	moveq #0,d5
	move.w slicks_title_third_color,d5
	bra.s sui_title_text_body
sui_draw_small_text:
	movem.l d0-d7/a0-a6,-(sp)
	move.w d3,slicks_title_text_page
	movea.l a1,a2
	movea.l slicks_title_small_font,a1
	moveq #0,d5
sui_title_text_body:
	move.l a1,d4
	beq.s .done
	move.b d2,6(a1)
	moveq #5,d2
	moveq #1,d3
	moveq #10,d4
	move.w #$0100,d6
	jsr sui_font_string_planar
.done:
	movem.l (sp)+,d0-d7/a0-a6
	rts
