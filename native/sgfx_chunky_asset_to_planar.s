	section	code
	xdef	sgfx_chunky_asset_to_planar

; Convert an original archive 320x200 chunky image into the four-bank sprite
; layout consumed by the translated VGA blitters.
;
; In:  a0 = archive asset [big-endian width word, height byte, chunky pixels]
;      a1 = destination [byte width, height, four 16000-byte banks]
; Out: d0.w = 0 on success, -1 for unsupported geometry
; Preserves: d1-d7/a0-a6
sgfx_chunky_asset_to_planar:
	movem.l	d1-d7/a0-a6,-(sp)
	cmpi.w	#320,(a0)+
	bne.s	.bad
	cmpi.b	#200,(a0)+
	bne.s	.bad
	move.b	#80,(a1)+
	move.b	#200,(a1)+
	movea.l	a1,a2
	lea	16000(a2),a3
	lea	16000(a3),a4
	lea	16000(a4),a5
	move.w	#15999,d1
.pixel_group:
	move.b	(a0)+,(a2)+
	move.b	(a0)+,(a3)+
	move.b	(a0)+,(a4)+
	move.b	(a0)+,(a5)+
	dbf	d1,.pixel_group
	moveq	#0,d0
	bra.s	.done
.bad:
	moveq	#-1,d0
.done:
	movem.l	(sp)+,d1-d7/a0-a6
	rts
