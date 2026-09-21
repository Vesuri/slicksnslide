	section	code
	xdef	sgfx_checker_fill
	xref	sgfx_plot_plane

; Direct 68020 translation of the recovered Slicks function at runtime offset
; A498h. The bounded BASIC trace has not reached this caller, but its callee is
; the heavily exercised live_vga_plot_plane primitive.
; The original walks a signed half-open rectangle, toggles a byte for every
; visited pixel, and plots on alternating visits through live_vga_plot_plane.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      d0.w = x0, d1.w = y0, d2.w = x1, d3.w = y1
;      d4.b = value, d5.w = screen base, d6.w = byte stride
; Out: alternating pixels in [x0,x1) x [y0,y1) updated
; Preserves: d2-d7/a0-a6 (safe at the temporary C platform boundary)
; Clobbers: d0-d1/cc
sgfx_checker_fill:
	movem.l	d2-d7/a1-a6,-(sp)
	movea.l	d0,a1		; x0
	movea.l	d2,a2		; x1
	movea.l	d3,a3		; y1
	movea.l	d4,a4		; pixel value
	movea.l	d5,a5		; screen base
	movea.l	d6,a6		; byte stride
	moveq	#0,d7		; original byte toggle starts at zero

	move.w	a3,d2
	cmp.w	d2,d1
	bge.s	.done
.row:
	move.w	a1,d0
	move.w	a2,d2
	cmp.w	d2,d0
	bge.s	.next_row
.pixel:
	eori.b	#1,d7
	beq.s	.skip_plot
	move.l	a4,d2
	move.l	a5,d3
	move.l	a6,d4
	jsr	sgfx_plot_plane
.skip_plot:
	addq.w	#1,d0
	move.w	a2,d2
	cmp.w	d2,d0
	blt.s	.pixel
.next_row:
	addq.w	#1,d1
	move.w	a3,d2
	cmp.w	d2,d1
	blt.s	.row
.done:
	movem.l	(sp)+,d2-d7/a1-a6
	rts
