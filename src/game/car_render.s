; C callers may tail-call this entry: keep it in the compiler .text section.
	section	.text,code
	xdef	slicks_draw_car_native
	xref	slicks_mark_dirty_rect
	xref	slicks_draw_sprite_opaque
	xref	slicks_draw_car_chunky
	xref	mult320
	include	"race_offsets.i"

; C ABI: int slicks_draw_car_native(race, car_index)
; Prepared-cache form of race_runtime.c draw_car() for the authoritative
; chunky surface. Every eligibility test (bounds, cache identity, frame size,
; tile occlusion maximum, masked-style limit) runs before any side effect:
; return 0 means "untouched, use the C renderer"; 1 means handled, including
; the original silent return for an off-track rectangle.
slicks_draw_car_native:
	movem.l	d2-d7/a2-a5,-(sp)
	movea.l	44(sp),a4		; race
	move.l	48(sp),d7		; car index
	move.w	d7,d0
	mulu.w	#CAR_SIZE,d0
	lea	RACE_CARS(a4),a5
	adda.l	d0,a5
	moveq	#0,d6
	move.w	CAR_HEADING(a5),d6
	divu.w	#1200,d6		; direction (low word)
	moveq	#3,d0
	and.w	d6,d0
	moveq	#0,d1
	move.b	CAR_VEHICLE(a5),d1
	lsl.w	#2,d1
	add.w	d0,d1
	mulu.w	#SPRITE_SIZE,d1
	movea.l	a4,a0
	adda.l	#RACE_SPRITES,a0
	adda.l	d1,a0
	moveq	#0,d4			; width
	moveq	#0,d5			; height
	move.b	SPRITE_WIDTH(a0),d4
	move.b	SPRITE_HEIGHT(a0),d5
	btst	#2,d6			; rotation & 1
	beq.s	.sized
	exg	d4,d5
.sized:
	move.l	CAR_X(a5),d0
	move.l	d0,d2
	muls.l	#$51eb851f,d1:d2
	asr.l	#5,d1
	add.l	d0,d0
	subx.l	d0,d0
	sub.l	d0,d1
	move.w	d4,d0
	lsr.w	#1,d0
	sub.w	d0,d1
	move.w	d1,d2			; origin x
	move.l	CAR_Y(a5),d0
	move.l	d0,d3
	muls.l	#$51eb851f,d1:d3
	asr.l	#5,d1
	add.l	d0,d0
	subx.l	d0,d0
	sub.l	d0,d1
	move.w	d5,d0
	lsr.w	#1,d0
	sub.w	d0,d1
	move.w	d1,d3			; origin y
	bmi.w	.handled
	tst.w	d2
	bmi.w	.handled
	move.w	d2,d0
	add.w	d4,d0
	cmpi.w	#320,d0
	bgt.w	.handled
	move.w	d3,d0
	add.w	d5,d0
	cmpi.w	#190,d0
	bgt.w	.handled
	; No side effects until the prepared frame is proved usable.
	tst.w	d4
	beq.w	.general
	tst.w	d5
	beq.w	.general
	cmpi.w	#16,d6
	bcc.w	.general
	movea.l	a4,a1
	adda.l	#RACE_CACHE_READY,a1
	tst.b	(a1)
	beq.w	.general
	move.w	d7,d0
	mulu.w	#CACHE_CAR_SIZE,d0
	movea.l	a4,a3
	adda.l	#RACE_CACHE_CARS,a3
	adda.l	d0,a3
	tst.b	CACHE_CAR_READY(a3)
	beq.w	.general
	move.b	CACHE_CAR_VEHICLE(a3),d0
	cmp.b	CAR_VEHICLE(a5),d0
	bne.w	.general
	move.b	CACHE_CAR_STYLE(a3),d0
	cmp.b	CAR_STYLE(a5),d0
	bne.w	.general
	move.w	d6,d0
	mulu.w	#CACHE_FRAME_SIZE,d0
	lea	CACHE_CAR_FRAMES(a3,d0.l),a3	; frame
	cmp.b	CACHE_FRAME_WIDTH(a3),d4
	bne.w	.general
	cmp.b	CACHE_FRAME_HEIGHT(a3),d5
	bne.w	.general
	moveq	#0,d6			; occlusion limit
	move.b	CAR_ACTOR_LAYER(a5),d6
	mulu.w	#15,d6
	andi.w	#$ff,d6
	moveq	#1,d7			; visible
	tst.w	d6
	beq.s	.visibility
	move.w	d2,d0			; tile columns ox>>3 .. (ox+w-1)>>3
	lsr.w	#3,d0
	move.w	d2,d1
	add.w	d4,d1
	subq.w	#1,d1
	lsr.w	#3,d1
	sub.w	d0,d1
	movea.w	d1,a0			; columns - 1
	move.w	d3,d1			; tile rows oy>>3 .. (oy+h-1)>>3
	lsr.w	#3,d1
	move.w	d3,d7
	add.w	d5,d7
	subq.w	#1,d7
	lsr.w	#3,d7
	sub.w	d1,d7			; rows - 1
	mulu.w	#80,d1			; 40 words per tile row
	add.w	d0,d0
	add.l	d0,d1
	lea	RACE_CACHE_TILE_MAX-RACE_CACHE_READY(a1),a2
	adda.l	d1,a2
.tile_row:
	movea.l	a2,a1
	move.w	a0,d1
.tile:
	cmp.w	(a1)+,d6		; any tile maximum above the car layer
	bcs.s	.occluded
	dbf	d1,.tile
	lea	80(a2),a2
	dbf	d7,.tile_row
	moveq	#1,d7
	bra.s	.visibility
.occluded:
	moveq	#0,d7
	cmpi.b	#51,CAR_STYLE(a5)	; synthetic ramp wrap stays general
	bcc.w	.general
.visibility:
	move.w	d3,d0			; mark_dirty_rect(ox, oy, ox+w, oy+h)
	add.w	d5,d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	d2,d0
	add.w	d4,d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	d3,d0
	ext.l	d0
	move.l	d0,-(sp)
	move.w	d2,d0
	ext.l	d0
	move.l	d0,-(sp)
	move.l	a4,-(sp)
	jsr	slicks_mark_dirty_rect
	lea	20(sp),sp
	move.w	d2,CAR_OLD_X(a5)
	move.b	d3,CAR_OLD_Y(a5)
	move.b	d4,CAR_OLD_WIDTH(a5)
	move.b	d5,CAR_OLD_HEIGHT(a5)
	lea	mult320,a0
	move.l	(a0,d3.w*4),d0
	moveq	#0,d1
	move.w	d2,d1
	add.l	d1,d0			; row offset
	movea.l	RACE_CHUNKY(a4),a2
	adda.l	d0,a2
	tst.b	d7
	beq.s	.masked
	move.l	d5,-(sp)		; opaque(dest, pixels, saved, opacity, w, h)
	move.l	d4,-(sp)
	pea	CACHE_FRAME_OPACITY(a3)
	pea	CAR_SAVED_UNDER(a5)
	pea	CACHE_FRAME_PIXELS(a3)
	move.l	a2,-(sp)
	jsr	slicks_draw_sprite_opaque
	lea	24(sp),sp
	bra.s	.drawn
.masked:
	move.l	d6,-(sp)		; car_chunky(..., 1, w, 0, occlusion)
	clr.l	-(sp)
	move.l	d4,-(sp)
	pea	1.w
	move.l	d5,-(sp)
	move.l	d4,-(sp)
	movea.l	a4,a0
	adda.l	#RACE_SURFACE_MAP,a0
	pea	(a0,d0.l)
	movea.l	a4,a0
	adda.l	#RACE_MATERIAL_MAP,a0
	pea	(a0,d0.l)
	pea	CAR_SAVED_UNDER(a5)
	pea	CACHE_FRAME_PIXELS(a3)
	move.l	a2,-(sp)
	jsr	slicks_draw_car_chunky
	lea	44(sp),sp
.drawn:
	move.b	#1,CAR_SAVED_VALID(a5)
.handled:
	moveq	#1,d0
	bra.s	.return
.general:
	moveq	#0,d0
.return:
	movem.l	(sp)+,d2-d7/a2-a5
	rts
