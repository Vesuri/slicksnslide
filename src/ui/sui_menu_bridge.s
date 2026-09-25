	section	code
	xdef	slicks_menu_measure
	xdef	slicks_menu_text
	xdef	slicks_records_text
	xdef	slicks_standings_text
	xdef	slicks_help_measure
	xdef	slicks_help_text
	xref	sui_font_measure
	xref	sui_font_string

; GCC ABI: short slicks_menu_measure(font, text)
slicks_menu_measure:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a1
	movea.l	52(sp),a2
	moveq	#0,d0
	moveq	#1,d3
	moveq	#10,d4
	jsr	sui_font_measure
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; GCC ABI: short slicks_help_measure(font,text,spacing)
; Help's <distx> changes the original signed DS:1604 byte.
slicks_help_measure:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a1
	movea.l	52(sp),a2
	moveq	#0,d0
	move.w	58(sp),d3
	moveq	#10,d4
	jsr	sui_font_measure
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; GCC ABI: short slicks_help_text(pixels,font,text,x,y,spacing)
; The returned advance, not measured width, positions following text spans.
slicks_help_text:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	movea.l	56(sp),a2
	move.w	62(sp),d0
	move.w	66(sp),d1
	moveq	#0,d2
	move.w	70(sp),d3
	moveq	#10,d4
	moveq	#0,d5
	move.w	#$0100,d6
	jsr	sui_font_string
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; GCC ABI: void slicks_menu_text(pixels,font,text,x,y,flags)
; Original startup text settings: spacing 1, tab 10, highlight 0, shadow (1,0).
slicks_menu_text:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	movea.l	56(sp),a2
	move.w	62(sp),d0
	move.w	66(sp),d1
	move.w	70(sp),d2
	moveq	#1,d3
	moveq	#10,d4
	moveq	#0,d5
	move.w	#$0100,d6
	jsr	sui_font_string
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Same ABI plus highlight colour. Records names/times use original flags=4
; with DS:1600 set by their painter, rather than startup's fixed zero.
slicks_records_text:
	movem.l	d2-d7/a2-a6,-(sp)
	move.w	#$0100,d6
	bra.s	slicks_result_text_body
slicks_standings_text:
	movem.l	d2-d7/a2-a6,-(sp)
	move.w	#$0101,d6
slicks_result_text_body:
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	movea.l	56(sp),a2
	move.w	62(sp),d0
	move.w	66(sp),d1
	move.w	70(sp),d2
	moveq	#1,d3
	moveq	#10,d4
	moveq	#0,d5
	move.b	75(sp),d5
	jsr	sui_font_string
	movem.l	(sp)+,d2-d7/a2-a6
	rts
