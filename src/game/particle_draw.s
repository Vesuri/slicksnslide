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

; C ABI: particle, chunky, material, surface, dirty_pixels, dirty_count,
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
	move.b PD_FLAGS(a0),d3
	cmpi.w #319,d0
	bhi.s .hidden
	cmpi.w #183,d1
	bhi.s .hidden
	moveq #0,d2
	move.w d1,d2
	move.l (a6,d2.l*4),d2
	moveq #0,d4
	move.w d0,d4
	add.l d4,d2
	tst.b PD_OCCLUSION(a0)
	beq.s .visible
	moveq #0,d4
	move.b (a2,d2.l),d4
	lsl.w #3,d4
	move.b (a3,d2.l),d3
	andi.w #7,d3
	or.w d3,d4
	moveq #0,d3
	move.b PD_OCCLUSION(a0),d3
	cmp.w d3,d4
	bhi.s .masked
	move.b PD_FLAGS(a0),d3
	bra.s .visible
.masked:
	move.b PD_FLAGS(a0),d3
.hidden:
	btst #1,d3
	beq.s .clear
	bsr.s .queue_old
.clear:
	clr.b PD_FLAGS(a0)
	bra.s .done
.visible:
	btst #1,d3
	beq.s .queue_new
	cmp.w PD_OLD_X(a0),d0
	bne.s .moved
	cmp.w PD_OLD_Y(a0),d1
	beq.s .paint
.moved:
	bsr.s .queue_old
.queue_new:
	move.w d0,(a4)+
	move.b d1,(a4)+
	clr.b (a4)+
	addq.w #1,d5
.paint:
	move.w d0,PD_OLD_X(a0)
	move.w d1,PD_OLD_Y(a0)
	move.b (a1,d2.l),PD_SAVED(a0)
	move.b PD_COLOUR(a0),(a1,d2.l)
	move.b #1,PD_FLAGS(a0)
.done:
	jmp (a5)
.fallback:
	moveq #1,d0
	movem.l (sp)+,d2-d5/a2-a6
	rts
.queue_old:
	cmpi.w #319,PD_OLD_X(a0)
	bhi.s .old_done
	cmpi.w #199,PD_OLD_Y(a0)
	bhi.s .old_done
	move.w PD_OLD_X(a0),(a4)+
	move.b PD_OLD_Y+1(a0),(a4)+
	clr.b (a4)+
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
	movea.l 4(sp),a0
	moveq #0,d0
	move.w (a0,d6.w*2),d0
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
	move.l 84(sp),(sp)
	move.l 56(sp),4(sp)
	movea.l 60(sp),a1
	movea.l 64(sp),a2
	movea.l 68(sp),a3
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
	movea.l (sp),a0
	moveq #0,d0
	move.w (a0)+,d0
	move.l a0,(sp)
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
	move.l 92(sp),4(sp)
	move.l 60(sp),d7		; persistent particle base across the chain
	movea.l 64(sp),a1
	movea.l 68(sp),a2
	movea.l 72(sp),a3
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
