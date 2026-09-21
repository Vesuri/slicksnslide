	section	code
	xdef	sgfx_plot

; Direct replacement for live_vga_plot with caller-selected plane made explicit.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      d0.w = x, d1.w = y, d2.b = value
;      d3.w = screen base, d4.w = byte stride, d5.w = write plane
; Out: selected byte updated
; Clobbers: d6-d7/cc.  All address arithmetic wraps to the guest's 16 bits.
sgfx_plot:
	moveq	#0,d6
	move.w	d1,d6
	mulu.w	d4,d6
	moveq	#0,d7
	move.w	d0,d7
	lsr.w	#2,d7
	add.w	d7,d6
	add.w	d3,d6
	andi.l	#$ffff,d6

	moveq	#0,d7
	move.w	d5,d7
	andi.w	#3,d7
	swap	d7
	add.l	d6,d7
	move.b	d2,(a0,d7.l)
	rts
