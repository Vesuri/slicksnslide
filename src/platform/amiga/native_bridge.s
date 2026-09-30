	section	code
	xdef	slicks_draw_title_pages
	xdef	slicks_draw_title_menu_selection
	xdef	slicks_draw_title_text
	xdef slicks_draw_title_registration
	xdef slicks_tick_title_registration
	xdef slicks_advance_title_registration
	xdef slicks_draw_title_status_text
	xdef slicks_tick_title_colours
	xdef slicks_title_font_text
	xdef slicks_draw_title_background
	xdef	slicks_draw_original_text
	xdef	slicks_dispatch_title_key
	xdef	slicks_setup_basic_mode
	xdef	slicks_prepare_title_frame
	xref	sgfx_title_pages
	xref	sui_title_step
	xref	sui_title_colours
	xref	sui_title_menu
	xref	sui_draw_text
	xref sui_draw_small_text
	xref	sui_font_string
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
	xref slicks_title_background
	xref sgfx_title_crop

; C ABI: slicks_prepare_title_frame(asset, frame)
; C ABI: slicks_draw_original_text(chunky, font, text, x, y)
; HUD uses ordinary rendering, DS:1604=1, tab=10, shadow offset=(1,0).
slicks_draw_original_text:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
	movea.l	56(sp),a2
	move.w	62(sp),d0
	move.w	66(sp),d1
	moveq	#0,d2
	moveq	#1,d3
	moveq	#10,d4
	moveq	#0,d5
	move.w	#$0100,d6
	jsr	sui_font_string
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C ABI: planes,text,x,y,flags. Status counters use the small font with
; original right/left alignment (6/4), not the centred title-label wrapper.
slicks_draw_title_status_text:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	movea.l 52(sp),a2
	movea.l slicks_title_small_font,a1
	clr.w slicks_title_text_page
	move.w 58(sp),d0
	move.w 62(sp),d1
	move.w 66(sp),d2
	moveq #1,d3
	moveq #10,d4
	moveq #0,d5
	move.w slicks_title_third_color,d5
	move.w #$0100,d6
	jsr sui_font_string_planar
	movem.l (sp)+,d2-d7/a2-a6
	rts

; Generic original title font/flags bridge for the two-row Arcade renderer.
; C ABI: planes, font, text, x, y, flags, shadow colour.
slicks_title_font_text:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	movea.l 52(sp),a1
	movea.l 56(sp),a2
	move.w 62(sp),d0
	move.w 66(sp),d1
	move.w 70(sp),d2
	move.w 74(sp),d5
	moveq #1,d3
	moveq #10,d4
	move.w #$0100,d6
	clr.w slicks_title_text_page
	jsr sui_font_string_planar
	movem.l (sp)+,d2-d7/a2-a6
	rts

slicks_prepare_title_frame:
	movea.l	4(sp),a0
	movea.l	8(sp),a1
	jsr	sgfx_chunky_asset_to_planar
	rts

; Temporary C-platform bridge. The translated/native side uses the register
; ABI directly; this wrapper preserves the Amiga GCC callee-saved registers.
; C ABI: slicks_draw_title_pages(planes, frame, palette)
; Arcade initialization must not execute the ordinary renderer and advance
; the shared colour counter an extra time before drawing its own first frame.
slicks_draw_title_background:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	movea.l 52(sp),a1
	moveq #100,d4
	move.w #$7fbc,d5
	moveq #0,d6
	jsr sgfx_title_pages
	movea.l 56(sp),a0
	lea slicks_title_render_state,a1
	lea slicks_title_fallback_color,a2
	lea slicks_title_phase,a3
	jsr sui_title_tail
	movem.l (sp)+,d2-d7/a2-a6
	rts

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

; Original 29779..297e8, once per visible normal-title update. The native
; colour-only entry leaves static title pixels intact for glyph-only updates.
; C ABI: slicks_tick_title_colours(planes, palette)
slicks_tick_title_colours:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	movea.l slicks_title_background,a1
	movea.l 52(sp),a2
	lea slicks_title_counter,a3
	lea slicks_title_third_color,a4
	moveq #100,d4
	moveq #0,d7
	jsr sui_title_colours
	move.w d0,slicks_title_ordinary_color
	move.w d1,slicks_title_selected_color
	movem.l (sp)+,d2-d7/a2-a6
	rts

; C ABI: slicks_draw_title_menu_selection(planes, palette, selection)
slicks_draw_title_menu_selection:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	; Erase the old selection from the original artwork, not a flat fill.
	movea.l slicks_title_background,a1
	move.l a1,d0
	beq.s .selection_background_ready
	moveq #100,d4
	moveq #0,d7
	jsr sgfx_title_crop
.selection_background_ready:
	movea.l	52(sp),a1
	move.w	58(sp),d2
	move.w	slicks_title_ordinary_color,d0
	move.w	slicks_title_selected_color,d1
	moveq	#0,d7
	jsr	sui_title_menu
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C ABI: slicks_draw_title_text(planes, text, x, y, colour)
; C ABI: slicks_draw_title_registration(planes, text). Original right-aligned
; registered-name anchor (310,190), small font, flags=2, no forced shadow.
; C ABI: slicks_tick_title_registration(planes, text, palette).
; Advance the original colour pulse only while the title is visible.
; C ABI: slicks_advance_title_registration(palette), state-only for a retained
; title. Full redraw callers keep their unconditional painter below.
slicks_advance_title_registration:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 48(sp),a0
	lea slicks_title_render_state,a1
	lea slicks_title_fallback_color,a2
	lea slicks_title_phase,a3
	jsr sui_title_tail
	movem.l (sp)+,d2-d7/a2-a6
	rts

slicks_tick_title_registration:
	movem.l d2-d7/a2-a6,-(sp)
	movea.l 56(sp),a0
	lea slicks_title_render_state,a1
	lea slicks_title_fallback_color,a2
	lea slicks_title_phase,a3
	jsr sui_title_tail
	bra.s slicks_registration_draw
slicks_draw_title_registration:
	movem.l d2-d7/a2-a6,-(sp)
slicks_registration_draw:
	movea.l 48(sp),a0
	movea.l 52(sp),a2
	movea.l slicks_title_small_font,a1
	move.b 6(a1),-(sp)
	move.b slicks_title_render_state+6,6(a1)
	clr.w slicks_title_text_page
	move.w #310,d0
	move.w #190,d1
	moveq #2,d2
	moveq #1,d3
	moveq #10,d4
	moveq #0,d5
	move.w #$0100,d6
	jsr sui_font_string_planar
	move.b (sp)+,6(a1)
	movem.l (sp)+,d2-d7/a2-a6
	rts
	xref slicks_title_small_font
	xref slicks_title_text_page
	xref sui_font_string_planar

slicks_draw_title_text:
	movem.l	d2-d7/a2-a6,-(sp)
	movea.l	48(sp),a0
	movea.l	52(sp),a1
; GCC stacks narrow integer arguments in 32-bit slots, right-aligned.
	move.w	58(sp),d0
	move.w	62(sp),d1
	moveq	#0,d2
	move.b	67(sp),d2
	moveq	#0,d3
	jsr	sui_draw_small_text
	movem.l	(sp)+,d2-d7/a2-a6
	rts

; C-platform bridge for the native title-loop scan-code classifier.
slicks_dispatch_title_key:
	move.w	6(sp),d0
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
