	section	code
	xdef	sui_title_step
	xdef	sui_title_colours
	xref	sutil_palette_nearest
	xref	sgfx_title_crop

; Direct 68020 translation of the observed UI prefix at runtime offsets
; 19653h..19718h. It derives three palette indices, advances the title colour
; counter, stores the third index, and restores the fixed title crop.
;
; In:  a0 = four consecutive 64 KiB VGA planes
;      a1 = full-screen planar title image
;      a2 = 768-byte RGB palette
;      a3 = address of the byte animation counter
;      a4 = address of the word third-colour result
;      d4.w = byte stride
;      d7.w = current screen base
; Out: d0.b = fixed colour index for RGB 55,15,10
;      d1.b = animated colour index for ramp+30,ramp+30,ramp+20
;      d2.b = fixed colour index for RGB 10,10,20
; Preserves: d3-d7/a0-a6
sui_title_step:
	bsr sui_title_colours
	movem.l d0-d2,-(sp)
	jsr sgfx_title_crop
	movem.l (sp)+,d0-d2
	rts

; Same counter/palette state, without erasing already-correct static pixels.
sui_title_colours:
	movem.l	d3-d7/a0-a6,-(sp)
	movea.l	a2,a6

	movea.l	a6,a1
	moveq	#55,d0
	moveq	#15,d1
	moveq	#10,d2
	jsr	sutil_palette_nearest
	move.l	d0,d3

	moveq	#0,d5
	move.b	(a3),d5
	addq.b	#4,d5
	move.b	d5,(a3)
	lsr.b	#2,d5
	cmpi.b	#32,d5
	ble.s	.ramp_ready
	moveq	#63,d6
	sub.b	d5,d6
	move.b	d6,d5
.ramp_ready:
	movea.l	a6,a1
	moveq	#0,d0
	move.b	d5,d0
	addi.b	#30,d0
	move.l	d0,d1
	moveq	#0,d2
	move.b	d5,d2
	addi.b	#20,d2
	jsr	sutil_palette_nearest
	move.l	d0,d5

	movea.l	a6,a1
	moveq	#10,d0
	moveq	#10,d1
	moveq	#20,d2
	jsr	sutil_palette_nearest
	move.l	d0,d6
	move.w	d6,(a4)

	move.l	d3,d0
	move.l	d5,d1
	move.l	d6,d2
	movem.l	(sp)+,d3-d7/a0-a6
	rts
