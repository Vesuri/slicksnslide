	section code,code
	xdef slicks_draw_particle
	xdef slicks_draw_particle_batch

; C ABI: particle, chunky, material, surface, dirty_pixels, dirty_count,
;        mult320. Returns 1 WITHOUT changes if the dirty list needs the
; existing rectangle-overflow path; otherwise returns 0.
; Particle layout is the checked 24-byte particle_runtime.s ABI.
slicks_draw_particle:
	movem.l d2-d5/a2-a6,-(sp)
	movea.l 60(sp),a5
	moveq #0,d5
	move.w (a5),d5
	cmpi.w #510,d5
	bhi.w .fallback
	movea.l 40(sp),a0
	movea.l 44(sp),a1
	movea.l 48(sp),a2
	movea.l 52(sp),a3
	movea.l 56(sp),a4
	lea (a4,d5.l*4),a4
	movea.l 64(sp),a6
	bsr.s .body
	movem.l (sp)+,d2-d5/a2-a6
	rts
.body:
	move.l (a0),d0
	asr.l #6,d0
	move.l 4(a0),d1
	asr.l #6,d1
	move.b 20(a0),d3
	cmpi.w #319,d0
	bhi.s .hidden
	cmpi.w #183,d1
	bhi.s .hidden
	moveq #0,d2
	move.w d1,d2
	move.l (a6,d2.l*4),d2
	moveq #0,d4
	move.w d0,d4
	add.l d4,d2
	tst.b 22(a0)
	beq.s .visible
	moveq #0,d4
	move.b (a2,d2.l),d4
	lsl.w #3,d4
	move.b (a3,d2.l),d3
	andi.w #7,d3
	or.w d3,d4
	moveq #0,d3
	move.b 22(a0),d3
	cmp.w d3,d4
	bhi.s .masked
	move.b 20(a0),d3
	bra.s .visible
.masked:
	move.b 20(a0),d3
.hidden:
	btst #1,d3
	beq.s .clear
	bsr.s .queue_old
.clear:
	clr.b 20(a0)
	bra.s .done
.visible:
	btst #1,d3
	beq.s .queue_new
	cmp.w 12(a0),d0
	bne.s .moved
	cmp.w 14(a0),d1
	beq.s .paint
.moved:
	bsr.s .queue_old
.queue_new:
	move.w d0,(a4)+
	move.b d1,(a4)+
	clr.b (a4)+
	addq.w #1,d5
.paint:
	move.w d0,12(a0)
	move.w d1,14(a0)
	move.b (a1,d2.l),16(a0)
	move.b 18(a0),(a1,d2.l)
	move.b #1,20(a0)
.done:
	move.w d5,(a5)
	moveq #0,d0
	rts
.fallback:
	moveq #1,d0
	movem.l (sp)+,d2-d5/a2-a6
	rts
.queue_old:
	cmpi.w #319,12(a0)
	bhi.s .old_done
	cmpi.w #199,14(a0)
	bhi.s .old_done
	move.w 12(a0),(a4)+
	move.b 15(a0),(a4)+
	clr.b (a4)+
	addq.w #1,d5
.old_done:
	rts

; Same first seven arguments, then sorted particle-index words and count.
; Return number processed; caller handles the suffix on dirty-list overflow.
; Sprite actors split batches so saved-under priority order stays exact.
slicks_draw_particle_batch equ .batch_entry
.batch_entry:
	movem.l d2-d7/a2-a6,-(sp)
	subq.l #8,sp
	move.l 84(sp),(sp)
	move.l 56(sp),4(sp)
	movea.l 60(sp),a1
	movea.l 64(sp),a2
	movea.l 68(sp),a3
	movea.l 72(sp),a4
	movea.l 76(sp),a5
	movea.l 80(sp),a6
	moveq #0,d5
	move.w (a5),d5
	lea (a4,d5.l*4),a4
	move.l 88(sp),d6
	moveq #0,d7
.batch_loop:
	tst.l d6
	beq.s .batch_done
	cmpi.w #510,d5
	bhi.s .batch_done
	movea.l (sp),a0
	moveq #0,d0
	move.w (a0)+,d0
	move.l a0,(sp)
	mulu.w #24,d0
	movea.l 4(sp),a0
	adda.l d0,a0
	bsr.w .body
	addq.l #1,d7
	subq.l #1,d6
	bra.s .batch_loop
.batch_done:
	move.l d7,d0
	addq.l #8,sp
	movem.l (sp)+,d2-d7/a2-a6
	rts
