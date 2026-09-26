	section code,code
	xdef slicks_draw_sprite_opaque
; C ABI: destination, source, saved, opacity, width, height.
; In-bounds, no foreground mask or colour ramp. Save every background byte
; while merging four original source pixels at a time, with exact tails.
slicks_draw_sprite_opaque:
	movem.l d2-d5/a2-a3,-(sp)
	movea.l 28(sp),a0
	movea.l 32(sp),a1
	movea.l 36(sp),a2
	movea.l 40(sp),a3
	move.w 46(sp),d4
	move.w 50(sp),d3
	bsr.w slicks_draw_sprite_opaque_regs
	movem.l (sp)+,d2-d5/a2-a3
	rts

	xdef slicks_draw_sprite_opaque_regs
; Private register ABI for callers already preserving the render registers.
; a0 destination, a1 pixels, a2 saved, a3 opacity, d4 width, d3 height.
; Clobbers d0-d5/a0-a3; no nested C argument frame or duplicate saves.
slicks_draw_sprite_opaque_regs:
	tst.w d4
	beq.w .done
	tst.w d3
	beq.w .done
	subq.w #1,d3
	move.w #320,d2
	sub.w d4,d2
	move.w d4,d5
	andi.w #3,d5
	lsr.w #2,d4
	subq.w #1,d4
.row:
	move.w d4,d0
	bmi.s .tail
.longs:
	move.l (a0),d1
	move.l d1,(a2)+
	and.l (a3)+,d1
	or.l (a1)+,d1
	move.l d1,(a0)+
	dbf d0,.longs
.tail:
	btst #1,d5
	beq.s .byte
	move.w (a0),d1
	move.w d1,(a2)+
	and.w (a3)+,d1
	or.w (a1)+,d1
	move.w d1,(a0)+
.byte:
	btst #0,d5
	beq.s .next
	move.b (a0),d1
	move.b d1,(a2)+
	and.b (a3)+,d1
	or.b (a1)+,d1
	move.b d1,(a0)+
.next:
	adda.w d2,a0
	dbf d3,.row
.done:
	rts

	xdef slicks_draw_sprite_visible
; Same ABI, but mask $ff copies source and zero preserves background.
slicks_draw_sprite_visible:
	movem.l d2-d6/a2-a3,-(sp)
	movea.l 32(sp),a0
	movea.l 36(sp),a1
	movea.l 40(sp),a2
	movea.l 44(sp),a3
	move.w 50(sp),d4
	move.w 54(sp),d3
	bsr.w slicks_draw_sprite_visible_regs
	movem.l (sp)+,d2-d6/a2-a3
	rts

	xdef slicks_draw_sprite_visible_regs
; Same private register ABI; also clobbers d6.
slicks_draw_sprite_visible_regs:
	tst.w d4
	beq.w .done
	tst.w d3
	beq.w .done
	subq.w #1,d3
	move.w #320,d2
	sub.w d4,d2
	move.w d4,d5
	andi.w #3,d5
	lsr.w #2,d4
	subq.w #1,d4
.row:
	move.w d4,d0
	bmi.s .tail
.longs:
	move.l (a0),d1
	move.l d1,(a2)+
	move.l (a1)+,d6
	eor.l d1,d6
	and.l (a3)+,d6
	eor.l d6,d1
	move.l d1,(a0)+
	dbf d0,.longs
.tail:
	btst #1,d5
	beq.s .byte
	move.w (a0),d1
	move.w d1,(a2)+
	move.w (a1)+,d6
	eor.w d1,d6
	and.w (a3)+,d6
	eor.w d6,d1
	move.w d1,(a0)+
.byte:
	btst #0,d5
	beq.s .next
	move.b (a0),d1
	move.b d1,(a2)+
	move.b (a1)+,d6
	eor.b d1,d6
	and.b (a3)+,d6
	eor.b d6,d1
	move.b d1,(a0)+
.next:
	adda.w d2,a0
	dbf d3,.row
.done:
	rts
