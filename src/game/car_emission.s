; C callers may tail-call these entries: elf2hunk resolves PC-relative
; references only within one section, so share the compiler .text section.
	section	.text,code
	xdef	slicks_emit_wheel_surface
	xref	slicks_track_material_sample
	xref	slicks_actor_allocate_native
	xref	slicks_race_disable_particles
	include	"race_offsets.i"
	ifnd SLICKS_PARTICLE_WORD_COORDINATES
SLICKS_PARTICLE_WORD_COORDINATES equ 0
	endif
	ifne SLICKS_PARTICLE_WORD_COORDINATES
EP_SIZE equ 20
EP_LIFETIME equ 13
EP_X equ 0
EP_Y equ 2
EP_VELOCITY_X equ 4
EP_COLOUR equ 14
EP_SAVED_VALID equ 16
	else
EP_SIZE equ PARTICLE_SIZE
EP_LIFETIME equ PARTICLE_LIFETIME
EP_X equ PARTICLE_X
EP_Y equ PARTICLE_Y
EP_VELOCITY_X equ PARTICLE_VELOCITY_X
EP_COLOUR equ PARTICLE_COLOUR
EP_SAVED_VALID equ PARTICLE_SAVED_VALID
	endif

; Stack frame; leaf subroutines address it with SUB added for their return.
EF_BASEX	equ	0		; (short)(car->x/100) - 3
EF_BASEY	equ	2
EF_WHEEL	equ	4
EF_ROAD		equ	6		; road-surface cloud/mark decision
EF_LAYER	equ	7
EF_OCC		equ	8		; (layer*15) << 8, OR-ed into bytes 20..23
EF_COLOUR	equ	10
EF_LONG		equ	11
EF_SX		equ	12
EF_SY		equ	14
EF_BEFORE	equ	16
EFRAME		equ	18
EARGS		equ	EFRAME+44+4
SUB		equ	4

; C ABI: void slicks_emit_wheel_surface(race, car, controls)
; Native race_runtime.c emit_wheel_surface() for the shared actor pool,
; including emit_offroad_wheel(), add_trail_component() and the wheel sound.
; RNG draws, allocation and particle records match the reference exactly.
; New particles receive their final occlusion limit when created.
slicks_emit_wheel_surface:
	movem.l	d2-d7/a2-a6,-(sp)
	lea	-EFRAME(sp),sp
	movea.l	EARGS(sp),a4
	movea.l	EARGS+4(sp),a5
	tst.b	CAR_FORWARD_DRIVE_LATCH(a5)
	beq.w	.done
	tst.w	CAR_SPECIAL_DRIVE_STATE(a5)
	bne.w	.done
	move.l	CAR_VELOCITY_X(a5),d0	; magnitude = (|vx|+|vy|)/2
	bpl.s	.abs_x
	neg.l	d0
.abs_x:
	move.l	CAR_VELOCITY_Y(a5),d1
	bpl.s	.abs_y
	neg.l	d1
.abs_y:
	add.l	d1,d0
	bpl.s	.halve
	addq.l	#1,d0
.halve:
	asr.l	#1,d0
	movea.l	d0,a6
	moveq	#0,d3			; road threshold
	move.b	CAR_VEHICLE(a5),d3
	mulu.w	#PROPERTY_SIZE,d3
	movea.l	a4,a0
	adda.l	#RACE_PROPERTIES,a0
	adda.l	d3,a0
	moveq	#0,d3
	move.b	PROPERTY_EFFECT_PROFILE(a0),d3
	clr.b	EF_ROAD(sp)
	move.b	EARGS+11(sp),d0
	andi.b	#CONTROL_ACCELERATE|CONTROL_BRAKE,d0
	cmpi.b	#CONTROL_BRAKE,d0
	bne.s	.not_brake
	move.l	d3,d1			; brake: magnitude > threshold*2
	add.l	d1,d1
	cmpa.l	d1,a6
	ble.s	.decided
	bra.s	.road
.not_brake:
	cmpi.b	#CONTROL_ACCELERATE,d0
	bne.s	.decided
	btst	#0,CAR_SERVICE_FLAGS(a5)
	bne.s	.decided
	move.w	CAR_DAMAGE(a5),d1	; (short)((short)(t*(10-d/100))*10)/10
	ext.l	d1
	divs.w	#100,d1
	moveq	#10,d0
	sub.w	d1,d0
	muls.w	d3,d0
	muls.w	#10,d0
	ext.l	d0
	divs.w	#10,d0
	ext.l	d0
	cmpa.l	d0,a6
	bge.s	.decided
.road:
	move.b	#1,EF_ROAD(sp)
.decided:
	moveq	#0,d0			; wheel geometry for the heading
	move.w	CAR_HEADING(a5),d0
	divu.w	#1200,d0
	moveq	#3,d1
	and.w	d0,d1
	lsr.w	#2,d0
	add.w	d0,d0
	moveq	#0,d2
	move.b	CAR_VEHICLE(a5),d2
	lsl.w	#2,d2
	add.w	d1,d2
	mulu.w	#SPRITE_SIZE,d2
	movea.l	a4,a2
	adda.l	#RACE_SPRITES+SPRITE_WHEEL_X,a2
	adda.l	d2,a2
	adda.w	d0,a2			; &wheel_x[rotation][0]
	move.l	CAR_X(a5),d0
	move.l	d0,d2
	muls.l	#$51eb851f,d1:d2
	asr.l	#5,d1
	add.l	d0,d0
	subx.l	d0,d0
	sub.l	d0,d1
	subq.w	#3,d1
	move.w	d1,EF_BASEX(sp)
	move.l	CAR_Y(a5),d0
	move.l	d0,d2
	muls.l	#$51eb851f,d1:d2
	asr.l	#5,d1
	add.l	d0,d0
	subx.l	d0,d0
	sub.l	d0,d1
	subq.w	#3,d1
	move.w	d1,EF_BASEY(sp)
	moveq	#0,d0
	move.b	CAR_ACTOR_LAYER(a5),d0
	move.b	d0,EF_LAYER(sp)
	mulu.w	#15,d0
	andi.w	#$ff,d0
	lsl.w	#8,d0
	move.w	d0,EF_OCC(sp)
	clr.w	EF_WHEEL(sp)
.wheel:
	move.w	EF_WHEEL(sp),d0
	move.b	(a2,d0.w),d4
	bmi.w	.next_wheel
	move.b	SPRITE_WHEEL_Y-SPRITE_WHEEL_X(a2,d0.w),d5
	ext.w	d4
	add.w	EF_BASEX(sp),d4		; x
	ext.w	d5
	add.w	EF_BASEY(sp),d5		; y
	move.w	d4,d0
	move.w	d5,d1
	bsr.w	.sample
	tst.l	d0
	bmi.w	.sample_error
	cmpi.l	#31,d0
	bhi.w	.next_wheel
	move.b	.classes(pc,d0.w),d0
	beq.w	.next_wheel
	subq.b	#1,d0
	beq.s	.road_surface
	subq.b	#1,d0
	beq.w	.water
	bra.w	.offroad
.classes:
	dc.b	1,1,0,3,3,4,4,2,2,4,4,5,5,4,4,0
	dc.b	0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1
.road_surface:
	tst.b	EF_ROAD(sp)
	beq.w	.next_wheel
	moveq	#3,d0
	bsr.w	.random
	moveq	#70,d1
	add.b	d0,d1
	move.b	d1,EF_COLOUR(sp)
	moveq	#15,d0
	bsr.w	.random
	moveq	#5,d3
	add.b	d0,d3			; cloud lifetime
	moveq	#22,d0
	bsr.w	.random
	subi.w	#11,d0
	move.w	d0,d2
	swap	d2
	moveq	#22,d0
	bsr.w	.random
	subi.w	#11,d0
	move.w	d0,d2			; vx:vy
	move.w	#(218<<8)|6,d1
	bsr.w	.add
	cmpa.l	#100,a6
	ble.s	.road_mark
	moveq	#3,d0
	bsr.w	.random
	addq.b	#2,d0
	bsr.w	.sound
.road_mark:
	moveq	#0,d1
	move.b	EF_COLOUR(sp),d1
	lsl.w	#8,d1
	moveq	#0,d2
	moveq	#3,d3
	bsr.w	.add
	bra.w	.next_wheel
.water:
	cmpa.l	#300,a6
	ble.w	.next_wheel
	moveq	#20,d0
	bsr.w	.random
	subi.w	#10,d0
	move.w	d0,d2
	swap	d2
	moveq	#20,d0
	bsr.w	.random
	subi.w	#10,d0
	move.w	d0,d2
	move.w	#(55<<8)|3,d1
	moveq	#20,d3
	bsr.w	.add
	bra.w	.next_wheel
.offroad:
	subq.b	#1,d0			; 0: 67, 1: 61, 2: 64 and long-lived
	move.b	d0,d7
	moveq	#3,d0
	bsr.w	.random
	addi.b	#67,d0
	clr.b	EF_LONG(sp)
	subq.b	#1,d7
	bmi.s	.colour
	subq.b	#6,d0
	subq.b	#1,d7
	bmi.s	.colour
	addq.b	#3,d0
	move.b	#1,EF_LONG(sp)
.colour:
	move.b	d0,EF_COLOUR(sp)
	cmpa.l	#200,a6
	ble.w	.next_wheel
	move.l	a6,d0
	divs.l	#120,d0
	move.w	d0,d7			; radius
	moveq	#0,d0
	move.w	d7,d0
	bsr.w	.random
	add.w	d4,d0
	move.w	d7,d1
	bpl.s	.half_x
	addq.w	#1,d1
.half_x:
	asr.w	#1,d1
	sub.w	d1,d0
	move.w	d0,EF_SX(sp)
	moveq	#0,d0
	move.w	d7,d0
	bsr.w	.random
	add.w	d5,d0
	move.w	d7,d1
	bpl.s	.half_y
	addq.w	#1,d1
.half_y:
	asr.w	#1,d1
	sub.w	d1,d0
	move.w	d0,EF_SY(sp)
	move.w	d0,d1
	cmpi.w	#200,d1
	bcc.w	.next_wheel
	move.w	EF_SX(sp),d0
	cmpi.w	#320,d0
	bcc.w	.next_wheel
	bsr.w	.sample
	moveq	#2,d1
	cmp.l	d1,d0
	beq.s	.dust
	moveq	#15,d1
	cmp.l	d1,d0
	beq.s	.dust
	moveq	#22,d1
	cmp.l	d1,d0
	blt.s	.mark
	moveq	#26,d1
	cmp.l	d1,d0
	ble.s	.dust
.mark:
	move.w	RACE_TRAIL_PARTICLE_COUNT(a4),EF_BEFORE(sp)
	move.w	d4,d7
	swap	d7
	move.w	d5,d7
	move.w	EF_SX(sp),d4
	move.w	EF_SY(sp),d5
	moveq	#0,d1
	move.b	EF_COLOUR(sp),d1
	lsl.w	#8,d1
	moveq	#0,d2
	moveq	#3,d3
	tst.b	EF_LONG(sp)
	beq.s	.mark_life
	moveq	#30,d3
.mark_life:
	bsr.w	.add
	move.w	d7,d5
	swap	d7
	move.w	d7,d4
	tst.b	EF_LONG(sp)
	beq.s	.dust
	move.w	RACE_TRAIL_PARTICLE_COUNT(a4),d0
	cmp.w	EF_BEFORE(sp),d0
	bhi.s	.long_life
	tst.b	slicks_race_disable_particles
	beq.s	.dust
.long_life:
	moveq	#20,d0
	bsr.w	.random
	addi.w	#30,d0
	moveq	#0,d1
	move.w	EF_BEFORE(sp),d1
	cmp.w	RACE_TRAIL_PARTICLE_COUNT(a4),d1
	bcc.s	.dust
	mulu.w	#EP_SIZE,d1
	lea	RACE_TRAIL_PARTICLES+EP_LIFETIME(a4),a0
	move.b	d0,(a0,d1.l)
.dust:
	cmpa.l	#250,a6
	ble.s	.next_wheel
	moveq	#10,d0
	bsr.w	.random
	moveq	#15,d3
	add.b	d0,d3
	moveq	#23,d0
	bsr.w	.random
	subi.w	#11,d0
	move.w	d0,d2			; vy
	moveq	#23,d0
	bsr.w	.random
	subi.w	#11,d0
	swap	d2
	move.w	d0,d2
	swap	d2			; vx:vy
	moveq	#0,d1
	move.b	EF_COLOUR(sp),d1
	lsl.w	#8,d1
	addq.w	#5,d1
	bsr.w	.add
.next_wheel:
	addq.w	#1,EF_WHEEL(sp)
	cmpi.w	#2,EF_WHEEL(sp)
	bcs.w	.wheel
.done:
	lea	EFRAME(sp),sp
	movem.l	(sp)+,d2-d7/a2-a6
	rts
.sample_error:
	move.b	#1,RACE_COLLISION_ERROR(a4)
	bra.s	.next_wheel

; Material sample: d0.w x, d1.w y -> d0.l class or -1. Clobbers d1/d6/a0/a1.
; Visible samples read the maps directly; others use the shared C sampler.
.sample:
	cmpi.w	#320,d0
	bcc.s	.sample_slow
	cmpi.w	#190,d1
	bcc.s	.sample_slow
	mulu.w	#320,d1
	add.w	d0,d1
	movea.l	a4,a0
	tst.b	EF_LAYER+SUB(sp)
	bne.s	.sample_upper
	adda.l	#RACE_MATERIAL_MAP,a0
	moveq	#0,d0
	move.b	(a0,d1.l),d0
	rts
.sample_upper:
	adda.l	#RACE_SURFACE_MAP,a0
	moveq	#31,d0
	and.b	(a0,d1.l),d0
	rts
.sample_slow:
	move.b	EF_LAYER+SUB(sp),d6	; signed char layer as int
	extb.l	d6
	move.l	d6,-(sp)
	ext.l	d1
	move.l	d1,-(sp)
	ext.l	d0
	move.l	d0,-(sp)
	movea.l	a4,a0
	adda.l	#RACE_SURFACE_MAP,a0
	move.l	a0,-(sp)
	movea.l	a4,a0
	adda.l	#RACE_MATERIAL_MAP,a0
	move.l	a0,-(sp)
	jsr	slicks_track_material_sample
	lea	20(sp),sp
	rts

; random_scaled: d0.l limit (u16) -> d0.l. Borland LCG. Clobbers d6.
.random:
	move.l	RACE_RANDOM_STATE(a4),d6
	mulu.l	#$015a4e35,d6
	addq.l	#1,d6
	move.l	d6,RACE_RANDOM_STATE(a4)
	swap	d6
	andi.l	#$7fff,d6
	mulu.w	d6,d0			; limit and draw are both below 65536
	moveq	#15,d6
	lsr.l	d6,d0
	rts

; emit_sound_event(d0.b sample, flags 2, priority 10). Clobbers d1/d6/a0/a1.
.sound:
	moveq	#0,d6
	move.b	d0,d6
	lea	RACE_SOUND_EVENT_TOTALS(a4),a0
	addq.l	#1,(a0,d6.w*4)
	moveq	#0,d6
	move.b	RACE_SOUND_EVENT_COUNT(a4),d6
	lea	RACE_SOUND_EVENTS(a4),a0
	cmpi.b	#8,d6
	bcc.s	.sound_full
	addq.b	#1,RACE_SOUND_EVENT_COUNT(a4)
	mulu.w	#3,d6
	adda.w	d6,a0
.sound_store:
	move.b	d0,(a0)
	move.b	#2,1(a0)
	move.b	#10,2(a0)
	rts
.sound_full:
	movea.l	a0,a1			; first lowest priority wins
	moveq	#6,d6
.sound_scan:
	addq.l	#3,a0
	move.b	2(a0),d1
	cmp.b	2(a1),d1
	bcc.s	.sound_next
	movea.l	a0,a1
.sound_next:
	dbf	d6,.sound_scan
	cmpi.b	#10,2(a1)
	bcc.s	.sound_ret
	movea.l	a1,a0
	bra.s	.sound_store
.sound_ret:
	rts

; add_trail_component: d4.w x, d5.w y, d1.w colour<<8|priority,
; d2.l vx<<16|vy, d3.b lifetime. Clobbers d0/d6/a0/a3; preserves d1..d5.
.add:
	tst.b	slicks_race_disable_particles
	bne.w	.add_ret
	moveq	#0,d6
	move.w	RACE_TRAIL_PARTICLE_COUNT(a4),d6
	cmpi.w	#PARTICLE_MAX,d6
	bcc.w	.add_ret
	lea	RACE_WEAPON_SLOTS(a4),a0
	moveq	#0,d0
	move.w	RACE_EMISSION_SLOT_CURSOR(a4),d0
	beq.w	.add_native
	tst.w	SLOTS_CAPACITY(a0)
	beq.w	.add_ret
.add_scan:				; lowest free slot in cursor..high_water-1
	move.l	d6,-(sp)
	move.w	SLOTS_HIGH_WATER(a0),d6
	sub.w	d0,d6
	bls.s	.add_scan_end		; cursor at or past high_water
	lea	(a0,d0.w),a3
	subq.w	#1,d6
.add_scan_next:
	tst.b	(a3)+
	dbeq	d6,.add_scan_next
	move.l	a3,d0
	sub.l	a0,d0
	subq.w	#1,d0
	move.l	(sp)+,d6
	tst.b	(a0,d0.w)
	beq.s	.add_found
	addq.w	#1,d0			; none free: cursor = high_water
	bra.s	.add_extend
.add_scan_end:
	move.l	(sp)+,d6
.add_extend:
	move.w	d0,RACE_EMISSION_SLOT_CURSOR(a4)
	move.w	SLOTS_HIGH_WATER(a0),d0
	cmp.w	SLOTS_CAPACITY(a0),d0
	bcc.w	.add_ret
	addq.w	#1,SLOTS_HIGH_WATER(a0)
.add_found:
	move.b	#1,(a0,d0.w)
	addq.w	#1,d0
	move.w	d0,RACE_EMISSION_SLOT_CURSOR(a4)
	subq.w	#1,d0
.add_slot:
	lea	RACE_TRAIL_HANDLE(a4),a0
	move.b	d0,(a0,d6.w)
	lea	RACE_TRAIL_INDEX(a4),a0
	move.w	d6,(a0,d0.w*2)
	mulu.w	#ACTOR_SIZE,d0
	lea	RACE_ACTORS(a4),a0
	adda.l	d0,a0
	clr.b	ACTOR_KIND(a0)
	clr.b	ACTOR_SAVED(a0)
	addq.w	#1,RACE_TRAIL_PARTICLE_COUNT(a4)
	mulu.w	#EP_SIZE,d6
	lea	RACE_TRAIL_PARTICLES(a4),a3
	adda.l	d6,a3
	move.w	d4,d0
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	asl.w	#6,d0
	move.w	d0,EP_X(a3)
	else
	ext.l	d0
	asl.l	#6,d0
	move.l	d0,EP_X(a3)
	endif
	move.w	d5,d0
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	asl.w	#6,d0
	move.w	d0,EP_Y(a3)
	else
	ext.l	d0
	asl.l	#6,d0
	move.l	d0,EP_Y(a3)
	endif
	move.l	d2,EP_VELOCITY_X(a3)
	move.b	d3,EP_LIFETIME(a3)
	move.w	d1,EP_COLOUR(a3)
	moveq	#1,d0			; saved 0, transient, occlusion, state 1
	tst.b	d1
	bne.s	.add_state
	cmpi.b	#3,d3
	bne.s	.add_state
	move.l	#$00010005,d0		; permanent road mark, DOS state 5
.add_state:
	or.w	EF_OCC+SUB(sp),d0
	move.l	d0,EP_SAVED_VALID(a3)
	addq.l	#1,RACE_SKIDMARK_COUNT(a4)
.add_ret:
	rts
.add_native:
	move.l	d1,-(sp)		; cursor zero: ordinary lowest-slot allocator
	pea	1.w
	move.l	a0,-(sp)
	jsr	slicks_actor_allocate_native
	addq.l	#8,sp
	move.l	(sp)+,d1
	andi.l	#$ffff,d0
	beq.s	.add_ret
	bra.w	.add_slot
