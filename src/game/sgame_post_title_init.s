	section	code
	xdef	sgame_post_title_init

; Direct 68020 translation of the observed post-title 4 x 13 state reset at
; runtime offsets 16272h..162E4h.  Each player receives the common seed word;
; every grid word is cleared, then becomes 4 when mode is zero and the
; corresponding low flag bit is set.
;
; In: a0 = 52-word player grid, four rows of 13 words
;     a1 = 13 flag bytes shared by all four rows
;     a2 = four player seed words
;     d0.w = mode word
;     d1.w = common player seed
; Preserves: d0-d7/a0-a6
sgame_post_title_init:
	movem.l	d0-d7/a0-a6,-(sp)
	movea.l	a0,a3
	moveq	#3,d2
.player:
	move.w	d1,(a2)+
	movea.l	a1,a4
	moveq	#12,d3
.cell:
	clr.w	(a3)
	tst.w	d0
	bne.s	.next
	btst	#0,(a4)
	beq.s	.next
	move.w	#4,(a3)
.next:
	addq.l	#2,a3
	addq.l	#1,a4
	dbra	d3,.cell
	dbra	d2,.player
	movem.l	(sp)+,d0-d7/a0-a6
	rts
