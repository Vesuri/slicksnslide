	section	code,code
	xdef	slicks_advance_particles
	xref	mult320

; C ABI:
; unsigned short slicks_advance_particles(particles, count, indices, counts,
;                                         dirty_pixels, dirty_count, chunky,
;                                         actor_page)
; SlicksTrailParticle is deliberately 24 bytes on m68k:
; x.l, y.l, vx.w, vy.w, old_x.w, old_y.w, saved/flags bytes, padding.
slicks_advance_particles:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0		; source particle
	move.l	52(sp),d7		; source count
	movea.l	56(sp),a1		; four 256-byte index buckets
	movea.l	60(sp),a2		; four word counts
	movea.l	64(sp),a3		; dirty pixel entries
	movea.l	68(sp),a4		; dirty pixel count
	clr.l	(a2)
	clr.l	4(a2)
	movea.l	a0,a6			; compacted destination
	moveq	#0,d6			; source index
	moveq	#0,d5			; destination index
	tst.w	d7
	beq.w	.done
	subq.w	#1,d7
.particle:
	tst.b	23(a0)			; -2/-6 release in this pass's retirement
	bmi.w	.next_source
	tst.b	17(a0)			; DOS zero lifetime is unlimited
	beq.s	.motion
	subq.b	#1,17(a0)
	bne.s	.motion
	tst.l	76(sp)			; DOS page zero defers expiry
	bne.s	.retire
	move.b	#1,17(a0)
	bra.s	.motion
.retire:
	move.b	#-2,23(a0)		; state 1 -> -1 -> -2 on expiry pass
	tst.b	21(a0)
	beq.s	.retire_pixel
	move.b	#-6,23(a0)		; state 5 -> -5 -> -6
.retire_pixel:
	btst	#1,20(a0)
	beq.w	.retain_retired
	tst.b	21(a0)			; permanent DOS state-5 mark
	beq.s	.queue_expiry
	moveq	#0,d0
	move.w	14(a0),d0
	lea	mult320,a5
	move.l	(a5,d0.w*4),d0
	moveq	#0,d1
	move.w	12(a0),d1
	add.l	d1,d0
	movea.l	72(sp),a5		; authoritative chunky surface
	move.b	18(a0),0(a5,d0.l)
.queue_expiry:
	moveq	#0,d0
	move.w	(a4),d0
	cmpi.w	#512,d0
	bcc.w	.retain_retired
	move.w	12(a0),0(a3,d0.l*4)
	move.b	15(a0),2(a3,d0.l*4)
	clr.b	3(a3,d0.l*4)
	addq.w	#1,(a4)
	bra.s	.retain_retired
.motion:
	move.w	8(a0),d0
	add.w	2(a0),d0		; DOS ADD word, then signed storage
	ext.l	d0
	move.l	d0,(a0)
	move.w	10(a0),d0
	add.w	6(a0),d0
	ext.l	d0
	move.l	d0,4(a0)
.survives:
	cmp.w	d6,d5
	beq.s	.no_copy
	movem.l	(a0),d0-d4/a5
	movem.l	d0-d4/a5,(a6)
.no_copy:
	moveq	#0,d4
	move.b	19(a6),d0
	beq.s	.bucket_ready
	moveq	#1,d4
	cmpi.b	#3,d0
	beq.s	.bucket_ready
	moveq	#2,d4
	cmpi.b	#5,d0
	beq.s	.bucket_ready
	moveq	#3,d4
.bucket_ready:
	moveq	#0,d0
	move.w	0(a2,d4.l*2),d0
	move.l	d4,d1
	lsl.w	#8,d1
	add.w	d0,d1
	move.b	d5,0(a1,d1.l)
	addq.w	#1,0(a2,d4.l*2)
	bra.s	.keep_entry
.retain_retired:
	clr.b	20(a0)			; no next-frame restore/draw for dead point
	cmp.w	d6,d5
	beq.s	.keep_entry
	movem.l	(a0),d0-d4/a5
	movem.l	d0-d4/a5,(a6)
.keep_entry:
	addq.w	#1,d5
	adda.w	#24,a6
.next_source:
	addq.w	#1,d6
	adda.w	#24,a0
	dbf	d7,.particle
.done:
	move.w	d5,d0
	movem.l	(sp)+,d2-d7/a2-a6
	rts
