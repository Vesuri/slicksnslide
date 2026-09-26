	section	code,code
	xdef	slicks_integrate_car_motion
	xref	slicks_resolve_track_velocity
	xref	slicks_car_direction_x
	xref	slicks_car_direction_y
	include	"race_offsets.i"

; Exact signed C truncating division by 100/2000 (GCC's reciprocal forms).
; \1 dividend (clobbered), \2 quotient, \3 scratch.
DIV100	macro
	move.l	\1,\3
	muls.l	#$51eb851f,\2:\3
	asr.l	#5,\2
	add.l	\1,\1
	subx.l	\1,\1
	sub.l	\1,\2
	endm
DIV2000	macro
	move.l	\1,\3
	muls.l	#$10624dd3,\2:\3
	asr.l	#7,\2
	add.l	\1,\1
	subx.l	\1,\1
	sub.l	\1,\2
	endm

; One step of move_car_through_track's ray (steps 1..count).
; \1/\2 x/y registers (minor d0 and major d5, by orientation), \3 nonzero
; for the upper layer. d2 dbf counter, d3 step*delta_minor low word, d4
; delta_minor, d5 major coordinate, d6 |major delta|, d7 major sign, a0 map
; base + 32768, a1 old minor, a2/a3 clear x/y, a6 boundary_level-1.
PROBE	macro
.step\@:
	add.w	d7,d5
	add.w	d4,d3
	move.w	d3,d0
	ext.l	d0
	divs.w	d6,d0
	neg.w	d0
	add.w	a1,d0
	if	\3
	cmpi.w	#320,\1
	bcc.w	.sample_error
	cmpi.w	#190,\2
	bcc.w	.sample_error
	endif
	move.w	\2,d1
	mulu.w	#320,d1
	add.w	\1,d1
	if	\3==0
	cmpi.w	#60800,d1
	bcc.w	.sample_error
	endif
	eori.w	#$8000,d1
	move.b	(a0,d1.w),d1
	cmpi.b	#2,d1
	beq.w	.hit
	subi.b	#22,d1
	cmpi.b	#4,d1
	bhi.s	.clear\@
	ext.w	d1
	cmp.w	a6,d1
	ble.w	.hit
.clear\@:
	movea.w	\1,a2
	movea.w	\2,a3
	dbf	d2,.step\@
	bra.w	.no_hit
	endm

FRAME		equ	10
F_OLDX		equ	0
F_OLDY		equ	2
F_QUANTUM	equ	4
F_TIMESTEP	equ	6
F_ACTIVE	equ	8
ARGS		equ	FRAME+44+4

; C ABI: void slicks_integrate_car_motion(race, car, timestep, active_drive)
; Native form of race_runtime.c integrate_car_motion(), including
; move_car_through_track(), record_track_contact() and clamp_car_to_track().
; Rare blocked-ray responses call the C resolve_track_velocity().
slicks_integrate_car_motion:
	movem.l	d2-d7/a2-a6,-(sp)
	lea	-FRAME(sp),sp
	movea.l	ARGS(sp),a4
	movea.l	ARGS+4(sp),a5
	move.w	ARGS+10(sp),F_TIMESTEP(sp)
	move.b	ARGS+15(sp),F_ACTIVE(sp)
	clr.w	F_QUANTUM(sp)
.quantum:
	move.w	F_QUANTUM(sp),d0
	cmp.w	F_TIMESTEP(sp),d0
	bge.w	.done
	move.l	CAR_X(a5),d0
	DIV100	d0,d1,d2
	move.w	d1,F_OLDX(sp)
	move.l	CAR_Y(a5),d0
	DIV100	d0,d1,d2
	move.w	d1,F_OLDY(sp)
	move.w	CAR_SPECIAL_DRIVE_STATE(a5),d0
	bgt.w	.special_forward
	blt.w	.special_reverse
	tst.b	F_ACTIVE(sp)
	bne.s	.forces
	moveq	#0,d1			; coast: unsigned Q15 speed decay
	move.w	#$7bdd,d1
	sub.w	CAR_DRIVE_BIAS(a5),d1
	move.l	CAR_SPEED_FIXED(a5),d0
	muls.l	d1,d0
	moveq	#15,d2
	asr.l	d2,d0
	move.l	d0,CAR_SPEED_FIXED(a5)
.forces:
	move.w	CAR_DRIVE_COEFFICIENTS(a5),d3
	muls.w	CAR_DRIVE_COEFFICIENTS+6(a5),d3
	move.w	CAR_DAMAGE(a5),d1
	ext.l	d1
	divs.w	#70,d1
	ext.l	d1
	moveq	#10,d0
	add.l	d0,d1
	muls.l	d1,d3
	moveq	#23,d1
	tst.b	F_ACTIVE(sp)
	beq.s	.divisor
	moveq	#38,d1
.divisor:
	muls.l	d1,d3			; force divisor
	moveq	#0,d0
	move.w	CAR_HEADING(a5),d0
	divu.w	#1200,d0
	lea	slicks_car_direction_x,a0
	move.b	(a0,d0.w),d4
	extb.l	d4
	lea	slicks_car_direction_y,a0
	move.b	(a0,d0.w),d5
	extb.l	d5
	move.l	CAR_SPEED_FIXED(a5),d0
	muls.l	d0,d4
	muls.l	#200,d4
	divs.l	d3,d4			; force x
	muls.l	d0,d5
	muls.l	#200,d5
	divs.l	d3,d5			; force y
	moveq	#0,d1
	move.w	#$7bd7,d1
	tst.b	F_ACTIVE(sp)
	beq.s	.factor
	move.w	#$7dc2,d1
.factor:
	sub.w	CAR_DRIVE_BIAS(a5),d1
	move.w	CAR_DRIVE_COEFFICIENTS+2(a5),d2
	ext.l	d2
	addi.l	#$8000,d2
	move.l	CAR_VELOCITY_X(a5),d0
	muls.l	d1,d0
	divs.l	d2,d0
	add.l	d4,d0
	move.l	d0,CAR_VELOCITY_X(a5)
	move.l	CAR_VELOCITY_Y(a5),d0
	muls.l	d1,d0
	divs.l	d2,d0
	add.l	d5,d0
	move.l	d0,CAR_VELOCITY_Y(a5)
	bra.s	.position
.special_forward:
	moveq	#0,d1
	move.w	#$7ffc,d1
	sub.w	CAR_DRIVE_BIAS(a5),d1
	moveq	#15,d2
	move.l	CAR_VELOCITY_X(a5),d0
	muls.l	d1,d0
	asr.l	d2,d0
	move.l	d0,CAR_VELOCITY_X(a5)
	move.l	CAR_VELOCITY_Y(a5),d0
	muls.l	d1,d0
	asr.l	d2,d0
	move.l	d0,CAR_VELOCITY_Y(a5)
	bra.s	.position
.special_reverse:
	moveq	#0,d1
	move.w	#$7bdd,d1
	sub.w	CAR_DRIVE_BIAS(a5),d1
	move.l	CAR_SPEED_FIXED(a5),d0
	muls.l	d1,d0
	moveq	#15,d2
	asr.l	d2,d0
	move.l	d0,CAR_SPEED_FIXED(a5)
.position:
	moveq	#0,d3
	move.b	CAR_POSITION_SCALE(a5),d3
	move.l	CAR_VELOCITY_X(a5),d0
	muls.l	d3,d0
	DIV2000	d0,d1,d2
	add.l	d1,CAR_X(a5)
	move.l	CAR_VELOCITY_Y(a5),d0
	muls.l	d3,d0
	DIV2000	d0,d1,d2
	add.l	d1,CAR_Y(a5)
; move_car_through_track
	move.l	CAR_X(a5),d0
	DIV100	d0,d4,d1
	move.l	CAR_Y(a5),d0
	DIV100	d0,d5,d1
	move.w	F_OLDX(sp),d2
	sub.w	d4,d2			; delta x
	move.w	F_OLDY(sp),d3
	sub.w	d5,d3			; delta y
	move.w	d2,d4
	bpl.s	.abs_x
	neg.w	d4
.abs_x:
	move.w	d3,d5
	bpl.s	.abs_y
	neg.w	d5
.abs_y:
	cmp.w	d4,d5
	bgt.s	.y_count
	tst.w	d4
	beq.w	.no_hit
	bra.s	.counted
.y_count:
	tst.w	d5
	beq.w	.no_hit
.counted:
	tst.w	d4
	bmi.w	.sample_error
	tst.w	d5
	bmi.w	.sample_error
	; Every sample returns zero with a special state or disabled sampling.
	tst.w	CAR_SPECIAL_DRIVE_STATE(a5)
	bne.w	.no_hit
	tst.b	CAR_COLLISION_SAMPLING(a5)
	beq.w	.no_hit
	move.w	RACE_BOUNDARY_LEVEL(a4),d0
	subq.w	#1,d0
	movea.w	d0,a6
	movea.w	F_OLDX(sp),a2
	movea.w	F_OLDY(sp),a3
	movea.l	a4,a0
	; Step zero can only report an invalid sample; it never blocks.
	tst.b	CAR_ACTOR_LAYER(a5)
	bne.s	.upper_origin
	adda.l	#RACE_MATERIAL_MAP+32768,a0
	move.w	a3,d1
	mulu.w	#320,d1
	add.w	a2,d1
	cmpi.w	#60800,d1
	bcc.w	.sample_error
	bra.s	.origin_ok
.upper_origin:
	adda.l	#RACE_SURFACE_MAP+32768,a0
	cmpa.w	#320,a2
	bcc.w	.sample_error
	cmpa.w	#190,a3
	bcc.w	.sample_error
.origin_ok:
	cmp.w	d4,d5
	bgt.w	.y_major
	move.w	d4,d6			; X-major divisor |dx|
	moveq	#-1,d7
	tst.w	d2
	bpl.s	.x_sign
	moveq	#1,d7
.x_sign:
	move.w	d3,d4			; minor delta y
	moveq	#0,d3
	movea.w	F_OLDY(sp),a1
	move.w	F_OLDX(sp),d5
	move.w	d6,d2
	subq.w	#1,d2
	tst.b	CAR_ACTOR_LAYER(a5)
	bne.w	.x_upper
	PROBE	d5,d0,0
.x_upper:
	PROBE	d5,d0,1
.y_major:
	move.w	d5,d6			; Y-major divisor |dy|
	moveq	#-1,d7
	tst.w	d3
	bpl.s	.y_sign
	moveq	#1,d7
.y_sign:
	move.w	d2,d4			; minor delta x
	moveq	#0,d3
	movea.w	F_OLDX(sp),a1
	move.w	F_OLDY(sp),d5
	move.w	d6,d2
	subq.w	#1,d2
	tst.b	CAR_ACTOR_LAYER(a5)
	bne.w	.y_upper
	PROBE	d0,d5,0
.y_upper:
	PROBE	d0,d5,1
.hit:
	move.l	a3,-(sp)
	move.l	a2,-(sp)
	move.l	a5,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_resolve_track_velocity
	lea	16(sp),sp
	move.w	a3,F_QUANTUM(sp)	; both ray outputs alias the counter
	tst.b	CAR_TOUCHING_SOLID(a5)
	bne.s	.touching
	addq.l	#1,RACE_TRACK_COLLISION_COUNT(a4)
.touching:
	move.b	#1,CAR_ACTOR_CONTACT(a5)
	move.b	#1,CAR_TOUCHING_SOLID(a5)
	bra.s	.clamp
.sample_error:
	move.b	#1,RACE_COLLISION_ERROR(a4)
.no_hit:
	clr.b	CAR_TOUCHING_SOLID(a5)
.clamp:
	moveq	#0,d1
	move.w	#$7dd4,d1
	sub.w	CAR_DRIVE_BIAS(a5),d1
	moveq	#15,d2
	move.l	CAR_X(a5),d0
	cmpi.l	#300,d0
	blt.s	.x_low
	cmpi.l	#31700,d0
	ble.s	.x_ok
	move.l	#31700,CAR_X(a5)
	bra.s	.x_damp
.x_low:
	move.l	#300,CAR_X(a5)
.x_damp:
	bsr.s	.damp
.x_ok:
	move.l	CAR_Y(a5),d0
	cmpi.l	#300,d0
	blt.s	.y_low
	cmpi.l	#17900,d0
	ble.s	.y_ok
	move.l	#17900,CAR_Y(a5)
	bra.s	.y_damp
.y_low:
	move.l	#300,CAR_Y(a5)
.y_damp:
	bsr.s	.damp
.y_ok:
	addq.w	#1,F_QUANTUM(sp)
	bra.w	.quantum
.damp:
	move.l	CAR_VELOCITY_X(a5),d0
	muls.l	d1,d0
	asr.l	d2,d0
	move.l	d0,CAR_VELOCITY_X(a5)
	move.l	CAR_VELOCITY_Y(a5),d0
	muls.l	d1,d0
	asr.l	d2,d0
	move.l	d0,CAR_VELOCITY_Y(a5)
	rts
.done:
	lea	FRAME(sp),sp
	movem.l	(sp)+,d2-d7/a2-a6
	rts
