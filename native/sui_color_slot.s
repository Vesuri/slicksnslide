	section	code
	xdef	sui_color_slot

; Direct 68020 translation of the colour-slot helper at runtime offset 1FD63h.
;
; In:  a1 = render/font state (count at +5, colour slots at +6)
;      a2 = address of the fallback colour word
;      d0.b = replacement colour
;      d1.b = signed slot index (only the original low byte is significant)
; Out: d0.l = previous colour byte
; Preserves: d1-d7/a0-a6
sui_color_slot:
	movem.l	d1-d7/a0-a6,-(sp)
	moveq	#0,d2
	move.b	d1,d2
	ext.w	d2
	tst.w	d2
	bmi.s	.fallback

	moveq	#0,d3
	move.b	6(a1,d2.w),d3
	moveq	#0,d4
	move.b	5(a1),d4
	cmp.w	d4,d2
	bge.s	.done
	move.b	d0,6(a1,d2.w)
	bra.s	.done

.fallback:
	moveq	#0,d3
	move.b	1(a2),d3
	moveq	#0,d4
	move.b	d0,d4
	ext.w	d4
	move.w	d4,(a2)
.done:
	move.l	d3,d0
	movem.l	(sp)+,d1-d7/a0-a6
	rts
