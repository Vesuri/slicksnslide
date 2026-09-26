	section code,code
	xdef slicks_draw_car_chunky
	xdef slicks_restore_car_chunky
; C ABI: destination, source, saved, material, surface, width, height,
; source_dx, source_dy, colour_ramp_offset, mask_limit.
; Caller supplies an in-bounds rectangle and the rotated source origin.
slicks_draw_car_chunky:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a1
	movea.l 52(sp),a0
	movea.l 56(sp),a2
	movea.l 60(sp),a3
	movea.l 64(sp),a4
	move.w 70(sp),d0
	beq.w .done
	move.w 74(sp),d1
	beq.w .done
	subq.w #1,d1
	move.w 78(sp),d3
	move.w d0,d4
	muls.w d3,d4
	neg.l d4
	movea.w 82(sp),a6
	add.l a6,d4
	moveq #0,d6
	move.b 91(sp),d6
	; Compare material first; only equality needs the surface's low 3 bits.
	; This is exactly (material*8 + (surface&7)) <= original mask.
	; Keep the original byte for the special mask=0 unmasked path.
	move.w d6,d5
	lsr.w #3,d5
	movea.w d5,a5
	andi.w #7,d6
	; Track/effect frames are contiguous and need no player colour ramp.
	; Keep rotated/recoloured car sprites on the general path below.
	tst.b 87(sp)
	bne.s .general
	cmpi.w #1,d3
	bne.s .general
	cmpa.w d0,a6
	beq.w .contiguous
.general:
	tst.b 91(sp)
	bne.s .masked_row
.row:
	move.w d0,d7
	subq.w #1,d7
.pixel:
	move.b (a1),(a2)+
	move.b (a0),d2
	beq.s .transparent
	cmpi.b #5,d2
	bhi.s .write
	add.b 87(sp),d2
.write:
	move.b d2,(a1)
.transparent:
	adda.w d3,a0
	addq.l #1,a1
	dbf d7,.pixel
	adda.l d4,a0
	adda.w #320,a1
	suba.w d0,a1
	dbf d1,.row
	bra.s .done
.masked_row:
	move.w d0,d7
	subq.w #1,d7
.masked_pixel:
	move.b (a1),(a2)+
	move.b (a0),d2
	beq.s .masked_skip
	moveq #0,d5
	move.b (a3),d5
	cmpa.w d5,a5
	blo.s .masked_skip
	bhi.s .masked_colour
	move.b (a4),d5
	andi.w #7,d5
	cmp.w d6,d5
	bhi.s .masked_skip
.masked_colour:
	cmpi.b #5,d2
	bhi.s .masked_write
	add.b 87(sp),d2
.masked_write:
	move.b d2,(a1)
.masked_skip:
	adda.w d3,a0
	addq.l #1,a1
	addq.l #1,a3
	addq.l #1,a4
	dbf d7,.masked_pixel
	adda.l d4,a0
	adda.w #320,a1
	suba.w d0,a1
	adda.w #320,a3
	suba.w d0,a3
	adda.w #320,a4
	suba.w d0,a4
	dbf d1,.masked_row
.done:
	movem.l (sp)+,d2-d7/a2-a6
	rts

.contiguous:
	tst.b 91(sp)
	bne.s .contiguous_masked_row
.contiguous_row:
	move.w d0,d7
	subq.w #1,d7
.contiguous_pixel:
	move.b (a1),(a2)+
	move.b (a0)+,d3
	beq.s .contiguous_skip
	move.b d3,(a1)
.contiguous_skip:
	addq.l #1,a1
	dbf d7,.contiguous_pixel
	adda.w #320,a1
	suba.w d0,a1
	dbf d1,.contiguous_row
	bra.w .done
.contiguous_masked_row:
	move.w d0,d7
	subq.w #1,d7
.contiguous_masked_pixel:
	move.b (a1),(a2)+
	move.b (a0)+,d3
	beq.s .contiguous_masked_skip
	moveq #0,d5
	move.b (a3),d5
	cmpa.w d5,a5
	blo.s .contiguous_masked_skip
	bhi.s .contiguous_masked_write
	move.b (a4),d5
	andi.w #7,d5
	cmp.w d6,d5
	bhi.s .contiguous_masked_skip
.contiguous_masked_write:
	move.b d3,(a1)
.contiguous_masked_skip:
	addq.l #1,a1
	addq.l #1,a3
	addq.l #1,a4
	dbf d7,.contiguous_masked_pixel
	adda.w #320,a1
	suba.w d0,a1
	adda.w #320,a3
	suba.w d0,a3
	adda.w #320,a4
	suba.w d0,a4
	dbf d1,.contiguous_masked_row
	bra.w .done

; C ABI: destination, saved, width, height. Caller clips before saving.
slicks_restore_car_chunky:
	movem.l d2-d4,-(sp)
	movea.l 16(sp),a0
	movea.l 20(sp),a1
	move.w 26(sp),d0
	beq.s .done
	move.w 30(sp),d1
	beq.s .done
	subq.w #1,d1
	move.w #320,d3
	sub.w d0,d3
	move.w d0,d4
	andi.w #3,d4
	lsr.w #2,d0
	subq.w #1,d0
.row:
	move.w d0,d2
	bmi.s .tail
.long:
	move.l (a1)+,(a0)+
	dbf d2,.long
.tail:
	btst #1,d4
	beq.s .byte
	move.w (a1)+,(a0)+
.byte:
	btst #0,d4
	beq.s .next_row
	move.b (a1)+,(a0)+
.next_row:
	adda.w d3,a0
	dbf d1,.row
.done:
	movem.l (sp)+,d2-d4
	rts
