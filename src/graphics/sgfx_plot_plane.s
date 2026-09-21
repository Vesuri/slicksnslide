	section	code
	xdef	sgfx_plot_plane

; Direct 68020 replacement for live_vga_plot_plane.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      d0.w = x, d1.w = y, d2.b = value
;      d3.w = screen base, d4.w = byte stride
; Out: selected byte updated
; Clobbers: d5-d6/cc.  All address arithmetic wraps to the guest's 16 bits.
sgfx_plot_plane:
	moveq	#0,d5
	move.w	d1,d5
	mulu.w	d4,d5
	moveq	#0,d6
	move.w	d0,d6
	lsr.w	#2,d6
	add.w	d6,d5
	add.w	d3,d5
	andi.l	#$ffff,d5

	moveq	#0,d6
	move.w	d0,d6
	andi.w	#3,d6
	swap	d6
	add.l	d5,d6
	move.b	d2,(a0,d6.l)
	rts
