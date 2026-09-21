	section	code
	xdef	sgfx_title_crop
	xref	sgfx_planar_subrect_blit

; Direct 68020 translation of the observed redraw block at runtime offsets
; 196ECh..19718h. It restores the title image's fixed 100x97-pixel crop.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      a1 = full-screen planar image
;      d4.w = byte stride
;      d7.w = current screen base (observed 0000h and 7FBCh)
; Out: the cropped title region is copied to the current page
; Preserves: d2-d7/a0-a6 (safe at the temporary C platform boundary)
; Clobbers: d0-d1/cc
sgfx_title_crop:
	movem.l	d2-d7/a1-a6,-(sp)
	moveq	#0,d0
	moveq	#0,d1
	moveq	#110,d2
	moveq	#77,d3
	moveq	#100,d5
	moveq	#97,d6
	jsr	sgfx_planar_subrect_blit
	movem.l	(sp)+,d2-d7/a1-a6
	rts
