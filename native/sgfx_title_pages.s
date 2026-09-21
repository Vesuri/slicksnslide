	section	code
	xdef	sgfx_title_pages
	xref	sgfx_planar_blit

; Direct 68020 translation of the observed title-page submission block at
; runtime offsets 195F0h..19639h. The original calls the opaque planar blitter
; at (0,0) first for the page in data word 1D89h and then for 1D87h.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      a1 = full-screen planar image
;      d4.w = byte stride
;      d5.w = first screen base (observed 7FBCh)
;      d6.w = second screen base (observed 0000h)
; Out: both logical VGA pages receive the image
; Preserves: d2-d7/a0-a6 (safe at the temporary C platform boundary)
; Clobbers: d0-d1/cc
sgfx_title_pages:
	movem.l	d2-d7/a1-a6,-(sp)
	move.w	d6,-(sp)
	movea.l	a1,a6

	moveq	#0,d0
	moveq	#0,d1
	move.w	d5,d3
	jsr	sgfx_planar_blit

	movea.l	a6,a1
	moveq	#0,d0
	moveq	#0,d1
	move.w	(sp),d3
	jsr	sgfx_planar_blit

	addq.l	#2,sp
	movem.l	(sp)+,d2-d7/a1-a6
	rts
