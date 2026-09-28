	section code,code
	xdef slicks_draw_unchanged_track_sprite
	xref slicks_draw_sprite_opaque
	xref slicks_draw_sprite_visible
	xref slicks_draw_sprite_opaque_regs
	xref slicks_draw_sprite_visible_regs
	xref mult320
	xref slicks_draw_animated_track_sprite_publish_regs
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
	beq.s .disabled
	movea.l 48(sp),a2
	movea.l 52(sp),a3
	movea.l 56(sp),a4
	movea.l 60(sp),a5
	movea.l 64(sp),a6
	bsr.w slicks_draw_unchanged_track_sprite_regs
	bra.s .return
.disabled:
	moveq #0,d0
.return:
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Private ABI: a2 actor, a3 previous, a4 visibility, a5 assets, a6 chunky.
; Deferred drawing only. Clobbers d0-d7/a0-a6; the batch owns register saves.
slicks_draw_unchanged_track_sprite_regs:
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
	lea mult320,a4
	adda.l 0(a4,d7.w*4),a6
	adda.w d4,a6
	movea.l a6,a0
	movea.l a2,a4
	movea.l a3,a6
	movea.l a1,a3
	movea.l a5,a1
	lea 36(a4),a2
	move.w d5,d4
	tst.w d3
	beq.s .opaque
	move.w d6,d3
	jsr slicks_draw_sprite_visible_regs+SLICKS_SPRITE_TEST_BASE
	bra.s .painted
.opaque:
	move.w d6,d3
	jsr slicks_draw_sprite_opaque_regs+SLICKS_SPRITE_TEST_BASE
.painted:
	move.b #1,32(a4)
	clr.b 6(a6)
	moveq #1,d0
	bra.s .return
.fallback:
	moveq #0,d0
.return:
	rts
.frames:
	dc.b 0,5,6,7,1,1,1,1,2,8,9,10,3,3,3,3,4,11,12,13

	xdef slicks_restore_actor_sprite
; C ABI: actor, previous description, chunky. Called only with deferred
; sprite dirtiness. Reject clipped/empty/unsaved rectangles without writes.
; Copy the original saved background and capture the same twelve metadata
; bytes as the scalar renderer; the caller appends its handle to the list.
slicks_restore_actor_sprite:
	movem.l d2-d4/a2-a3,-(sp)
	movea.l 24(sp),a2
	movea.l 28(sp),a3
	movea.l 32(sp),a0
	bsr.w slicks_restore_actor_sprite_regs
	movem.l (sp)+,d2-d4/a2-a3
	rts

; Private ABI: a2 actor, a3 previous descriptor, a0 chunky base.
; Clobbers d0-d4/a0-a2; preserves d5-d7/a3-a6 for batched traversal.
slicks_restore_actor_sprite_regs:
	tst.b 32(a2)
	beq.w .fallback
	move.w 26(a2),d0
	bmi.w .fallback
	move.w 28(a2),d1
	bmi.w .fallback
	moveq #0,d4
	move.b 30(a2),d4
	beq.w .fallback
	moveq #0,d2
	move.b 31(a2),d2
	beq.w .fallback
	move.w d0,d3
	add.w d4,d3
	cmpi.w #320,d3
	bhi.w .fallback
	move.w d1,d3
	add.w d2,d3
	cmpi.w #200,d3
	bhi.w .fallback
	lea mult320,a1
	adda.l 0(a1,d1.w*4),a0
	adda.w d0,a0
	lea 36(a2),a1
	move.w d2,d1
	subq.w #1,d1
	move.w #320,d3
	sub.w d4,d3
	move.w d4,d0
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
	beq.s .next
	move.b (a1)+,(a0)+
.next:
	adda.w d3,a0
	dbf d1,.row
	move.l 26(a2),(a3)
	move.w 30(a2),4(a3)
	move.b 21(a2),6(a3)
	move.b 20(a2),7(a3)
	move.b 16(a2),8(a3)
	move.b 24(a2),9(a3)
	move.w 22(a2),10(a3)
	clr.b 32(a2)
	moveq #1,d0
	bra.s .return
.fallback:
	moveq #0,d0
.return:
	rts

	xdef slicks_restore_sprite_chain
; C ABI: actors, previous descriptors, chunky, next handles, trail indices,
; first handle, dirty handles, dirty count. Deferred restoration only.
; Restore consecutive in-bounds sprites; return the first point/clipped/
; unsaved handle without changing it, or zero when the chain is exhausted.
slicks_restore_sprite_chain:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a5
	move.l 52(sp),d6
	movea.l 56(sp),a6
	movea.l 60(sp),a4
	move.l 64(sp),d7
	move.l 68(sp),d5
.actor:
	tst.w d5
	beq.w .done
	movea.l d7,a0
	tst.w 0(a0,d5.w*2)
	bpl.w .done
	move.w slicks_sprite_actor_offsets(pc,d5.w*2),d0
	movea.l a5,a2
	adda.w d0,a2
	move.w d5,d0
	add.w d0,d0
	add.w d5,d0
	lsl.w #2,d0
	movea.l d6,a3
	adda.w d0,a3
	; sprite_retention.inc: RETAIN_NEXT keeps the saved sprite on screen.
	; NEXT is granted only by unchanged drawing (or a validated kept draw).
	; That preserves every previous-description field except its kind marker,
	; which unchanged painting clears to suppress deferred dirty rectangles.
	; No pixels, no dirty entry, and no redundant metadata copies.
	btst #0,33(a2)
	beq.s .restore_now
	tst.b 32(a2)
	beq.s .restore_now
	move.b 21(a2),6(a3)
	move.b #2,33(a2)		; RETAIN_KEPT
	bra.s .restored
.restore_now:
	clr.b 33(a2)
	movea.l a6,a0
	bsr.w slicks_restore_actor_sprite_regs
	tst.l d0
	beq.s .done
	movea.l 76(sp),a0
	moveq #0,d0
	move.w (a0),d0
	addq.w #1,(a0)
	movea.l 72(sp),a1
	move.b d5,0(a1,d0.w)
.restored:
	move.b 0(a4,d5.w),d5
	andi.w #255,d5
	bra.w .actor
.done:
	move.l d5,d0
	movem.l (sp)+,d2-d7/a2-a6
	rts

	xdef slicks_draw_sprite_chain
; C ABI: actors, previous descriptors, visibility cache, assets, chunky,
; next handles, trail indices, first handle, validated drawing packets, race.
; A null race disables animation-change publication (isolated legacy oracle).
; Caller supplies an active,
; ordered chain and deferred sprite dirtiness. Return first general/point
; handle unchanged, or zero when the chain is exhausted.
slicks_draw_sprite_chain:
	movem.l d2-d7/a2-a6,-(sp)
	move.l 76(sp),-(sp)
.actor:
	move.l (sp),d0
	beq.w .done
	movea.l 76(sp),a0
	tst.w 0(a0,d0.w*2)
	bpl.w .done
	move.w slicks_sprite_actor_offsets(pc,d0.w*2),d1
	movea.l 52(sp),a2
	adda.w d1,a2
	btst #1,33(a2)		; RETAIN_KEPT: pixels and background remain
	beq.s .not_kept
	move.b #1,33(a2)	; RETAIN_NEXT
	bra.w .next
.not_kept:
	move.w d0,d1
	add.w d1,d1
	add.w d0,d1
	lsl.w #2,d1
	movea.l 56(sp),a3
	adda.w d1,a3
	andi.w #63,d0
	move.w d0,d1
	lsl.w #4,d0
	add.w d1,d0
	lsl.w #3,d0
	movea.l 60(sp),a4
	adda.w d0,a4
	move.l (sp),d0
	andi.w #63,d0
	move.w slicks_sprite_packet_offsets(pc,d0.w*2),d0
	movea.l 84(sp),a0
	adda.w d0,a0
	tst.b 33(a0)
	beq.w .slow
	move.l (a3),d0
	cmp.l (a0),d0
	bne.w .slow
	move.l 4(a3),d0
	cmp.l 4(a0),d0
	bne.w .slow
	move.l 8(a3),d0
	cmp.l 8(a0),d0
	bne.w .slow
	move.l 26(a2),d0
	cmp.l (a0),d0
	bne.w .slow
	move.w 30(a2),d0
	cmp.w 4(a0),d0
	bne.w .slow
	move.l 20(a2),d0
	cmp.l 12(a0),d0
	bne.w .slow
	move.l (a2),d0
	andi.l #$ffc0ffc0,d0
	cmp.l 16(a0),d0
	bne.w .slow
	move.b 16(a2),d0
	cmp.b 8(a0),d0
	bne.w .slow
	move.b 24(a2),d0
	cmp.b 9(a0),d0
	bne.w .slow
	tst.b 32(a0)
	beq.s .cached
	; The shared foreground mask may have been replaced by an aliased
	; handle. Its complete key must still match before reusing its pointer.
	move.l (a4),d0
	cmp.l 36(a0),d0
	bne.s .slow
	move.l 4(a4),d0
	cmp.l 40(a0),d0
	bne.s .slow
.cached:
	movea.l a2,a4
	movea.l a3,a6
	movea.l 20(a0),a1
	movea.l 24(a0),a3
	lea 36(a4),a2
	moveq #0,d4
	move.b 4(a0),d4
	moveq #0,d3
	move.b 5(a0),d3
	movea.l 28(a0),a0
.opaque:
	cmpi.w #5,d4
	beq.w .opaque5
	cmpi.w #8,d4
	beq.w .opaque8
	cmpi.w #4,d4
	beq.w .opaque4
	jsr slicks_draw_sprite_opaque_regs+SLICKS_SPRITE_TEST_BASE
.painted:
	move.b #1,32(a4)
	clr.b 6(a6)
.retain:
	moveq #0,d0		; RETAIN_NEXT only if prepared as eligible
	btst #3,33(a4)
	beq.s .retained
	moveq #1,d0
.retained:
	move.b d0,33(a4)
	bra.w .next
.slow:
	movea.l 64(sp),a5
	movea.l 68(sp),a6
	bsr.w slicks_draw_unchanged_track_sprite_regs
	tst.l d0
	beq.w .animation
	; Publish a packet only after the existing exact dispatcher validates
	; and draws this sprite. a4=actor, a6=previous, a5=source base;
	; a0/a3 point one rectangle past the destination/mask respectively.
	move.l (sp),d0
	andi.w #63,d0
	move.w slicks_sprite_packet_offsets(pc,d0.w*2),d0
	movea.l 84(sp),a2
	adda.w d0,a2
	clr.w 32(a2)
	move.l (a6),(a2)
	move.l 4(a6),4(a2)
	move.l 8(a6),8(a2)
	move.b #3,6(a2)
	move.l 20(a4),12(a2)
	move.l (a4),d0
	andi.l #$ffc0ffc0,d0
	move.l d0,16(a2)
	move.l a5,20(a2)
	moveq #0,d0
	moveq #0,d1
	move.b 30(a4),d0
	move.b 31(a4),d1
	mulu.w d1,d0
	suba.l d0,a3
	move.l a3,24(a2)
	lea mult320,a1
	suba.l 0(a1,d1.w*4),a0
	move.l a0,28(a2)
	clr.w 34(a2)
	clr.l 36(a2)
	clr.l 40(a2)
	tst.b 23(a4)
	beq.s .publish
	cmpi.w #190,28(a4)
	bge.s .publish
	move.b #1,32(a2)
	move.l (sp),d0
	andi.w #63,d0
	move.w d0,d1
	lsl.w #4,d0
	add.w d1,d0
	lsl.w #3,d0
	movea.l 60(sp),a1
	adda.w d0,a1
	move.l (a1),36(a2)
	move.l 4(a1),40(a2)
	; Pre-mask immutable source pixels once, so the repeated merge uses
	; the same AND/OR loop as an opaque sprite. Never cache backgrounds.
	moveq #0,d2
	moveq #0,d1
	move.b 4(a2),d2
	move.b 5(a2),d1
	mulu.w d1,d2
	addq.w #3,d2
	lsr.w #2,d2
	subq.w #1,d2
	lea 44(a2),a0
	lea 172(a2),a1
	move.l a0,20(a2)
	move.l a1,24(a2)
.clip_source:
	move.l (a3)+,d0
	move.l (a5)+,d1
	and.l d0,d1
	move.l d1,(a0)+
	not.l d0
	move.l d0,(a1)+
	dbf d2,.clip_source
.publish:
	move.b #1,33(a2)
	bra.w .retain
.animation:
	; Cheap necessary gates avoid setting up the new path for the hidden
	; setup flag, moved unchanged-frame sprites and other general actors.
	cmpi.b #3,6(a3)
	bne.w .done
	move.b 16(a2),d0
	cmp.b 8(a3),d0
	beq.w .done
	movea.l 88(sp),a0
	move.l a0,d0
	beq.w .done
	; Failed unchanged validation leaves actor/previous in a2/a3.
	; Reset the asset base: validation may have advanced a5 before rejecting.
	movea.l 64(sp),a5
	movea.l 68(sp),a6
	move.l (sp),d0
	andi.w #63,d0
	move.w slicks_sprite_packet_offsets(pc,d0.w*2),d0
	movea.l 84(sp),a1
	adda.w d0,a1
	jsr slicks_draw_animated_track_sprite_publish_regs+SLICKS_SPRITE_TEST_BASE
	tst.l d0
	beq.w .done
	; Publication already consumed the descriptor and invalidated the packet.
	; Changed frames do not acquire NEXT retention, matching the general draw.
.next:
	movea.l 72(sp),a0
	move.l (sp),d0
	moveq #0,d1
	move.b 0(a0,d0.w),d1
	move.l d1,(sp)
	bra.w .actor
.done:
	move.l (sp)+,d0
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Validated cached rectangles only. Exact row widths avoid the generic
; longword-count and tail tests on every row; register contracts are unchanged.
.opaque5:
	subq.w #1,d3
.opaque5_row:
	move.l (a0),d1
	move.l d1,(a2)+
	and.l (a3)+,d1
	or.l (a1)+,d1
	move.l d1,(a0)+
	move.b (a0),d1
	move.b d1,(a2)+
	and.b (a3)+,d1
	or.b (a1)+,d1
	move.b d1,(a0)+
	adda.w #315,a0
	dbf d3,.opaque5_row
	bra.w .painted
.opaque4:
	subq.w #1,d3
.opaque4_row:
	move.l (a0),d1
	move.l d1,(a2)+
	and.l (a3)+,d1
	or.l (a1)+,d1
	move.l d1,(a0)+
	adda.w #316,a0
	dbf d3,.opaque4_row
	bra.w .painted
.opaque8:
	subq.w #1,d3
.opaque8_row:
	move.l (a0),d1
	move.l d1,(a2)+
	and.l (a3)+,d1
	or.l (a1)+,d1
	move.l d1,(a0)+
	move.l (a0),d1
	move.l d1,(a2)+
	and.l (a3)+,d1
	or.l (a1)+,d1
	move.l d1,(a0)+
	adda.w #312,a0
	dbf d3,.opaque8_row
	bra.w .painted

; Active actor handles are 1..199; packet keys are explicitly masked to 63.
; Both byte offsets fit the signed word consumed by ADDA.W above.
slicks_sprite_actor_offsets:
sprite_actor_offset set 0
	rept 200
	dc.w sprite_actor_offset
sprite_actor_offset set sprite_actor_offset+164
	endr
slicks_sprite_packet_offsets:
sprite_packet_offset set 0
	rept 64
	dc.w sprite_packet_offset
sprite_packet_offset set sprite_packet_offset+300
	endr
