	section	code
	xdef	sui_title_menu
	xdef	slicks_title_labels
	xref	sui_bevel
	xref	sui_draw_text

; Original 29852..29928. Entry four is unconditionally skipped by this build.
;
; In: a0 = logical planes, a1 = RGB palette
;     d0.b = ordinary label colour, d1.b = selected label colour
;     d2.w = selected native entry (0..6)
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

	movea.l slicks_title_labels,a1
	moveq	#85,d1
	moveq	#0,d6
	bsr.s	.label
	movea.l slicks_title_labels+4,a1
	moveq	#98,d1
	moveq	#1,d6
	bsr.s	.label
	movea.l slicks_title_labels+8,a1
	moveq	#111,d1
	moveq	#2,d6
	bsr.s	.label
	movea.l slicks_title_labels+12,a1
	moveq	#124,d1
	moveq	#3,d6
	bsr.s	.label
	movea.l slicks_title_labels+20,a1
	move.w	#137,d1
	moveq	#5,d6
	bsr.s	.label
	movea.l slicks_title_labels+24,a1
	move.w	#150,d1
	moveq	#6,d6
	bsr.s	.label
	bra.s	.done

.label:
	moveq	#0,d2
	move.b	(a6),d2
.check_selected:
	cmp.w	8(a6),d6
	bne.s	.draw
	; Original bevel follows the selected label, before drawing its text.
	movem.l d0-d7/a0-a1,-(sp)
	moveq #120,d0
	subq.w #3,d1
	moveq #81,d2
	moveq #14,d3
	moveq #50,d4
	moveq #10,d5
	moveq #10,d6
	move.w 2(a6),d7
	movea.l 4(a6),a1
	jsr sui_bevel
	movem.l (sp)+,d0-d7/a0-a1
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
; Startup resolves these into the resident language table. Original 36227
; falls back to the key itself when a table/key is absent.
slicks_title_labels:
	dc.l .label_go,.label_players,.label_tracks,.label_options
	dc.l .label_load,.label_read,.label_quit
.label_go:	dc.b	"menu1",0
.label_players:	dc.b	"menu2",0
.label_tracks:	dc.b	"menu3",0
.label_options:	dc.b	"menu4",0
.label_load:	dc.b	"menu5",0
.label_read:	dc.b	"menu6",0
.label_quit:	dc.b	"menu7",0
	even
