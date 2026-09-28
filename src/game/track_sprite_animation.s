	section code,code
	xdef slicks_draw_animated_track_sprite
	xdef slicks_draw_animated_track_sprite_regs
	xref slicks_draw_sprite_opaque_regs
	xref mult320
	ifnd SLICKS_SPRITE_TEST_BASE
SLICKS_SPRITE_TEST_BASE equ 0
	endif

; Changed-frame native drawing core, also exercised by an isolated oracle.
; C ABI: actor, previous descriptor, track assets, chunky.
; Caller proves active track actor, ready assets and deferred sprite dirtiness.
; Return 1 after drawing a changed unmasked animation at unchanged position;
; caller MUST publish both previous and new rectangles before displaying it.
; Return 0 without any writes for the general renderer. No packet is touched;
; the integration must invalidate/revalidate any existing cached packet.
slicks_draw_animated_track_sprite:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a2
	movea.l 52(sp),a3
	movea.l 56(sp),a5
	movea.l 60(sp),a6
	bsr.w slicks_draw_animated_track_sprite_regs
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Register ABI for the outer drawing chain owning the saves:
; a2 actor, a3 previous, a5 assets, a6 chunky. Clobbers d0-d7/a0-a6.
; On success a4 actor/a6 previous remain available to the publication step.
slicks_draw_animated_track_sprite_regs:
	cmpi.b #3,21(a2)
	bne.w .reject
	cmpi.b #3,6(a3)
	bne.w .reject
	tst.b 23(a2)
	bne.w .reject
	moveq #0,d0
	move.b 20(a2),d0
	cmp.b 7(a3),d0
	bne.w .reject
	cmpi.b #5,d0
	bcc.w .reject
	moveq #0,d1
	move.b 16(a2),d1
	cmp.b 8(a3),d1
	beq.w .reject
	cmpi.b #4,d1
	bcc.w .reject
	move.b 24(a2),d2
	cmp.b 9(a3),d2
	bne.w .reject
	move.w 22(a2),d2
	cmp.w 10(a3),d2
	bne.w .reject
	move.w (a2),d6
	asr.w #6,d6
	bmi.w .reject
	cmp.w (a3),d6
	bne.w .reject
	move.w 2(a2),d7
	asr.w #6,d7
	bmi.w .reject
	cmp.w 2(a3),d7
	bne.w .reject
	move.l 26(a2),d2
	cmp.l (a3),d2
	bne.w .reject
	move.w 30(a2),d2
	cmp.w 4(a3),d2
	bne.w .reject
	lsl.w #2,d0
	add.w d1,d0
	lea .frames(pc),a0
	moveq #0,d1
	move.b (a0,d0.w),d1
	mulu.w #259,d1
	adda.l d1,a5
	tst.b 258(a5)
	beq.s .reject
	moveq #0,d4
	move.b 128(a5),d4
	beq.s .reject
	moveq #0,d3
	move.b 129(a5),d3
	beq.s .reject
	move.w d4,d0
	mulu.w d3,d0
	cmpi.l #128,d0
	bhi.s .reject
	move.w d6,d0
	add.w d4,d0
	cmpi.w #320,d0
	bgt.s .reject
	move.w d7,d0
	add.w d3,d0
	cmpi.w #200,d0
	bgt.s .reject
	; Every rejection precedes writes. New dimensions may differ from old.
	move.w 128(a5),30(a2)
	clr.b 33(a2)
	movea.l a2,a4
	movea.l a6,a0
	movea.l a3,a6
	lea mult320,a1
	adda.l (a1,d7.w*4),a0
	adda.w d6,a0
	movea.l a5,a1
	lea 130(a5),a3
	lea 36(a4),a2
	jsr slicks_draw_sprite_opaque_regs+SLICKS_SPRITE_TEST_BASE
	move.b #1,32(a4)
	clr.b 6(a6)
	moveq #1,d0
	rts
.reject:
	moveq #0,d0
	rts
.frames:
	dc.b 0,5,6,7,1,1,1,1,2,8,9,10,3,3,3,3,4,11,12,13
