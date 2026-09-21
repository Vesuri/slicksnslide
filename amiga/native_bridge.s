	section	code
	xdef	slicks_draw_title_pages
	xref	sgfx_title_pages
	xref	sui_title_step
	xref	sui_title_menu
	xref	slicks_basic_frame
	xref	slicks_basic_palette
	xref	slicks_title_counter
	xref	slicks_title_third_color

; Temporary C-platform bridge. The translated/native side uses the register
; ABI directly; this wrapper preserves the Amiga GCC callee-saved registers.
slicks_draw_title_pages:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	lea	slicks_basic_frame,a1
	moveq	#100,d4
	move.w	#$7fbc,d5
	moveq	#0,d6
	jsr	sgfx_title_pages
	lea	slicks_basic_frame,a1
	lea	slicks_basic_palette,a2
	lea	slicks_title_counter,a3
	lea	slicks_title_third_color,a4
	moveq	#0,d7
	jsr	sui_title_step
	lea	slicks_basic_palette,a1
	moveq	#0,d7
	jsr	sui_title_menu
	movem.l	(sp)+,d2-d7/a2-a6
	rts
