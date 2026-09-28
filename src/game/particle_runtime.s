	section	code,code
	xdef	slicks_advance_particles
	xref	mult320
	ifnd SLICKS_PARTICLE_WORD_COORDINATES
SLICKS_PARTICLE_WORD_COORDINATES equ 0
	endif
	ifne SLICKS_PARTICLE_WORD_COORDINATES
PA_SIZE equ 20
PA_OLD_X equ 8
PA_OLD_Y equ 10
PA_OLD_Y_LOW equ 11
PA_LIFE equ 13
PA_COLOUR equ 14
PA_PRIORITY equ 15
PA_FLAGS equ 16
PA_PERMANENT equ 17
PA_STATE equ 19
	else
PA_SIZE equ 24
PA_OLD_X equ 12
PA_OLD_Y equ 14
PA_OLD_Y_LOW equ 15
PA_LIFE equ 17
PA_COLOUR equ 18
PA_PRIORITY equ 19
PA_FLAGS equ 20
PA_PERMANENT equ 21
PA_STATE equ 23
	endif

; C ABI:
; unsigned short slicks_advance_particles(particles, count, indices, counts,
;                                         dirty_pixels, dirty_count, chunky,
;                                         actor_page)
; Null counts/indices omit legacy priority buckets for the shared actor pool.
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
	move.l	a2,d0
	beq.s	.counts_ready
	clr.l	(a2)
	clr.l	4(a2)
.counts_ready:
	movea.l	a0,a6			; compacted destination
	moveq	#0,d6			; source index
	moveq	#0,d5			; destination index
	tst.w	d7
	beq.w	.done
	subq.w	#1,d7
.particle:
	tst.b	PA_STATE(a0)			; -2/-6 release in this pass's retirement
	bmi.w	.next_source
	tst.b	PA_LIFE(a0)			; DOS zero lifetime is unlimited
	beq.s	.motion
	subq.b	#1,PA_LIFE(a0)
	bne.s	.motion
	tst.l	76(sp)			; DOS page zero defers expiry
	bne.w	.retire
	move.b	#1,PA_LIFE(a0)
.motion:
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	move.w	4(a0),d0
	add.w	d0,(a0)
	move.w	6(a0),d0
	add.w	d0,2(a0)
	else
	move.w	8(a0),d0
	add.w	2(a0),d0		; DOS ADD word, then signed storage
	ext.l	d0
	move.l	d0,(a0)
	move.w	10(a0),d0
	add.w	6(a0),d0
	ext.l	d0
	move.l	d0,4(a0)
	endif
.survives:
	cmp.w	d6,d5
	beq.s	.no_copy
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	movem.l	(a0),d0-d4
	movem.l	d0-d4,(a6)
	else
	movem.l	(a0),d0-d4/a5
	movem.l	d0-d4/a5,(a6)
	endif
.no_copy:
	cmpa.w	#0,a2
	bne.w	.legacy_buckets
.keep_entry:
	addq.w	#1,d5
	adda.w	#PA_SIZE,a6
.next_source:
	addq.w	#1,d6
	adda.w	#PA_SIZE,a0
	dbf	d7,.particle
.done:
	move.w	d5,d0
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Cold retirement and legacy buckets live outside the shared-pool motion
; loop so its instruction fetches do not evict each other from the I-cache.
.retire:
	move.b	#-2,PA_STATE(a0)		; state 1 -> -1 -> -2 on expiry pass
	tst.b	PA_PERMANENT(a0)
	beq.s	.retire_pixel
	move.b	#-6,PA_STATE(a0)		; state 5 -> -5 -> -6
.retire_pixel:
	btst	#1,PA_FLAGS(a0)
	beq.s	.retain_retired
	tst.b	PA_PERMANENT(a0)			; permanent DOS state-5 mark
	beq.s	.queue_expiry
	moveq	#0,d0
	move.w	PA_OLD_Y(a0),d0
	lea	mult320,a5
	move.l	(a5,d0.w*4),d0
	moveq	#0,d1
	move.w	PA_OLD_X(a0),d1
	add.l	d1,d0
	movea.l	72(sp),a5		; authoritative chunky surface
	move.b	PA_COLOUR(a0),0(a5,d0.l)
.queue_expiry:
	moveq	#0,d0
	move.w	(a4),d0
	cmpi.w	#512,d0
	bcc.s	.retain_retired
	move.w	PA_OLD_X(a0),0(a3,d0.l*4)
	move.b	PA_OLD_Y_LOW(a0),2(a3,d0.l*4)
	clr.b	3(a3,d0.l*4)
	addq.w	#1,(a4)
.retain_retired:
	clr.b	PA_FLAGS(a0)			; no next-frame restore/draw for dead point
	cmp.w	d6,d5
	beq.w	.keep_entry
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	movem.l	(a0),d0-d4
	movem.l	d0-d4,(a6)
	else
	movem.l	(a0),d0-d4/a5
	movem.l	d0-d4/a5,(a6)
	endif
	bra.w	.keep_entry
.legacy_buckets:
	moveq	#0,d4
	move.b	PA_PRIORITY(a6),d0
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
	bra.w	.keep_entry

	xdef	slicks_advance_shared_particles

; C ABI:
; unsigned short slicks_advance_shared_particles(particles, count, handles,
;     indices, states, dirty_pixels, dirty_count, chunky, actor_page)
; Shared actor-pool advance_trail_particles() in one pass. Points retired on
; the previous pass release their slot (state 0, index -1); the rest move or
; retire exactly as slicks_advance_particles, and each survivor's compacted
; record, handle, trail index and slot state are published together. Each
; record is read once and written once. Records past the new count are left
; untouched (the in-place reference also advances those dead records).
slicks_advance_shared_particles:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0		; source record
	move.l	52(sp),d7
	movea.l	56(sp),a1		; handles by trail index
	movea.l	60(sp),a2		; trail index words by handle
	movea.l	64(sp),a3		; slot states by handle
	movea.l	a0,a4			; destination record
	suba.l	a5,a5			; source index
	suba.l	a6,a6			; destination index
	moveq	#0,d6			; handle, zero-extended for indexing
	subq.w	#1,d7
	bcs.s	.shared_done
.shared_point:
	movem.l	(a0),d0-d5		; x, y, vx:vy, old, saved..priority, flags..state
	move.b	(a1,a5.w),d6
	tst.b	d5
	bmi.s	.shared_release
	swap	d4			; low byte: lifetime (zero is unlimited)
	tst.b	d4
	beq.s	.shared_motion
	subq.b	#1,d4
	beq.s	.shared_expire
.shared_motion:
	swap	d2			; DOS ADD word, then signed storage
	add.w	d2,d0
	ext.l	d0
	swap	d2
	add.w	d2,d1
	ext.l	d1
	swap	d4
.shared_keep:
	cmpa.l	a5,a6
	bne.s	.shared_copy
	movem.l	d0-d1,(a0)
	move.l	d4,16(a0)
	bra.s	.shared_publish
.shared_copy:
	movem.l	d0-d5,(a4)
	move.b	d6,(a1,a6.w)
.shared_publish:
	move.w	a6,(a2,d6.w*2)
	move.b	d5,(a3,d6.w)
	addq.w	#1,a6
	lea	24(a4),a4
.shared_next:
	addq.w	#1,a5
	lea	24(a0),a0
	dbf	d7,.shared_point
.shared_done:
	move.w	a6,d0
	movem.l	(sp)+,d2-d7/a2-a6
	rts
.shared_release:
	clr.b	(a3,d6.w)
	move.w	#-1,(a2,d6.w*2)
	bra.s	.shared_next
.shared_expire:
	tst.l	80(sp)			; DOS page zero defers expiry
	bne.s	.shared_retire
	move.b	#1,d4
	bra.s	.shared_motion
; Cold: x/y stay unmoved, the lifetime stays zero. Borrow a1 and d0/d1.
.shared_retire:
	swap	d4
	move.l	a1,-(sp)		; arguments now at 52(sp)
	move.l	d5,d0
	swap	d0			; d0.b: permanent
	move.b	#-2,d5			; state 1 -> -1 -> -2 on expiry pass
	tst.b	d0
	beq.s	.shared_retire_pixel
	move.b	#-6,d5			; state 5 -> -5 -> -6
.shared_retire_pixel:
	btst	#25,d5			; saved_valid bit 1: old display pixel
	beq.s	.shared_retired
	tst.b	d0			; permanent DOS state-5 mark
	beq.s	.shared_queue
	moveq	#0,d0
	move.w	d3,d0			; old_y
	mulu.w	#320,d0
	moveq	#0,d1
	swap	d3
	move.w	d3,d1			; old_x
	swap	d3
	add.l	d1,d0
	movea.l	80(sp),a1		; authoritative chunky surface
	move.w	d4,d1
	lsr.w	#8,d1			; colour
	move.b	d1,(a1,d0.l)
.shared_queue:
	movea.l	76(sp),a1		; dirty count
	moveq	#0,d0
	move.w	(a1),d0
	cmpi.w	#512,d0
	bcc.s	.shared_retired
	addq.w	#1,(a1)
	movea.l	72(sp),a1		; dirty pixels: old_x.w, old_y.b, 0
	move.l	d3,d1
	lsl.w	#8,d1
	move.l	d1,(a1,d0.l*4)
.shared_retired:
	andi.l	#$00ffffff,d5		; no next-frame restore/draw for dead point
	movea.l	(sp)+,a1
	movem.l	(a0),d0-d1
	movem.l	d0-d5,(a4)		; a4 is a0 unless compacting
	cmpa.l	a5,a6
	beq.w	.shared_publish
	move.b	d6,(a1,a6.w)
	bra.w	.shared_publish
