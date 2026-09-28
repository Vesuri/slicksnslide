; Experimental 20-byte shared particle record. Not linked into the game.
; x.w,y.w,vx.w,vy.w,old_x.w,old_y.w,saved/life/colour/priority,
; saved_valid/permanent/occlusion/state. All coordinate arithmetic wraps
; exactly as the DOS words. C ABI is the existing nine-argument shared pass.
	section code,code
	xdef slicks_advance_compact_particles
slicks_advance_compact_particles:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	move.l 52(sp),d7
	movea.l 56(sp),a1
	movea.l 60(sp),a2
	movea.l 64(sp),a3
	movea.l a0,a4
	suba.l a5,a5
	suba.l a6,a6
	moveq #0,d6
	subq.w #1,d7
	bcs.s .done
.point:
	movem.l (a0),d0-d4
	move.b (a1,a5.w),d6
	tst.b d4
	bmi.s .release
	swap d3
	tst.b d3
	beq.s .motion
	subq.b #1,d3
	beq.s .expire
.motion:
	add.w d1,d0
	swap d0
	swap d1
	add.w d1,d0
	swap d0
	swap d1
	swap d3
	cmpa.l a5,a6
	bne.s .copy
	move.l d0,(a0)
	move.l d3,12(a0)
	bra.s .publish
.copy:
	movem.l d0-d4,(a4)
	move.b d6,(a1,a6.w)
.publish:
	move.w a6,(a2,d6.w*2)
	move.b d4,(a3,d6.w)
	addq.w #1,a6
	lea 20(a4),a4
.next:
	addq.w #1,a5
	lea 20(a0),a0
	dbf d7,.point
.done:
	move.w a6,d0
	movem.l (sp)+,d2-d7/a2-a6
	rts
.release:
	clr.b (a3,d6.w)
	move.w #-1,(a2,d6.w*2)
	bra.s .next
.expire:
	tst.l 80(sp)
	bne.s .retire
	move.b #1,d3
	bra.s .motion
.retire:
	swap d3
	move.l a1,-(sp)
	move.l d4,d0
	swap d0
	move.b #-2,d4
	tst.b d0
	beq.s .retire_pixel
	move.b #-6,d4
.retire_pixel:
	btst #25,d4
	beq.s .retired
	tst.b d0
	beq.s .queue
	moveq #0,d0
	move.w d2,d0
	mulu.w #320,d0
	move.l d2,d5
	swap d5
	and.l #$ffff,d5
	add.l d5,d0
	movea.l 80(sp),a1
	move.w d3,d5
	lsr.w #8,d5
	move.b d5,(a1,d0.l)
.queue:
	movea.l 76(sp),a1
	moveq #0,d0
	move.w (a1),d0
	cmpi.w #512,d0
	bcc.s .retired
	addq.w #1,(a1)
	movea.l 72(sp),a1
	move.l d2,d5
	lsl.w #8,d5
	move.l d5,(a1,d0.l*4)
.retired:
	andi.l #$00ffffff,d4
	movea.l (sp)+,a1
	movem.l (a0),d0-d1
	movem.l d0-d4,(a4)
	cmpa.l a5,a6
	beq.w .publish
	move.b d6,(a1,a6.w)
	bra.w .publish
