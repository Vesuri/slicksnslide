	section	code
	xdef	slicks_blit_basic_frame
	xref	sgfx_planar_blit
	xref	slicks_basic_frame

; Temporary C-platform bridge. The translated/native side uses the register
; ABI directly; this wrapper preserves the Amiga GCC callee-saved registers.
slicks_blit_basic_frame:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	lea	slicks_basic_frame,a1
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d3
	moveq	#100,d4
	jsr	sgfx_planar_blit
	movem.l	(sp)+,d2-d7/a2-a6
	rts
