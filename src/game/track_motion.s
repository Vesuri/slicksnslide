; C callers may tail-call these entries: elf2hunk resolves PC-relative
; references only within one section, so share the compiler .text section.
	section	.text,code
	xdef	slicks_update_track_actor_motion
	xref	slicks_track_actor_probe
	xref	slicks_track_material_sample
	include	"race_offsets.i"

; Exact signed C division of \1 by 100; quotient in \2, \3 scratch.
DIV100	macro
	move.l	\1,\3
	muls.l	#$51eb851f,\2:\3
	asr.l	#5,\2
	add.l	\1,\1
	subx.l	\1,\1
	sub.l	\1,\2
	endm
; Signed C division of word \1 by 16, in place.
DIV16	macro
	tst.w	\1
	bpl.s	.pos\@
	addi.w	#15,\1
.pos\@:
	asr.w	#4,\1
	endm

TF_CARX		equ	0		; four signed pixel x (car->x/100)
TF_CARY		equ	16
TF_LEFT		equ	32		; contact rectangle, or never-skip sentinels
TF_RIGHT	equ	34
TF_TOP		equ	36
TF_BOTTOM	equ	38
TF_NX		equ	40
TF_NY		equ	42
TF_READY	equ	44
TFRAME		equ	46
TARGS		equ	TFRAME+44+4

; C ABI: void slicks_update_track_actor_motion(race)
; Native race_runtime.c update_track_actor_motion(). Stationary objects keep
; a compact cached loop; car setup, off-map samples, moving-object rays (C
; slicks_track_actor_probe) and car contacts are out of line. Errors set
; collision_error and return, as the reference does.
slicks_update_track_actor_motion:
	movem.l	d2-d7/a2-a6,-(sp)
	lea	-TFRAME(sp),sp
	movea.l	TARGS(sp),a4
	movea.l	a4,a3			; navigation.actors[0] at offset 0
	lea	RACE_TRACK_ACTOR_HANDLES(a4),a2
	movea.l	a4,a6
	adda.l	#RACE_MATERIAL_MAP,a6
	lea	RACE_ACTORS(a4),a5
	clr.w	TF_READY(sp)
	move.w	RACE_NAVIGATION_ACTOR_COUNT(a4),d7
	beq.w	.done
	subq.w	#1,d7
.actor:
	moveq	#0,d0
	move.b	TRACK_ACTOR_KIND(a3),d0
	move.w	d0,RACE_TRACK_ACTOR_SCRATCH(a4)
	subq.b	#1,d0
	beq.s	.movable
	subq.b	#2,d0
	bne.w	.next
.movable:
	tst.w	TF_READY(sp)
	beq.w	.cars
.cars_ready:
	move.w	TRACK_ACTOR_X(a3),d2
	add.w	TRACK_ACTOR_VELOCITY_X(a3),d2
	move.w	d2,TF_NX(sp)
	move.w	TRACK_ACTOR_Y(a3),d3
	add.w	TRACK_ACTOR_VELOCITY_Y(a3),d3
	move.w	d3,TF_NY(sp)
	DIV16	d2
	DIV16	d3
	cmpi.w	#320,d2
	bcc.w	.slow_sample
	cmpi.w	#190,d3
	bcc.w	.slow_sample
	mulu.w	#320,d3
	add.w	d2,d3
	moveq	#0,d2
	move.b	(a6,d3.l),d2
.sampled:
	tst.b	d2
	bne.s	.lower
	move.b	#1,TRACK_ACTOR_LAYER(a3)
	bra.s	.layered
.lower:
	cmpi.b	#19,d2
	bne.s	.layered
	clr.b	TRACK_ACTOR_LAYER(a3)
.layered:
	moveq	#0,d6			; moving
	tst.l	TRACK_ACTOR_VELOCITY_X(a3)
	bne.w	.moving
.placed:
	move.w	TRACK_ACTOR_X(a3),d4
	DIV16	d4			; actor pixel x
	move.w	TRACK_ACTOR_Y(a3),d5
	DIV16	d5
	tst.b	RACE_TRACK_ACTORS_READY(a4)
	beq.s	.configured
	moveq	#0,d0
	move.b	(a2),d0
	beq.s	.configured
	cmpi.w	#ACTOR_CAPACITY,d0
	bcc.s	.configured
	lea	RACE_WEAPON_SLOTS(a4),a0
	move.b	#1,(a0,d0.w)
	mulu.w	#ACTOR_SIZE,d0
	lea	(a5,d0.l),a0
	move.w	d4,d1
	subq.w	#2,d1
	lsl.w	#6,d1
	move.w	d1,ACTOR_MOTION_X(a0)
	move.w	d5,d1
	subq.w	#2,d1
	lsl.w	#6,d1
	move.w	d1,ACTOR_MOTION_Y(a0)
	clr.l	ACTOR_MOTION_VX(a0)
	clr.l	ACTOR_MOTION_AX(a0)
	clr.l	ACTOR_MOTION_LIFETIME(a0)
	clr.w	ACTOR_MOTION_FRAME(a0)
	moveq	#1,d1
	cmpi.b	#1,TRACK_ACTOR_KIND(a3)
	beq.s	.priority
	moveq	#4,d1
.priority:
	move.b	d1,ACTOR_PRIORITY(a0)
	move.b	TRACK_ACTOR_LAYER(a3),d1
	mulu.w	#15,d1
	move.b	d1,ACTOR_OCCLUSION(a0)
.configured:
	tst.b	d6
	bne.w	.damp
.contact_check:
	cmp.w	TF_LEFT(sp),d4
	blt.s	.next
	cmp.w	TF_RIGHT(sp),d4
	bgt.s	.next
	cmp.w	TF_TOP(sp),d5
	blt.s	.next
	cmp.w	TF_BOTTOM(sp),d5
	ble.w	.contacts
.next:
	lea	TRACK_ACTOR_SIZE(a3),a3
	addq.l	#1,a2
	dbf	d7,.actor
.done:
	lea	TFRAME(sp),sp
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Out-of-line work follows the cached stationary loop.
.damp:
	move.w	TRACK_ACTOR_VELOCITY_X(a3),d0	; (short)(v*10)/11
	muls.w	#10,d0
	ext.l	d0
	divs.w	#11,d0
	move.w	d0,TRACK_ACTOR_VELOCITY_X(a3)
	move.w	TRACK_ACTOR_VELOCITY_Y(a3),d0
	muls.w	#10,d0
	ext.l	d0
	divs.w	#11,d0
	move.w	d0,TRACK_ACTOR_VELOCITY_Y(a3)
	bra.s	.contact_check
.moving:
	moveq	#1,d6
	move.w	TF_NY(sp),d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	TF_NX+4(sp),d0
	ext.l	d0
	move.l	d0,-(sp)
	move.l	a3,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_track_actor_probe
	lea	16(sp),sp
	moveq	#-2,d1
	cmp.l	d1,d0
	bne.w	.placed
.error:
	move.b	#1,RACE_COLLISION_ERROR(a4)
	bra.s	.done
.slow_sample:
	pea	0.w			; lower layer
	ext.l	d3
	move.l	d3,-(sp)
	ext.l	d2
	move.l	d2,-(sp)
	movea.l	a4,a0
	adda.l	#RACE_SURFACE_MAP,a0
	move.l	a0,-(sp)
	move.l	a6,-(sp)
	jsr	slicks_track_material_sample
	lea	20(sp),sp
	move.l	d0,d2
	bmi.s	.error
	bra.w	.sampled
.contacts:
	lea	RACE_CARS(a4),a0
	moveq	#0,d3
.car:
	move.b	CAR_ACTOR_LAYER(a0),d0
	cmp.b	TRACK_ACTOR_LAYER(a3),d0
	bne.w	.car_next
	move.w	d4,d1
	ext.l	d1
	move.l	TF_CARX(sp,d3.w*4),d0
	sub.l	d1,d0
	bpl.s	.dx
	neg.l	d0
.dx:
	cmpi.w	#4,d0
	bge.s	.car_next
	move.w	d5,d1
	ext.l	d1
	move.l	TF_CARY(sp,d3.w*4),d0
	sub.l	d1,d0
	bpl.s	.dy
	neg.l	d0
.dy:
	cmpi.w	#4,d0
	bge.s	.car_next
	move.l	#300,d0
	cmp.l	CAR_MEASURED_SPEED(a0),d0
	bge.s	.car_next
	move.l	CAR_VELOCITY_X(a0),d0
	DIV100	d0,d1,d2
	move.w	d1,TRACK_ACTOR_VELOCITY_X(a3)
	move.l	CAR_VELOCITY_Y(a0),d0
	DIV100	d0,d1,d2
	move.w	d1,TRACK_ACTOR_VELOCITY_Y(a3)
	moveq	#15,d1
	move.l	CAR_VELOCITY_X(a0),d0
	muls.l	#$7cc9,d0
	asr.l	d1,d0
	move.l	d0,CAR_VELOCITY_X(a0)
	move.l	CAR_VELOCITY_Y(a0),d0
	muls.l	#$7cc9,d0
	asr.l	d1,d0
	move.l	d0,CAR_VELOCITY_Y(a0)
.car_next:
	lea	CAR_SIZE(a0),a0
	addq.w	#1,d3
	cmpi.w	#4,d3
	bcs.w	.car
	bra.w	.next
.cars:
	move.w	#1,TF_READY(sp)
	move.w	#30004,d4		; left/top
	move.w	#-30004,d5		; right/bottom
	move.w	d4,TF_LEFT(sp)
	move.w	d4,TF_TOP(sp)
	move.w	d5,TF_RIGHT(sp)
	move.w	d5,TF_BOTTOM(sp)
	moveq	#1,d6			; wrap-free contact bounds
	lea	RACE_CARS(a4),a0
	moveq	#0,d3
.car_setup:
	move.l	CAR_X(a0),d0
	DIV100	d0,d4,d1
	move.l	d4,TF_CARX(sp,d3.w*4)
	move.l	CAR_Y(a0),d0
	DIV100	d0,d5,d1
	move.l	d5,TF_CARY(sp,d3.w*4)
	move.l	#30000,d0
	cmp.l	d0,d4
	bgt.s	.wrapping
	cmp.l	d0,d5
	bgt.s	.wrapping
	neg.l	d0
	cmp.l	d0,d4
	blt.s	.wrapping
	cmp.l	d0,d5
	bge.s	.bounded
.wrapping:
	moveq	#0,d6
.bounded:
	move.l	#300,d0
	cmp.l	CAR_MEASURED_SPEED(a0),d0
	bge.s	.setup_next
	subq.w	#3,d4
	cmp.w	TF_LEFT(sp),d4
	bge.s	.left
	move.w	d4,TF_LEFT(sp)
.left:
	addq.w	#6,d4
	cmp.w	TF_RIGHT(sp),d4
	ble.s	.right
	move.w	d4,TF_RIGHT(sp)
.right:
	subq.w	#3,d5
	cmp.w	TF_TOP(sp),d5
	bge.s	.top
	move.w	d5,TF_TOP(sp)
.top:
	addq.w	#6,d5
	cmp.w	TF_BOTTOM(sp),d5
	ble.s	.setup_next
	move.w	d5,TF_BOTTOM(sp)
.setup_next:
	lea	CAR_SIZE(a0),a0
	addq.w	#1,d3
	cmpi.w	#4,d3
	bcs.w	.car_setup
	tst.b	d6
	bne.w	.cars_ready
	move.w	#-32768,TF_LEFT(sp)	; any wrap: always test all cars
	move.w	#32767,TF_RIGHT(sp)
	move.w	#-32768,TF_TOP(sp)
	move.w	#32767,TF_BOTTOM(sp)
	bra.w	.cars_ready

	xdef	slicks_advance_weapon_actors
; C ABI: void slicks_advance_weapon_actors(race)
; Native race_runtime.c advance_weapon_actors() for the shared pool (the C
; caller checks the pool). Point slots, free slots and inert reserved slots
; are skipped; retirement and slicks_actor_advance() are exact, including
; page-zero expiry deferral and signed-word motion/age arithmetic.
slicks_advance_weapon_actors:
	movem.l	d2-d7/a2-a3,-(sp)
	movea.l	36(sp),a0
	moveq	#0,d7
	move.w	RACE_WEAPON_SLOTS+SLOTS_HIGH_WATER(a0),d7
	subq.w	#2,d7			; handles 1..high_water-1
	bmi.w	.advanced
	move.b	RACE_ACTOR_PAGE(a0),d6
	lea	RACE_WEAPON_SLOTS+1(a0),a2
	lea	RACE_TRAIL_INDEX+2(a0),a1
	lea	RACE_ACTORS+ACTOR_SIZE(a0),a3
.slot:
	tst.w	(a1)+			; point slots advance with their particle
	bpl.w	.skip
	move.b	(a2),d0
	beq.w	.skip
	tst.b	ACTOR_KIND(a3)
	bne.s	.live
	tst.w	ACTOR_MOTION_LIFETIME(a3)
	beq.w	.skip
.live:
	cmpi.b	#-2,d0
	bne.s	.advance
	clr.b	(a2)
	clr.b	ACTOR_KIND(a3)
	bra.w	.skip
.advance:
	tst.b	d0
	ble.w	.retired
	move.w	ACTOR_MOTION_LIFETIME(a3),d1
	beq.s	.move
	subq.w	#1,d1
	move.w	d1,ACTOR_MOTION_LIFETIME(a3)
	bgt.s	.move
	tst.b	d6
	bne.s	.expire
	move.w	#1,ACTOR_MOTION_LIFETIME(a3)
	bra.s	.move
.expire:
	neg.b	d0
	move.b	d0,(a2)
	bra.s	.retired
.move:
	; All remaining writes are no-ops for this exact state. Lifetime has
	; already been processed above. Test animation first so moving/animated
	; objects take the ordinary path cheaply; no cached ownership assumption.
	tst.l	ACTOR_MOTION_AGE(a3)	; age=0, frame=0, period=0
	bne.s	.moving
	tst.l	ACTOR_MOTION_VX(a3)	; vx=vy=0
	bne.s	.moving
	tst.l	ACTOR_MOTION_AX(a3)	; ax=ay=0
	beq.s	.skip
.moving:
	move.w	ACTOR_MOTION_VX(a3),d1
	add.w	ACTOR_MOTION_AX(a3),d1
	move.w	d1,ACTOR_MOTION_VX(a3)
	add.w	d1,ACTOR_MOTION_X(a3)
	move.w	ACTOR_MOTION_VY(a3),d1
	add.w	ACTOR_MOTION_AY(a3),d1
	move.w	d1,ACTOR_MOTION_VY(a3)
	add.w	d1,ACTOR_MOTION_Y(a3)
	move.b	ACTOR_MOTION_PERIOD(a3),d2
	ext.w	d2
	cmp.w	ACTOR_MOTION_AGE(a3),d2
	bge.s	.frame
	clr.w	ACTOR_MOTION_AGE(a3)
	addq.b	#1,ACTOR_MOTION_FRAME(a3)
.frame:
	move.b	ACTOR_MOTION_FRAME(a3),d1
	ext.w	d1
	moveq	#0,d3
	move.b	ACTOR_MOTION_FRAMES(a3),d3
	cmp.w	d3,d1
	blt.s	.aged
	clr.b	ACTOR_MOTION_FRAME(a3)
.aged:
	tst.b	d2
	beq.s	.retired
	addq.w	#1,ACTOR_MOTION_AGE(a3)
.retired:
	cmpi.b	#-1,(a2)
	bne.s	.skip
	move.b	#-2,(a2)
.skip:
	addq.l	#1,a2
	lea	ACTOR_SIZE(a3),a3
	dbf	d7,.slot
.advanced:
	movem.l	(sp)+,d2-d7/a2-a3
	rts
