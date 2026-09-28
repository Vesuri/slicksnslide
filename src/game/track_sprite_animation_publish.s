	section code,code
	xdef slicks_draw_animated_track_sprite_publish
	xdef slicks_draw_animated_track_sprite_publish_regs
	xref slicks_draw_animated_track_sprite_regs
	xref slicks_mark_dirty_rect
	ifnd SLICKS_SPRITE_TEST_BASE
SLICKS_SPRITE_TEST_BASE equ 0
	endif

; Candidate integration entry. Same preconditions as the drawing core.
; C ABI: race, actor, previous descriptor, assets, chunky, aliased packet.
; Success publishes both dirty rectangles and invalidates the cached packet.
slicks_draw_animated_track_sprite_publish:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	movea.l 52(sp),a2
	movea.l 56(sp),a3
	movea.l 60(sp),a5
	movea.l 64(sp),a6
	movea.l 68(sp),a1
	bsr.w slicks_draw_animated_track_sprite_publish_regs
	movem.l (sp)+,d2-d7/a2-a6
	rts

; a0 race, a1 packet, a2 actor, a3 previous, a5 assets, a6 chunky.
; Outer chain owns saves. All registers except sp may be clobbered.
slicks_draw_animated_track_sprite_publish_regs:
	movem.l a0-a1,-(sp)
	jsr slicks_draw_animated_track_sprite_regs+SLICKS_SPRITE_TEST_BASE
	tst.l d0
	beq.s .rejected
	movea.l (sp)+,a5
	movea.l (sp)+,a1
	clr.b 33(a1)		; never reuse a packet prepared for the old frame
	movea.l a6,a0		; previous rectangle remains after kind was cleared
	bsr.s .publish_rect
	lea 26(a4),a0		; current x/y/width/height, possibly resized
	bsr.s .publish_rect
	moveq #1,d0
	rts
.rejected:
	addq.l #8,sp
	rts

; a0 -> signed x/y words, unsigned width/height bytes. a5 race.
; Native dirty-rectangle ABI preserves a4-a6, including the actor/previous.
.publish_rect:
	moveq #0,d0
	move.b 5(a0),d0
	add.w 2(a0),d0
	ext.l d0
	move.l d0,-(sp)
	moveq #0,d0
	move.b 4(a0),d0
	add.w (a0),d0
	ext.l d0
	move.l d0,-(sp)
	move.w 2(a0),d0
	ext.l d0
	move.l d0,-(sp)
	move.w (a0),d0
	ext.l d0
	move.l d0,-(sp)
	move.l a5,-(sp)
	jsr slicks_mark_dirty_rect+SLICKS_SPRITE_TEST_BASE
	lea 20(sp),sp
	rts
