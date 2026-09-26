	section code,code
	xdef slicks_draw_unchanged_track_sprite
	xref slicks_draw_sprite_opaque
	xref slicks_draw_sprite_visible
	xref mult320
	ifnd SLICKS_SPRITE_TEST_BASE
SLICKS_SPRITE_TEST_BASE equ 0
	endif

; C ABI: actor, previous description, visibility entry, asset table, chunky,
; deferred-dirty flag. Return 1 only after drawing an unchanged in-bounds
; sprite. Otherwise return 0 without writing anything, for the general path.
; Layout assertions live beside the production caller.
slicks_draw_unchanged_track_sprite:
	movem.l d2-d7/a2-a6,-(sp)
	tst.l 68(sp)
	beq.w .fallback
	movea.l 48(sp),a2
	movea.l 52(sp),a3
	cmpi.b #3,21(a2)
	bne.w .fallback
	cmpi.b #3,6(a3)
	bne.w .fallback
	move.b 20(a2),d0
	cmp.b 7(a3),d0
	bne.w .fallback
	cmpi.b #5,d0
	bcc.w .fallback
	move.b 16(a2),d1
	cmp.b 8(a3),d1
	bne.w .fallback
	cmpi.b #4,d1
	bcc.w .fallback
	move.b 24(a2),d0
	cmp.b 9(a3),d0
	bne.w .fallback
	move.w 22(a2),d0
	cmp.w 10(a3),d0
	bne.w .fallback
	move.w (a2),d4
	asr.w #6,d4
	cmp.w (a3),d4
	bne.w .fallback
	tst.w d4
	bmi.w .fallback
	move.w 2(a2),d7
	asr.w #6,d7
	cmp.w 2(a3),d7
	bne.w .fallback
	tst.w d7
	bmi.w .fallback
	move.l 26(a2),d0
	cmp.l (a3),d0
	bne.w .fallback
	move.w 30(a2),d0
	cmp.w 4(a3),d0
	bne.w .fallback
	moveq #0,d0
	move.b 20(a2),d0
	lsl.w #2,d0
	moveq #0,d1
	move.b 16(a2),d1
	add.w d1,d0
	lea .frames(pc),a0
	moveq #0,d1
	move.b 0(a0,d0.w),d1
	move.w d1,d0
	lsl.w #8,d0
	add.w d1,d0
	add.w d1,d0
	add.w d1,d0
	movea.l 60(sp),a5
	adda.w d0,a5
	tst.b 258(a5)
	beq.w .fallback
	move.w 128(a5),d0
	cmp.w 4(a3),d0
	bne.w .fallback
	moveq #0,d5
	move.b 128(a5),d5
	beq.w .fallback
	moveq #0,d6
	move.b 129(a5),d6
	beq.w .fallback
	move.w d4,d0
	add.w d5,d0
	cmpi.w #320,d0
	bgt.w .fallback
	move.w d7,d0
	add.w d6,d0
	cmpi.w #200,d0
	bgt.w .fallback
	moveq #0,d3
	lea 130(a5),a1
	tst.b 23(a2)
	beq.s .paint
	cmpi.w #190,d7
	bge.s .paint
	movea.l 56(sp),a4
	tst.b 7(a4)
	beq.w .fallback
	move.l (a4),d0
	cmp.l (a3),d0
	bne.w .fallback
	move.w 4(a4),d0
	cmp.w 7(a3),d0
	bne.w .fallback
	move.b 6(a4),d0
	cmp.b 11(a3),d0
	bne.w .fallback
	lea 8(a4),a1
	moveq #1,d3
.paint:
	movea.l 64(sp),a6
	lea mult320,a4
	adda.l 0(a4,d7.w*4),a6
	adda.w d4,a6
	move.l d6,-(sp)
	move.l d5,-(sp)
	move.l a1,-(sp)
	lea 33(a2),a0
	move.l a0,-(sp)
	move.l a5,-(sp)
	move.l a6,-(sp)
	tst.w d3
	beq.s .opaque
	jsr slicks_draw_sprite_visible+SLICKS_SPRITE_TEST_BASE
	bra.s .painted
.opaque:
	jsr slicks_draw_sprite_opaque+SLICKS_SPRITE_TEST_BASE
.painted:
	lea 24(sp),sp
	move.b #1,32(a2)
	clr.b 6(a3)
	moveq #1,d0
	bra.s .return
.fallback:
	moveq #0,d0
.return:
	movem.l (sp)+,d2-d7/a2-a6
	rts
.frames:
	dc.b 0,5,6,7,1,1,1,1,2,8,9,10,3,3,3,3,4,11,12,13
