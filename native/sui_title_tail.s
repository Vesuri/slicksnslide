	section	code
	xdef	sui_title_tail
	xref	sutil_palette_nearest
	xref	sui_color_slot

; Direct 68020 translation of the live shared title-wrapper suffix at
; 19E4Ah..19EEFH.  The optional text branch is inactive on the BASIC path and
; the following VGA start-address update maps to no operation on the Amiga.
;
; In: a0 = 768-byte RGB palette
;     a1 = render/font state (count at +5, colour slots at +6)
;     a2 = fallback colour word
;     a3 = title phase counter word
; Preserves: d0-d7/a0-a6
sui_title_tail:
	movem.l	d0-d7/a0-a6,-(sp)
	movea.l	a1,a4
	movea.l	a2,a5
	movea.l	a3,a6

	addq.w	#1,(a6)
	move.w	(a6),d3
	cmpi.w	#2000,d3
	ble.s	.phase_ready
	clr.w	(a6)
	moveq	#0,d3
.phase_ready:
	moveq	#20,d4
	cmpi.w	#100,d3
	bge.s	.falling
	move.b	d3,d4
	addi.b	#20,d4
	bra.s	.lookup
.falling:
	cmpi.w	#140,d3
	bge.s	.lookup
	moveq	#-96,d4
	sub.b	d3,d4
.lookup:
	movea.l	a0,a1
	moveq	#0,d0
	move.b	d4,d0
	moveq	#20,d1
	moveq	#20,d2
	jsr	sutil_palette_nearest

	movea.l	a4,a1
	movea.l	a5,a2
	moveq	#0,d1
	jsr	sui_color_slot

	movem.l	(sp)+,d0-d7/a0-a6
	rts
