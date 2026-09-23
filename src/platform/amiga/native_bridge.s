	section	code
	xdef	slicks_draw_title_pages
	xdef	slicks_draw_title_menu_selection
	xdef	slicks_draw_title_text
	xdef	slicks_dispatch_title_key
	xdef	slicks_setup_basic_mode
	xdef	slicks_prepare_title_frame
	xref	sgfx_title_pages
	xref	sui_title_step
	xref	sui_title_menu
	xref	sui_draw_text
	xref	sui_title_tail
	xref	sui_title_dispatch
	xref	sgfx_mode_setup
	xref	sgfx_chunky_asset_to_planar
	xref	slicks_title_counter
	xref	slicks_title_third_color
	xref	slicks_title_render_state
	xref	slicks_title_fallback_color
	xref	slicks_title_phase
	xref	slicks_title_ordinary_color
	xref	slicks_title_selected_color

; C ABI: slicks_prepare_title_frame(asset, frame)
slicks_prepare_title_frame:
	movea.l	4(sp),a0
	movea.l	8(sp),a1
	jsr	sgfx_chunky_asset_to_planar
	rts

; Temporary C-platform bridge. The translated/native side uses the register
; ABI directly; this wrapper preserves the Amiga GCC callee-saved registers.
; C ABI: slicks_draw_title_pages(planes, frame, palette)
slicks_draw_title_pages:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	moveq	#100,d4
	move.w	#$7fbc,d5
	moveq	#0,d6
	jsr	sgfx_title_pages
	movea.l	52(sp),a1
	movea.l	56(sp),a2
	lea	slicks_title_counter,a3
	lea	slicks_title_third_color,a4
	moveq	#0,d7
	jsr	sui_title_step
	move.w	d0,slicks_title_ordinary_color
	move.w	d1,slicks_title_selected_color
	movea.l	56(sp),a1
	moveq	#0,d2
	moveq	#0,d7
	jsr	sui_title_menu
	movea.l	56(sp),a0
	lea	slicks_title_render_state,a1
	lea	slicks_title_fallback_color,a2
	lea	slicks_title_phase,a3
	jsr	sui_title_tail
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C ABI: slicks_draw_title_menu_selection(planes, palette, selection)
slicks_draw_title_menu_selection:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	move.w	56(sp),d2
	move.w	slicks_title_ordinary_color,d0
	move.w	slicks_title_selected_color,d1
	moveq	#0,d7
	jsr	sui_title_menu
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C ABI: slicks_draw_title_text(planes, text, x, y, colour)
slicks_draw_title_text:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	move.w	56(sp),d0
	move.w	60(sp),d1
	moveq	#0,d2
	move.b	67(sp),d2
	moveq	#0,d3
	jsr	sui_draw_text
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
