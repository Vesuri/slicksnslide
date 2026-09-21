	section	code
	xdef	slicks_draw_title_pages
	xref	sgfx_title_pages
	xref	slicks_basic_frame

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
	movem.l	(sp)+,d2-d7/a2-a6
	rts
