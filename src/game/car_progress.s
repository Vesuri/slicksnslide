; Isolated candidate, not linked into gameplay yet. Actual clock,
; checkpoint, layer-selection and lap-clock arithmetic shares one frame.
; Cold checkpoint/finish side effects use C bridges; coordinates are reloaded
; after checkpoint callbacks rather than assuming that callbacks are pure.
	section .text,code
	xdef slicks_advance_car_progress
	xdef slicks_advance_car_progress_regs
	xref slicks_progress_checkpoint_crossed
	xref slicks_progress_lap_crossed
	xref mult320
	include "race_offsets.i"

; d0 raw clock -> d0.w displayed centiseconds. Clamp the LOW WORD before
; scaling, exactly like the DOS formatter; the full counter stays untouched.
PROGRESS_FORMAT_CLOCK macro
	andi.l #$ffff,d0
	cmpi.w #17999,d0
	bls.s .clamped\@
	move.w #17999,d0
.clamped\@:
	mulu.w #5,d0
	divu.w #9,d0
	endm

; C ABI: race, car pointer, unsigned tick word. The caller invokes this
; after wheel emission and before pair collisions, preserving phase order.
; a2 race, a3 car, d2/d3 full signed pixel coordinates.
slicks_advance_car_progress:
	movem.l d2-d3/a2-a3,-(sp)
	movea.l 20(sp),a2
	movea.l 24(sp),a3
	moveq #0,d1
	move.w 30(sp),d1
	bsr.w slicks_advance_car_progress_regs
	movem.l (sp)+,d2-d3/a2-a3
	rts

; For a larger native finishing pass: a2 race, a3 car, d1 zero-extended
; tick word. a2/a3 and d4-d7/a4-a6 survive. d0-d3/a0-a1 are scratch.
; The outer pass owns the save frame, not each arithmetic section.
slicks_advance_car_progress_regs:
	tst.b CAR_FINISHED(a3)
	bne.s .coordinates
	add.l d1,d1
	move.l CAR_ELAPSED_UNITS(a3),d0
	add.l d1,d0
	move.l d0,CAR_ELAPSED_UNITS(a3)
	PROGRESS_FORMAT_CLOCK
	move.w d0,CAR_ELAPSED_CENTISECONDS(a3)
	move.l CAR_CURRENT_LAP_UNITS(a3),d0
	add.l d1,d0
	move.l d0,CAR_CURRENT_LAP_UNITS(a3)
	PROGRESS_FORMAT_CLOCK
	move.w d0,CAR_CURRENT_LAP_CENTISECONDS(a3)
.coordinates:
	move.l CAR_X(a3),d2
	move.l CAR_Y(a3),d3
	divs.l #100,d2
	divs.l #100,d3
	moveq #0,d0
	move.w CAR_CHECKPOINT(a3),d0
	cmp.w RACE_CHECKPOINT_COUNT(a2),d0
	bcc.s .layer
	lea RACE_CHECKPOINTS(a2),a0
	lea (a0,d0.w*2),a0
	lea (a0,d0.w*4),a0
	move.w d2,d0
	addq.w #1,d0
	cmp.w (a0),d0
	bcs.s .layer
	cmp.w 2(a0),d0
	bhi.s .layer
	move.w d3,d0
	addq.w #1,d0
	moveq #0,d1
	move.b 4(a0),d1
	cmp.w d1,d0
	bcs.s .layer
	move.b 5(a0),d1
	cmp.w d1,d0
	bhi.s .layer
	addq.w #1,CAR_CHECKPOINT(a3)
	; Stateful lap-limit query and flag activation happen here, even for
	; finished cars. The bridge does not increment the checkpoint again.
	move.l a3,-(sp)
	move.l a2,-(sp)
	jsr slicks_progress_checkpoint_crossed
	addq.l #8,sp
	move.l CAR_X(a3),d2
	move.l CAR_Y(a3),d3
	divs.l #100,d2
	divs.l #100,d3
.layer:
	; Layer sampling narrows to signed words; lap completion below does NOT.
	cmpi.w #320,d2
	bcc.s .lap_test
	cmpi.w #190,d3
	bcc.s .lap_test
	lea mult320,a0
	move.l (a0,d3.w*4),d0
	add.w d2,d0
	lea RACE_MATERIAL_MAP(a2),a0
	move.b (a0,d0.l),d1
	tst.b CAR_ACTOR_LAYER(a3)
	bne.s .upper
	tst.b d1
	bne.s .selected
	tst.w CAR_SPECIAL_DRIVE_STATE(a3)
	bne.s .selected
	tst.b CAR_ACTOR_CONTACT(a3)
	bne.s .selected
	move.b #1,CAR_ACTOR_LAYER(a3)
.upper:
	lea RACE_SURFACE_MAP(a2),a0
	move.b (a0,d0.l),d1
.selected:
	move.b d1,CAR_SELECTED_SURFACE(a3)
	cmpi.b #18,d1
	beq.s .oil_done
	clr.b CAR_OIL_ACTIVE(a3)
.oil_done:
	tst.w CAR_SPECIAL_DRIVE_STATE(a3)
	bne.s .special
	cmpi.b #19,d1
	bne.s .effective
	clr.b CAR_ACTOR_LAYER(a3)
.effective:
	move.b d1,CAR_EFFECTIVE_SURFACE(a3)
	bra.s .lap_test
.special:
	clr.b CAR_ACTOR_LAYER(a3)
	clr.b CAR_EFFECTIVE_SURFACE(a3)
	clr.b CAR_COLLISION_SAMPLING(a3)
.lap_test:
	move.w CAR_CHECKPOINT(a3),d0
	cmp.w RACE_CHECKPOINT_COUNT(a2),d0
	bcs.w .done
	cmpi.b #17,CAR_SELECTED_SURFACE(a3)
	beq.s .lap
	cmpi.l #320,d2
	bcc.w .done
	cmpi.l #190,d3
	bcc.w .done
	lea mult320,a0
	move.l (a0,d3.w*4),d0
	add.w d2,d0
	lea RACE_MATERIAL_MAP(a2),a0
	cmpi.b #17,(a0,d0.l)
	bne.w .done
.lap:
	clr.w CAR_CHECKPOINT(a3)
	move.l CAR_CURRENT_LAP_UNITS(a3),d0
	move.l d0,CAR_LAST_LAP_UNITS(a3)
	cmp.l CAR_BEST_LAP_UNITS(a3),d0
	bge.s .best_done
	move.l d0,CAR_BEST_LAP_UNITS(a3)
.best_done:
	PROGRESS_FORMAT_CLOCK
	move.w d0,CAR_LAST_LAP_CENTISECONDS(a3)
	move.l CAR_BEST_LAP_UNITS(a3),d0
	PROGRESS_FORMAT_CLOCK
	move.w d0,CAR_BEST_LAP_CENTISECONDS(a3)
	clr.l CAR_CURRENT_LAP_UNITS(a3)
	clr.w CAR_CURRENT_LAP_CENTISECONDS(a3)
	addq.w #1,CAR_LAP(a3)
	; Cold awards/sounds/ranking query the limit AFTER the lap increment.
	move.l a3,-(sp)
	move.l a2,-(sp)
	jsr slicks_progress_lap_crossed
	addq.l #8,sp
.done:
	rts
