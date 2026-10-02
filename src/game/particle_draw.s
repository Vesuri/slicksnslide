	section code,code
	xdef slicks_draw_particle
	xdef slicks_draw_particle_batch
	xdef slicks_draw_particle_chain
	ifnd SLICKS_PARTICLE_WORD_COORDINATES
SLICKS_PARTICLE_WORD_COORDINATES equ 0
	endif
	ifne SLICKS_PARTICLE_WORD_COORDINATES
PD_SIZE equ 20
PD_Y equ 2
PD_OLD_X equ 8
PD_OLD_Y equ 10
PD_SAVED equ 12
PD_COLOUR equ 14
PD_FLAGS equ 16
PD_OCCLUSION equ 18
	else
PD_SIZE equ 24
PD_Y equ 4
PD_OLD_X equ 12
PD_OLD_Y equ 14
PD_SAVED equ 16
PD_COLOUR equ 18
PD_FLAGS equ 20
PD_OCCLUSION equ 22
	endif

; C ABI: particle, chunky, visibility words, reserved, dirty_pixels, dirty_count,
;        mult320. Returns 1 WITHOUT changes if the dirty list needs the
; existing rectangle-overflow path; otherwise returns 0.
; Default layout is the checked 24-byte particle_runtime.s ABI. The isolated
; compact test opts into 20-byte word coordinates; gameplay is not switched.
slicks_draw_particle:
	movem.l d2-d5/a2-a6,-(sp)
	movea.l 60(sp),a5
	moveq #0,d5
	move.w (a5),d5
	cmpi.w #510,d5
	bhi.w .fallback
	movea.l 40(sp),a0
	movea.l 44(sp),a1
	movea.l 48(sp),a2
	movea.l 52(sp),a3
	movea.l 56(sp),a4
	lea (a4,d5.l*4),a4
	movea.l 64(sp),a6
	lea .single_done(pc),a5
	bra.s .body
.single_done:
	movea.l 60(sp),a5
	move.w d5,(a5)
	moveq #0,d0
	movem.l (sp)+,d2-d5/a2-a6
	rts
; a5 is the caller's continuation, not the dirty-count pointer. Batched
; callers publish the count only at the boundary and avoid one stack
; call/return plus a count store for every point.
.body:
	ifne SLICKS_PARTICLE_WORD_COORDINATES
	move.w (a0),d0
	asr.w #6,d0
	move.w PD_Y(a0),d1
	asr.w #6,d1
	else
	move.l (a0),d0
	asr.l #6,d0
	move.l PD_Y(a0),d1
	asr.l #6,d1
	endif
	move.l PD_FLAGS(a0),d3		; flags.b, -, occlusion.b, state.b
	cmpi.w #319,d0
	bhi.s .hidden
	cmpi.w #183,d1
	bhi.s .hidden
	; Bounds above prove signed-word row/index inputs and a byte offset
	; below 58880. The row-table long has a zero high word; ADD.W cannot
	; carry out of its low word, so no separate zero extensions are needed.
slicks_particle_address_start equ *
	move.l (a6,d1.w*4),d2
	add.w d0,d2
slicks_particle_address_end equ *
	move.w d3,d4
	lsr.w #8,d4			; occlusion limit, zero-extended
	beq.s .visible
	cmp.w (a2,d2.l*2),d4
	bcs.s .hidden
	bra.s .visible
.hidden:
	btst #25,d3			; flags bit 1: old pixel is displayed
	beq.s .clear
	move.l PD_OLD_X(a0),d3
	bsr.s .queue_old
.clear:
	clr.b PD_FLAGS(a0)
	bra.s .done
.visible:
	move.w d0,d4			; old_x:old_y form of the new pixel
	swap d4
	move.w d1,d4
	btst #25,d3
	beq.s .queue_new
	move.l PD_OLD_X(a0),d3
	cmp.l d3,d4
	beq.s .save_under
	bsr.s .queue_old
.queue_new:
	move.l d4,d3			; dirty entry x.w, y.b, 0 (y is below 184)
	lsl.w #8,d3
	move.l d3,(a4)+
	addq.w #1,d5
	move.l d4,PD_OLD_X(a0)
.save_under:
; A restored point already at this pixel retains the same coordinates.
; Still save/repaint the pixel: earlier actors may have changed its underlay.
	move.b (a1,d2.l),PD_SAVED(a0)
	move.b PD_COLOUR(a0),(a1,d2.l)
	move.b #1,PD_FLAGS(a0)
.done:
	jmp (a5)
.fallback:
	moveq #1,d0
	movem.l (sp)+,d2-d5/a2-a6
	rts
.queue_old:			; d3 = old_x:old_y
	cmpi.w #199,d3
	bhi.s .old_done
	cmpi.l #320<<16,d3
	bcc.s .old_done
	lsl.w #8,d3			; old_x.w, old_y.b, 0
	move.l d3,(a4)+
	addq.w #1,d5
.old_done:
	rts

; Keep the hot chain walk beside the body and old-point queue: together
; they fit in the 68020's 256-byte instruction cache. Placing the walker
; after both entry prologues makes it evict the paint code every point.
.chain_loop:
	tst.w d6
	beq.s .chain_done
	cmpi.w #510,d5
	bhi.s .chain_done
	moveq #0,d0
	move.w (a3,d6.w*2),d0
	bmi.s .chain_done
	move.w .particle_offsets(pc,d0.w*2),d0
	movea.l d7,a0
	adda.l d0,a0
	bra.w .body
.chain_continue:
	movea.l (sp),a0
	move.b (a0,d6.w),d6
	bra.s .chain_loop
.chain_done:
	movea.l 80(sp),a5
	move.w d5,(a5)
	move.l d6,d0
	lea 12(sp),sp
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Same first seven arguments, then sorted particle-index words and count.
; Return number processed; caller handles the suffix on dirty-list overflow.
; Sprite actors split batches so saved-under priority order stays exact.
slicks_draw_particle_batch equ .batch_entry
.batch_entry:
	movem.l d2-d7/a2-a6,-(sp)
	subq.l #8,sp
	movea.l 84(sp),a3
	move.l 56(sp),4(sp)
	movea.l 60(sp),a1
	movea.l 64(sp),a2
	movea.l 72(sp),a4
	movea.l 76(sp),a5
	movea.l 80(sp),a6
	moveq #0,d5
	move.w (a5),d5
	lea (a4,d5.l*4),a4
	move.l 88(sp),d6
	moveq #0,d7
	lea .batch_continue(pc),a5
.batch_loop:
	tst.l d6
	beq.s .batch_done
	cmpi.w #510,d5
	bhi.s .batch_done
	moveq #0,d0
	move.w (a3)+,d0
	move.w .particle_offsets(pc,d0.w*2),d0
	movea.l 4(sp),a0
	adda.l d0,a0
	bra.w .body
.batch_continue:
	addq.l #1,d7
	subq.l #1,d6
	bra.s .batch_loop
.batch_done:
	movea.l 76(sp),a5
	move.w d5,(a5)
	move.l d7,d0
	addq.l #8,sp
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Same first seven arguments, then actor next-byte table, signed trail-index
; word table and first handle. Return the first unprocessed handle (zero at
; end, or a sprite/dirty-overflow boundary). No intermediate index array.
slicks_draw_particle_chain equ .chain_entry
.chain_entry:
	movem.l d2-d7/a2-a6,-(sp)
	lea -12(sp),sp
	move.l 88(sp),(sp)
	movea.l 92(sp),a3		; freed terrain register retains the index table
	move.l 60(sp),d7		; persistent particle base across the chain
	movea.l 64(sp),a1
	movea.l 68(sp),a2
	movea.l 76(sp),a4
	movea.l 80(sp),a5
	movea.l 84(sp),a6
	moveq #0,d5
	move.w (a5),d5
	lea (a4,d5.l*4),a4
	move.l 96(sp),d6
	lea .chain_continue(pc),a5
	bra.w .chain_loop

; The fixed 256-slot pool fits in signed word offsets. Keep the multiply
; table outside the hot instruction loop; all entries are below 32768.
.particle_offsets:
particle_offset_value set 0
	rept 256
	dc.w particle_offset_value
particle_offset_value set particle_offset_value+PD_SIZE
	endr
