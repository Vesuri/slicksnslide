; Candidate whole-car preparation. Not linked into gameplay until its
; independent oracle, live shadow and target benchmarks pass.
; AI and weapon callbacks retain their existing C implementations; all
; surrounding fuel/control/steering arithmetic shares one register frame.
	section .text,code
	xdef slicks_prepare_car_motion
	xdef slicks_prepare_all_car_motion
	xref slicks_prepare_ai_controls
	xref slicks_prepare_weapon_controls
	xref slicks_integrate_car_motion_regs
	include "race_offsets.i"
	ifnd SLICKS_PREPARE_INTEGRATOR_BASE
SLICKS_PREPARE_INTEGRATOR_BASE equ 0
	endif

; These tiny blocks execute repeatedly per car. Inline them rather than
; pushing return addresses into Chip RAM just to share a few instructions.
PREPARE_ROLE macro
	moveq #0,d0
	tst.b RACE_PARTICIPATION_READY(a4)
	beq.s .legacy_role\@
	lea RACE_PARTICIPATION(a4),a0
	move.b (a0,d6.w),d0
	bra.s .role_done\@
.legacy_role\@:
	moveq #1,d0
	tst.w d6
	bne.s .role_done\@
	tst.b RACE_HUMAN_CONTROL(a4)
	beq.s .role_done\@
	moveq #-1,d0
.role_done\@:
	endm
PREPARE_NORMALIZE macro
.negative\@:
	tst.w d1
	bpl.s .upper\@
	addi.w #19200,d1
	bra.s .negative\@
.upper\@:
	cmpi.w #19200,d1
	blt.s .normalized\@
	subi.w #19200,d1
	bra.s .upper\@
.normalized\@:
	endm

; C ABI: race, unsigned car index, unsigned tick word. Return controls.
; Across C callbacks: a4 race, a5 car, d6 index, d7 ticks, d5 controls.
; The steering input is determined BEFORE weapon callbacks, as in C.
slicks_prepare_car_motion:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a4
	move.l 52(sp),d6
	move.l 56(sp),d7
	bsr.w slicks_prepare_car_motion_regs
	movem.l (sp)+,d2-d7/a2-a6
	rts

; a4 race, d6 driver index, d7 tick word. Outer caller owns register saves;
; only a4/sp survive. Return the driver's controls in d0.
slicks_prepare_car_motion_regs:
	andi.l #$ffff,d7
	move.l d6,d0
	mulu.w #CAR_SIZE,d0
	lea RACE_CARS(a4),a5
	adda.l d0,a5
	; Idle fuel: subtract sign-extended low word, then signed clamp.
	move.w d7,d0
	add.w d0,d0
	ext.l d0
	sub.l d0,CAR_FUEL(a5)
	bpl.s .idle_done
	clr.l CAR_FUEL(a5)
	tst.w RACE_FUEL_OPTION(a4)
	beq.s .idle_done
	tst.b RACE_DAMAGE_ENABLED(a4)
	beq.s .idle_done
	ori.b #1,CAR_SERVICE_FLAGS(a5)
.idle_done:
	PREPARE_ROLE
	tst.b d0
	bpl.s .ai
	moveq #0,d5
	tst.b RACE_PARTICIPATION_READY(a4)
	beq.s .legacy_input
	lea RACE_DRIVER_CONTROLS(a4),a0
	move.b (a0,d6.w),d5
	bra.s .finish_gate
.legacy_input:
	move.b RACE_CONTROLS(a4),d5
	bra.s .finish_gate
.ai:
	move.l a5,-(sp)
	move.l d7,-(sp)
	move.l d6,-(sp)
	move.l a4,-(sp)
	jsr slicks_prepare_ai_controls
	lea 16(sp),sp
	moveq #0,d5
	move.b d0,d5
.finish_gate:
	moveq #1,d4
	move.l RACE_FINISH_DEADLINE(a4),d1
	beq.s .gate_done
	move.l RACE_GAME_CLOCK(a4),d0
	cmp.l d1,d0
	ble.s .not_expired
	move.b #1,RACE_COMPLETE(a4)
.not_expired:
	tst.b CAR_FINISHED(a5)
	bne.s .suppress
	sub.l d0,d1
	cmpi.l #270,d1
	bge.s .gate_done
.suppress:
	moveq #0,d4
	moveq #0,d5
	lea RACE_DRIVER_CONTROLS(a4),a0
	andi.b #$fc,(a0,d6.w)
	andi.b #$fc,CAR_AI_CONTROL_LATCH(a5)
	tst.w d6
	bne.s .gate_done
	andi.b #$fc,RACE_CONTROLS(a4)
.gate_done:
	PREPARE_ROLE
	moveq #0,d2
	move.b CAR_POSITION_SCALE(a5),d2
	tst.b d0
	ble.s .input_ready
	mulu.w #7,d2
	divu.w #5,d2
.input_ready:
	move.w d2,-(sp)
	tst.w CAR_SPECIAL_DRIVE_STATE(a5)
	beq.s .throttle
	moveq #0,d4
	moveq #0,d5
.throttle:
	btst #0,d5
	beq.s .weapons
	move.w d7,d0
	add.w d0,d0
	ext.l d0
	sub.l d0,CAR_FUEL(a5)
	move.w d7,d0
	mulu.w #160,d0
	ext.l d0
	add.l CAR_SPEED_FIXED(a5),d0
	move.l d0,d1
	divs.l #100,d1
	moveq #0,d2
	move.w CAR_MAXIMUM_SPEED(a5),d2
	cmp.l d2,d1
	ble.s .maximum_ok
	mulu.w #100,d2
	move.w d2,d0
	ext.l d0
.maximum_ok:
	btst #0,CAR_SERVICE_FLAGS(a5)
	beq.s .service_ok
	cmpi.l #3000,d0
	ble.s .service_ok
	move.l #3000,d0
.service_ok:
	tst.b CAR_FINISHED(a5)
	beq.s .finished_ok
	cmpi.l #7000,d0
	ble.s .finished_ok
	move.l #7000,d0
.finished_ok:
	move.l d0,CAR_SPEED_FIXED(a5)
	move.b #1,CAR_FORWARD_DRIVE_LATCH(a5)
.weapons:
	tst.b RACE_WEAPONS_READY(a4)
	beq.s .brake
	; Bridge owns human request, conditional braking and firing, in order.
	move.l d4,-(sp)
	move.l d5,-(sp)
	move.l d7,-(sp)
	move.l d6,-(sp)
	move.l a4,-(sp)
	jsr slicks_prepare_weapon_controls
	lea 20(sp),sp
	moveq #0,d5
	move.b d0,d5
	bra.w .steering
.brake:
	btst #1,d5
	beq.w .steering
	cmpi.b #1,CAR_FORWARD_DRIVE_LATCH(a5)
	bne.s .reverse
	moveq #0,d0
	move.w #$7db5,d0
	sub.w CAR_DRIVE_BIAS(a5),d0
	move.l CAR_VELOCITY_X(a5),d1
	move.l CAR_VELOCITY_Y(a5),d2
	move.w d7,d3
	beq.s .brake_done
	subq.w #1,d3
	moveq #15,d4
.brake_tick:
	muls.l d0,d1
	asr.l d4,d1
	muls.l d0,d2
	asr.l d4,d2
	dbf d3,.brake_tick
.brake_done:
	move.l d1,CAR_VELOCITY_X(a5)
	move.l d2,CAR_VELOCITY_Y(a5)
	clr.l CAR_SPEED_FIXED(a5)
	cmpi.l #100,CAR_MEASURED_SPEED(a5)
	bge.s .steering
	moveq #0,d0
	move.b CAR_VEHICLE(a5),d0
	mulu.w #PROPERTY_SIZE,d0
	lea RACE_PROPERTIES(a4),a0
	tst.b PROPERTY_ENGINE_SOUND(a0,d0.l)
	beq.s .steering
	clr.b CAR_FORWARD_DRIVE_LATCH(a5)
	bra.s .steering
.reverse:
	cmpi.w #999,CAR_DAMAGE(a5)
	bge.s .steering
	moveq #0,d0
	move.w CAR_MAXIMUM_SPEED(a5),d0
	mulu.w #33,d0
	neg.w d0
	ext.l d0
	move.l d0,CAR_SPEED_FIXED(a5)
.steering:
	move.w (sp)+,d0
	move.w d0,CAR_STEERING_AMOUNT(a5)
	move.l d6,d1
	mulu.w #STEERING_CACHE_SIZE,d1
	lea RACE_STEERING_CACHE(a4),a3
	adda.l d1,a3
	move.w CAR_STEERING_SCALE(a5),d1
	move.w CAR_DAMAGE+6(a5),d2
	move.w CAR_STEERING_PROPERTY(a5),d3
	tst.b STEERING_VALID(a3)
	beq.s .cache_miss
	cmp.w STEERING_INPUT(a3),d0
	bne.s .cache_miss
	cmp.w STEERING_SCALE(a3),d1
	bne.s .cache_miss
	cmp.w STEERING_DAMAGE(a3),d2
	bne.s .cache_miss
	cmp.w STEERING_PROPERTY(a3),d3
	beq.s .cache_hit
.cache_miss:
	move.w d0,STEERING_INPUT(a3)
	move.w d1,STEERING_SCALE(a3)
	move.w d2,STEERING_DAMAGE(a3)
	move.w d3,STEERING_PROPERTY(a3)
	ext.l d1
	divs.w #10,d1
	muls.w d1,d0
	ext.l d0
	divs.w #155,d0
	ext.l d2
	divs.w #25,d2
	neg.w d2
	addi.w #80,d2
	muls.w d2,d0
	ext.l d0
	divs.w #100,d0
	muls.w d3,d0
	ext.l d0
	divs.w #50,d0
	move.w d0,STEERING_DELTA(a3)
	move.b #1,STEERING_VALID(a3)
.cache_hit:
	move.w STEERING_DELTA(a3),d0
	muls.w d7,d0
	move.w CAR_HEADING(a5),d1
	btst #2,d5
	beq.s .not_left
	sub.w d0,d1
.not_left:
	btst #3,d5
	beq.s .not_right
	add.w d0,d1
.not_right:
	move.b CAR_DAMAGE_TURN_SIGN(a5),d0
	beq.s .heading
	cmpi.l #30,CAR_MEASURED_SPEED(a5)
	ble.s .heading
	ext.w d0
	muls.w d7,d0
	muls.w CAR_DAMAGE+4(a5),d0
	ext.l d0
	divs.w #70,d0
	add.w d0,d1
.heading:
	PREPARE_NORMALIZE
	move.w d1,CAR_HEADING(a5)
	; The integrator shares the outer register-save frame. Only the returned
	; controls must survive its scratch registers; race/car bases survive.
	move.w d5,-(sp)
	move.l d7,d0
	move.l d5,d1
	andi.l #3,d1
	ifd SLICKS_PREPARE_TEST_INTEGRATOR
	jsr SLICKS_PREPARE_TEST_INTEGRATOR
	else
	jsr slicks_integrate_car_motion_regs+SLICKS_PREPARE_INTEGRATOR_BASE
	endif
	moveq #0,d5
	move.w (sp)+,d5
	move.l CAR_SPEED_FIXED(a5),d0
	divs.l #100,d0
	move.w d0,CAR_SPEED(a5)
	move.w CAR_HEADING(a5),d1
	PREPARE_NORMALIZE
	move.w d1,CAR_HEADING(a5)
	move.l d5,d0
	rts

; C ABI: race, unsigned ticks, unsigned char controls[4]. Preserve inactive
; output bytes and complete every driver's motion before any per-car tail.
; Locals: index, ticks, output pointer. One full register save for four cars.
slicks_prepare_all_car_motion:
	movem.l d2-d7/a2-a6,-(sp)
	lea -12(sp),sp
	movea.l 60(sp),a4
	move.l 64(sp),4(sp)
	move.l 68(sp),8(sp)
	clr.l (sp)
.car_loop:
	move.l (sp),d6
	PREPARE_ROLE
	tst.b d0
	beq.s .next_car
	move.l 4(sp),d7
	bsr.w slicks_prepare_car_motion_regs
	movea.l 8(sp),a0
	move.l (sp),d1
	move.b d0,(a0,d1.w)
.next_car:
	addq.l #1,(sp)
	cmpi.l #4,(sp)
	bne.s .car_loop
	lea 12(sp),sp
	movem.l (sp)+,d2-d7/a2-a6
	rts
