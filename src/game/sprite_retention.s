; C callers may tail-call this entry: keep it in the compiler .text section.
	section	.text,code
	xdef	slicks_prepare_sprite_retention
	xref	slicks_retention
	xref	slicks_race_disable_retention
	xref	slicks_retention_release
	xref	slicks_retention_weapons_live
	xref	slicks_retention_rebuild
	xref	slicks_retention_touch
	xref	slicks_retention_cars
	xref	slicks_retention_late
	include	"race_offsets.i"
	ifgt ((ACTOR_CAPACITY-1)*ACTOR_SIZE-32767)
	fail "Retention actor offsets must fit a signed word"
	endif
	ifne (PREV_SIZE-12)
	fail "Update retention previous-descriptor scaled addressing"
	endif

; C ABI: void slicks_prepare_sprite_retention(race)
; Per-update pass of sprite_retention.inc (prepare_sprite_retention_reference
; is the host form). After simulation and before drawing: detect geometry
; changes from raw motion keys, scan drawable points against candidate rows
; and cells, mark car rectangles, then keep or late-restore sprites. Rare
; work (rebuild, touches, late restoration, release) calls the C helpers.
slicks_prepare_sprite_retention:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a4
	cmpi.w	#8,RACE_NAVIGATION_ACTOR_COUNT(a4)	; RETENTION_MIN_CANDIDATES
	bcs.w	.done			; nothing can ever be retained
	tst.b	slicks_race_disable_retention
	bne.w	.release
	tst.b	RACE_RACING(a4)
	beq.w	.release
	tst.b	RACE_TRACK_ACTORS_READY(a4)
	beq.w	.release
	tst.b	RACE_SPRITE_DIRTY_DEFERRED(a4)
	beq.w	.release
	cmpi.w	#5,RACE_RACE_MODE(a4)
	beq.w	.release
	lea	RACE_SHADOWS(a4),a0
	lea	RACE_CARS(a4),a1
	moveq	#3,d0
.shadow:
	tst.b	SHADOW_SAVED_VALID(a0)
	bne.w	.release
	tst.b	SHADOW_STATE(a0)
	bgt.w	.release
	cmpi.w	#500,CAR_SPECIAL_DRIVE_STATE(a1)
	bgt.w	.release
	lea	SHADOW_SIZE(a0),a0
	lea	CAR_SIZE(a1),a1
	dbf	d0,.shadow
	tst.b	RACE_WEAPONS_READY(a4)
	beq.s	.geometry
	move.l	a4,-(sp)
	jsr	slicks_retention_weapons_live
	addq.l	#4,sp
	tst.l	d0
	bne.w	.release
.geometry:
	lea	slicks_retention,a5
	tst.b	RET_VALID(a5)
	beq.s	.reset_done
	lea	RET_ENTRIES+ENTRY_FLAGS(a5),a0
	moveq	#0,d0
	move.b	RET_COUNT(a5),d0
	subq.w	#1,d0
	bmi.s	.reset_done
.reset:
	andi.b	#5,(a0)			; keep candidacy/motion, drop conflicts
	lea	ENTRY_SIZE(a0),a0
	dbf	d0,.reset
.reset_done:
	lea	RET_ENTRIES(a5),a3
	moveq	#0,d6			; listed sprites
	moveq	#0,d5			; bit 0 rebuild, bit 1 a sprite moved
	tst.b	RET_VALID(a5)
	bne.s	.cached
	moveq	#1,d5
.cached:
	btst	#0,d5
	bne.s	.scan_geometry
	tst.b	RET_GEOMETRY_DIRTY(a5)
	bne.s	.scan_geometry
	move.b	RET_COUNT(a5),d6
	bra.w	.geometry_done
.scan_geometry:
	clr.b	RET_GEOMETRY_DIRTY(a5)
	lea	RACE_TRACK_ACTOR_HANDLES(a4),a1
	lea	RACE_ACTORS(a4),a6
	move.w	RACE_NAVIGATION_ACTOR_COUNT(a4),d7
	subq.w	#1,d7
.geo:
	moveq	#0,d1
	move.b	(a1)+,d1
	beq.w	.geo_next
	cmpi.w	#ACTOR_CAPACITY,d1
	bcc.w	.geo_next
	move.w	d1,d0
	mulu.w	#ACTOR_SIZE,d0
	lea	(a6,d0.l),a2
	cmpi.b	#3,ACTOR_KIND(a2)
	bne.w	.unlisted
	lea	RACE_WEAPON_SLOTS(a4),a0
	tst.b	(a0,d1.w)
	ble.w	.unlisted
	moveq	#0,d2
	move.b	ACTOR_ASSET(a2),d2
	cmpi.b	#5,d2
	bcc.w	.unlisted
	cmpi.b	#4,ACTOR_MOTION_FRAME(a2)
	bcc.w	.unlisted
	move.l	ACTOR_MOTION_X(a2),d4	; x:y, fraction bits masked
	andi.l	#$ffc0ffc0,d4
	cmp.b	ENTRY_HANDLE(a3),d1
	bne.s	.geo_new
	move.b	ACTOR_PRIORITY(a2),d0
	cmp.b	ENTRY_PRIORITY(a3),d0
	bne.s	.geo_new
	cmp.b	ENTRY_ASSET(a3),d2
	bne.s	.geo_new
	cmp.l	ENTRY_KEY(a3),d4	; union rectangles ignore animation frames
	beq.s	.geo_listed
	btst	#0,d5
	bne.s	.geo_rekey		; a rebuild follows anyway
	; A moving sprite is no candidate: both its rectangles conflict
	; until it settles and the maps are rebuilt.
	bsr.w	.touch_entry
	move.l	d4,ENTRY_KEY(a3)
	move.l	d4,d0
	swap	d0
	asr.w	#6,d0
	move.w	d0,ENTRY_LEFT(a3)
	lea	RET_KIND_WIDTH(a5),a0
	moveq	#0,d3
	move.b	(a0,d2.w),d3
	add.w	d3,d0
	move.w	d0,ENTRY_RIGHT(a3)
	move.w	d4,d0
	asr.w	#6,d0
	move.w	d0,ENTRY_TOP(a3)
	lea	RET_KIND_HEIGHT(a5),a0
	move.b	(a0,d2.w),d3
	add.w	d3,d0
	move.w	d0,ENTRY_BOTTOM(a3)
	bsr.w	.touch_entry
	move.b	#4,ENTRY_FLAGS(a3)	; RETENTION_MOVED
	move.b	#16,RET_REBUILD_PENDING(a5)	; RETENTION_SETTLE
	bset	#1,d5
	bra.s	.geo_listed
.geo_rekey:
	move.l	d4,ENTRY_KEY(a3)
	bra.s	.geo_listed
.geo_new:
	bset	#0,d5
	move.b	ACTOR_PRIORITY(a2),ENTRY_PRIORITY(a3)
	move.b	d1,ENTRY_HANDLE(a3)
	move.l	d4,ENTRY_KEY(a3)
	move.b	d2,ENTRY_ASSET(a3)
.geo_listed:
	lea	ENTRY_SIZE(a3),a3
	addq.w	#1,d6
	bra.s	.geo_next
.unlisted:
	btst	#1,ACTOR_RETAIN(a2)
	bne.s	.unlisted_kept
	clr.b	ACTOR_RETAIN(a2)
	bra.s	.geo_next
.unlisted_kept:
	move.l	a1,-(sp)		; no longer a live track sprite
	clr.l	-(sp)
	move.l	d1,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_retention_late
	lea	12(sp),sp
	movea.l	(sp)+,a1
.geo_next:
	dbf	d7,.geo
.geometry_done:
	btst	#0,d5
	bne.s	.rebuild
	cmp.b	RET_COUNT(a5),d6
	bne.s	.rebuild
	tst.b	RET_REBUILD_PENDING(a5)
	beq.s	.points
	btst	#1,d5
	bne.s	.moved			; still moving: rebuild once settled
	subq.b	#1,RET_REBUILD_PENDING(a5)
	beq.s	.rebuild
.moved:
	lea	RET_ENTRIES(a5),a3	; moved sprites stay conflict sources
	move.w	d6,d7
	subq.w	#1,d7
.moved_entry:
	btst	#2,ENTRY_FLAGS(a3)
	beq.s	.moved_next
	bsr.w	.touch_entry
.moved_next:
	lea	ENTRY_SIZE(a3),a3
	dbf	d7,.moved_entry
	bra.s	.points
.rebuild:
	bset	#0,d5			; also count changes / settling rebuilds
	move.l	d6,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_retention_rebuild
	addq.l	#8,sp
.points:
	tst.b	RET_CANDIDATES(a5)
	beq.w	.decide
	lea	RACE_TRAIL_PARTICLES(a4),a0
	lea	RET_ROWS(a5),a1
	lea	RET_CELLS(a5),a2
	move.w	RACE_TRAIL_PARTICLE_COUNT(a4),d7
	subq.w	#1,d7
	bmi.w	.cars
.point:
	move.b	PARTICLE_PRIORITY(a0),d4	; only lower-priority points matter
	cmp.b	RET_MAX_PRIORITY(a5),d4
	bcc.s	.point_next
	move.l	PARTICLE_Y(a0),d1	; the drawn row/column words
	asr.l	#6,d1
	cmpi.w	#184,d1
	bcc.s	.point_next
	tst.b	(a1,d1.w)
	beq.s	.point_next
	move.l	PARTICLE_X(a0),d0
	asr.l	#6,d0
	cmpi.w	#320,d0
	bcc.s	.point_next
	move.w	d1,d2
	lsr.w	#3,d2
	mulu.w	#40,d2
	move.w	d0,d3
	lsr.w	#3,d3
	add.w	d3,d2
	moveq	#0,d3
	move.b	(a2,d2.w),d3
	beq.s	.point_next
	cmpi.b	#255,d3
	beq.s	.point_shared
	subq.w	#1,d3			; one candidate: exact test in place
	mulu.w	#ENTRY_SIZE,d3
	lea	RET_ENTRIES(a5,d3.l),a3
	moveq	#3,d3
	and.b	ENTRY_FLAGS(a3),d3
	subq.b	#1,d3
	bne.s	.point_next		; not a candidate, or already touched
	cmp.b	ENTRY_PRIORITY(a3),d4
	bcc.s	.point_next
	cmp.w	ENTRY_LEFT(a3),d0
	blt.s	.point_next
	cmp.w	ENTRY_RIGHT(a3),d0
	bge.s	.point_next
	cmp.w	ENTRY_TOP(a3),d1
	blt.s	.point_next
	cmp.w	ENTRY_BOTTOM(a3),d1
	bge.s	.point_next
	bset	#1,ENTRY_FLAGS(a3)
.point_next:
	lea	PARTICLE_SIZE(a0),a0
	dbf	d7,.point
	bra.s	.cars
.point_shared:
	movem.l	d7/a0-a2,-(sp)
	andi.l	#$ff,d4
	move.l	d4,-(sp)		; point priority
	ext.l	d0
	ext.l	d1
	move.l	d1,d2
	addq.l	#1,d2
	move.l	d2,-(sp)
	move.l	d0,d2
	addq.l	#1,d2
	move.l	d2,-(sp)
	move.l	d1,-(sp)
	move.l	d0,-(sp)
	jsr	slicks_retention_touch
	lea	20(sp),sp
	movem.l	(sp)+,d7/a0-a2
	bra.s	.point_next
.cars:
	move.l	a4,-(sp)
	jsr	slicks_retention_cars
	addq.l	#4,sp
.decide:
	lea	RET_ENTRIES(a5),a3
	moveq	#0,d7
	move.b	RET_COUNT(a5),d7
	subq.w	#1,d7
	bmi.w	.done
	lea	RACE_ACTORS(a4),a6
.decide_loop:
	moveq	#0,d1
	move.b	ENTRY_HANDLE(a3),d1
	move.w	.actor_offsets(pc,d1.w*2),d0
	lea	(a6,d0.w),a2
	moveq	#0,d2			; eligible: candidate without conflict
	moveq	#3,d0
	and.b	ENTRY_FLAGS(a3),d0
	subq.b	#1,d0
	bne.s	.eligibility
	moveq	#1,d2
.eligibility:
	btst	#1,ACTOR_RETAIN(a2)
	beq.s	.set_bits
	tst.b	d2
	beq.s	.late
.previous_address:
	; PREV_SIZE is 12: three times the handle, scaled by four in the EA.
	move.w	d1,d0
	add.w	d0,d0
	add.w	d1,d0
	lea	RACE_SPRITE_DIRTY_PREVIOUS(a4),a0
	lea	(a0,d0.w*4),a0
	; With no rebuild, eligibility implies an unmoved cached entry: .geo
	; already checked its handle, kind, asset and pixel-position key. KEPT
	; came from an unchanged draw, so the previous descriptor has that same
	; geometry. Rebuilt entries retain the full comparison below. Frame,
	; colour and occlusion are not in the geometry key and always need tests.
	btst	#0,d5
	beq.s	.style
	cmpi.b	#3,PREV_KIND(a0)
	bne.s	.late
	move.w	PREV_X(a0),d0
	cmp.w	ENTRY_LEFT(a3),d0
	bne.s	.late
	move.w	PREV_Y(a0),d0
	cmp.w	ENTRY_TOP(a3),d0
	bne.s	.late
	move.b	PREV_ASSET(a0),d0
	cmp.b	ACTOR_ASSET(a2),d0
	bne.s	.late
.style:
	move.b	PREV_FRAME(a0),d0
	cmp.b	ACTOR_MOTION_FRAME(a2),d0
	bne.s	.late
	move.b	PREV_COLOUR(a0),d0
	cmp.b	ACTOR_COLOUR(a2),d0
	bne.s	.late
	move.w	PREV_PRIORITY(a0),d0	; priority and occlusion bytes
	cmp.w	ACTOR_PRIORITY(a2),d0
	bne.s	.late
.set_bits:
	moveq	#3,d0			; RETAIN_NEXT|RETAIN_KEPT
	and.b	ACTOR_RETAIN(a2),d0
	tst.b	d2
	beq.s	.bits
	ori.b	#8,d0			; RETAIN_ELIGIBLE
.bits:
	move.b	d0,ACTOR_RETAIN(a2)
	bra.s	.decide_next
.late:
	move.l	d2,-(sp)
	move.l	d1,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_retention_late
	lea	12(sp),sp
.decide_next:
	lea	ENTRY_SIZE(a3),a3
	dbf	d7,.decide_loop
.done:
	movem.l	(sp)+,d2-d7/a2-a6
	rts
.release:
	move.l	a4,-(sp)
	jsr	slicks_retention_release
	addq.l	#4,sp
	bra.s	.done
; slicks_retention_touch(entry rectangle, -1); preserves all but d0/d1/a0.
.touch_entry:
	move.l	a1,-(sp)
	pea	-1.w
	move.w	ENTRY_BOTTOM(a3),d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	ENTRY_RIGHT(a3),d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	ENTRY_TOP(a3),d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	ENTRY_LEFT(a3),d0
	ext.l	d0
	move.l	d0,-(sp)
	jsr	slicks_retention_touch
	lea	20(sp),sp
	movea.l	(sp)+,a1
	rts

; Every retained entry was validated against the 200-slot actor pool.
; 199*164 fits a signed word; keep the table outside the decision loop.
.actor_offsets:
actor_offset_value set 0
	rept ACTOR_CAPACITY
	dc.w actor_offset_value
actor_offset_value set actor_offset_value+ACTOR_SIZE
	endr
