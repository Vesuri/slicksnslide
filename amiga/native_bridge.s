	section	code
	xdef	slicks_draw_title_pages
	xdef	slicks_dispatch_title_key
	xdef	slicks_setup_basic_mode
	xdef	slicks_enter_basic_race
	xref	sgfx_title_pages
	xref	sui_title_step
	xref	sui_title_menu
	xref	sui_title_status
	xref	sui_title_tail
	xref	sui_title_dispatch
	xref	sgfx_mode_setup
	xref	sgfx_planar_blit
	xref	sgame_post_title_init
	xref	slicks_basic_frame
	xref	slicks_basic_palette
	xref	slicks_title_counter
	xref	slicks_title_third_color
	xref	slicks_title_render_state
	xref	slicks_title_fallback_color
	xref	slicks_title_phase
	xref	slicks_basic_race_frame

; Temporary C-platform bridge. The translated/native side uses the register
; ABI directly; this wrapper preserves the Amiga GCC callee-saved registers.
slicks_draw_title_pages:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	lea	slicks_basic_frame,a1
	moveq	#100,d4
	move.w	#$7fbc,d5
	moveq	#0,d6
	jsr	sgfx_title_pages
	lea	slicks_basic_frame,a1
	lea	slicks_basic_palette,a2
	lea	slicks_title_counter,a3
	lea	slicks_title_third_color,a4
	moveq	#0,d7
	jsr	sui_title_step
	lea	slicks_basic_palette,a1
	moveq	#0,d7
	jsr	sui_title_menu
	moveq	#0,d7
	jsr	sui_title_status
	lea	slicks_basic_palette,a0
	lea	slicks_title_render_state,a1
	lea	slicks_title_fallback_color,a2
	lea	slicks_title_phase,a3
	jsr	sui_title_tail
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C-platform bridge for the native title-loop scan-code classifier.
slicks_dispatch_title_key:
	move.w	4(sp),d0
	jsr	sui_title_dispatch
	rts

; C-platform bridge for the one observed video setup: mode 0 with a
; 400-pixel virtual width.  The native routine records its renderer geometry
; and clears the complete four-bank logical VGA store.
slicks_setup_basic_mode:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	moveq	#0,d0
	move.w	#400,d1
	jsr	sgfx_mode_setup
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; Cross the observed title activation boundary: run the translated 4x13
; player-state initialization, then install the exact sustained BASIC-race
; checkpoint through the already-proved native planar blitter.
; C ABI: slicks_enter_basic_race(logical, grid, flags, seeds)
slicks_enter_basic_race:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	52(sp),a0
	movea.l	56(sp),a1
	movea.l	60(sp),a2
	moveq	#0,d0
	moveq	#0,d1
	jsr	sgame_post_title_init

	movea.l	48(sp),a0
	lea	slicks_basic_race_frame,a1
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d3
	moveq	#100,d4
	jsr	sgfx_planar_blit
	movem.l	(sp)+,d2-d7/a2-a6
	rts
