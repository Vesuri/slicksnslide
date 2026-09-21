	section	code
	xdef	sgfx_read_pixel

; Direct 68020 replacement for live_vga_read_pixel.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      d0.w = x, d1.w = y
;      d3.w = screen base, d4.w = byte stride
; Out: d0.l = zero-extended pixel byte
; Clobbers: d5-d6/cc.  All address arithmetic wraps to the guest's 16 bits.
sgfx_read_pixel:
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
	moveq	#0,d0
	move.b	(a0,d6.l),d0
	rts
