	section	code
	xdef	sui_bevel
	xref	sutil_palette_nearest
	xref	sgfx_span_fill

; Direct native form of the bevel/button renderer at runtime offset 208CFh.
;
; In:  a0 = four consecutive 64 KiB logical VGA planes
;      a1 = 768-byte RGB palette
;      d0.w,d1.w = x,y
;      d2.w,d3.w = width,height
;      d4.b,d5.b,d6.b = base red,green,blue
;      d7.w = guest page base
; Out: beveled rectangle drawn through sgfx_span_fill
; Preserves: d0-d7/a0-a6
sui_bevel:
	movem.l	d0-d7/a0-a6,-(sp)
	suba.w	#32,sp
	movea.l	sp,a6
	move.w	d0,0(a6)		; x
	move.w	d1,2(a6)		; y
	move.w	d2,4(a6)		; width
	move.w	d3,6(a6)		; height
	move.w	d7,8(a6)		; page

	move.b	d4,d0
	addi.b	#15,d0
	move.b	d5,d1
	addi.b	#15,d1
	move.b	d6,d2
	addi.b	#15,d2
	jsr	sutil_palette_nearest
	move.w	d0,10(a6)		; highlight

	move.b	d4,d0
	move.b	d5,d1
	move.b	d6,d2
	jsr	sutil_palette_nearest
	move.w	d0,12(a6)		; base (retained for exact state)

	move.b	d4,d0
	subi.b	#20,d0
	move.b	d5,d1
	subi.b	#20,d1
	move.b	d6,d2
	subi.b	#20,d2
	jsr	sutil_palette_nearest
	move.w	d0,14(a6)		; shadow

	moveq	#0,d0
	move.w	4(a6),d0
	ext.l	d0
	moveq	#5,d1
	divs.w	d1,d0
	cmpi.w	#25,d0
	ble.s	.thickness_ready
	moveq	#25,d0
.thickness_ready:
	move.w	d0,16(a6)

	moveq	#0,d0
	move.w	6(a6),d0
	ext.l	d0
	moveq	#2,d1
	divs.w	d1,d0
	move.w	d0,18(a6)		; final inclusive layer

	moveq	#0,d0
	move.w	4(a6),d0
	ext.l	d0
	divs.w	d1,d0
	addq.w	#1,d0
	move.w	d0,20(a6)		; initial half width
	clr.w	22(a6)			; layer

.layer:
	move.w	2(a6),d0
	add.w	22(a6),d0
	move.w	d0,24(a6)
	move.w	10(a6),26(a6)
	bsr.s	.draw_row

	move.w	2(a6),d0
	add.w	6(a6),d0
	sub.w	22(a6),d0
	move.w	d0,24(a6)
	move.w	14(a6),26(a6)
	bsr.s	.draw_row

	move.w	16(a6),d0
	bne.s	.width_ready
	moveq	#1,d0
.width_ready:
	move.w	d0,20(a6)
	asr.w	16(a6)
	addq.w	#1,22(a6)
	move.w	22(a6),d0
	cmp.w	18(a6),d0
	ble.s	.layer
	bra	.done

; Draw the centre and two side spans for one bevel scanline.
.draw_row:
	move.w	0(a6),d0
	add.w	20(a6),d0
	move.w	24(a6),d1
	move.w	0(a6),d2
	add.w	4(a6),d2
	sub.w	20(a6),d2
	move.w	d1,d3
	addq.w	#1,d3
	move.w	12(a6),d4		; centre always uses the base colour
	move.w	8(a6),d5
	moveq	#100,d6
	jsr	sgfx_span_fill

	move.w	0(a6),d0
	add.w	16(a6),d0
	move.w	24(a6),d1
	move.w	0(a6),d2
	add.w	20(a6),d2
	move.w	d1,d3
	addq.w	#1,d3
	move.w	26(a6),d4
	move.w	8(a6),d5
	moveq	#100,d6
	jsr	sgfx_span_fill

	move.w	0(a6),d0
	add.w	4(a6),d0
	sub.w	20(a6),d0
	move.w	24(a6),d1
	move.w	0(a6),d2
	add.w	4(a6),d2
	sub.w	16(a6),d2
	move.w	d1,d3
	addq.w	#1,d3
	move.w	26(a6),d4
	move.w	8(a6),d5
	moveq	#100,d6
	jsr	sgfx_span_fill
	rts

.done:
	adda.w	#32,sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts
