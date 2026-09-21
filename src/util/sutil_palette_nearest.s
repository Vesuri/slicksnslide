	section	code
	xdef	sutil_palette_nearest

; Direct 68020 translation of the palette matcher at runtime offset 26EAEh.
; Entry zero is reserved. Entries 1..255 are compared by Manhattan RGB
; distance, with the original best index 1 / best distance 300 initialization.
;
; In:  a1 = 768-byte RGB palette
;      d0.b,d1.b,d2.b = requested red, green, blue
; Out: d0.l = best palette index, 1..255
; Preserves: d1-d7/a0-a6
sutil_palette_nearest:
	movem.l	d1-d7/a0-a6,-(sp)
	moveq	#0,d5
	move.b	d0,d5
	ext.w	d5
	moveq	#0,d6
	move.b	d1,d6
	ext.w	d6
	moveq	#0,d7
	move.b	d2,d7
	ext.w	d7
	lea	3(a1),a2
	moveq	#1,d2
	moveq	#1,d3
	move.w	#300,d4
.entry:
	moveq	#0,d0
	move.b	(a2)+,d0
	sub.w	d5,d0
	bpl.s	.red_positive
	neg.w	d0
.red_positive:
	move.w	d0,d1

	moveq	#0,d0
	move.b	(a2)+,d0
	sub.w	d6,d0
	bpl.s	.green_positive
	neg.w	d0
.green_positive:
	add.w	d0,d1

	moveq	#0,d0
	move.b	(a2)+,d0
	sub.w	d7,d0
	bpl.s	.blue_positive
	neg.w	d0
.blue_positive:
	add.w	d0,d1
	cmp.w	d4,d1
	bge.s	.next
	move.w	d1,d4
	move.w	d2,d3
.next:
	addq.w	#1,d2
	cmpi.w	#256,d2
	blt.s	.entry
	moveq	#0,d0
	move.b	d3,d0
	movem.l	(sp)+,d1-d7/a0-a6
	rts
