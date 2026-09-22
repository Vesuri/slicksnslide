	section	code
	xdef	sui_title_menu
	xref	sui_bevel
	xref	sui_draw_text

; Native title-menu slice corresponding to the observed six-entry loop at
; 19719h..19825h. Index four is intentionally absent, matching the original.
;
; In: a0 = logical planes, a1 = RGB palette
;     d0.b = ordinary label colour, d1.b = selected label colour
;     d2.w = selected native entry (0..5)
;     d7.w = guest page base
; Preserves: d0-d7/a0-a6
sui_title_menu:
	movem.l	d0-d7/a0-a6,-(sp)
	suba.w	#10,sp
	movea.l	sp,a6
	move.b	d0,(a6)
	move.b	d1,1(a6)
	move.w	d7,2(a6)
	move.l	a1,4(a6)
	move.w	d2,8(a6)

	moveq	#120,d0
	moveq	#82,d1
	moveq	#81,d2
	moveq	#14,d3
	moveq	#50,d4
	moveq	#10,d5
	moveq	#10,d6
	move.w	2(a6),d7
	movea.l	4(a6),a1
	jsr	sui_bevel

	lea	.label_go,a1
	moveq	#85,d1
	moveq	#0,d6
	bsr.s	.label
	lea	.label_players,a1
	moveq	#98,d1
	moveq	#1,d6
	bsr.s	.label
	lea	.label_tracks,a1
	moveq	#111,d1
	moveq	#2,d6
	bsr.s	.label
	lea	.label_options,a1
	moveq	#124,d1
	moveq	#3,d6
	bsr.s	.label
	lea	.label_read,a1
	move.w	#137,d1
	moveq	#4,d6
	bsr.s	.label
	lea	.label_quit,a1
	move.w	#150,d1
	moveq	#5,d6
	bsr.s	.label
	bra.s	.done

.label:
	moveq	#0,d2
	move.b	(a6),d2
.check_selected:
	cmp.w	8(a6),d6
	bne.s	.draw
	move.b	1(a6),d2
.draw:
	move.w	#160,d0
	moveq	#0,d3
	move.w	2(a6),d3
	jsr	sui_draw_text
	rts
.done:
	adda.w	#10,sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

	section	data,data
.label_go:	dc.b	"GO !!!",0
.label_players:	dc.b	"PLAYERS",0
.label_tracks:	dc.b	"TRACKS",0
.label_options:	dc.b	"OPTIONS",0
.label_read:	dc.b	"READ THIS",0
.label_quit:	dc.b	"QUIT",0
	even
