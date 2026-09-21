	section	data,data
	xdef	slicks_basic_frame
	xdef	slicks_basic_palette
	xdef	slicks_title_counter
	xdef	slicks_title_third_color
	xdef	slicks_title_render_state
	xdef	slicks_title_fallback_color
	xdef	slicks_title_phase

; Generated inputs remain under obj/ and are extracted from ignored reference
; captures by the Makefile. Only their measured hashes are checked in.
slicks_basic_frame:
	incbin	"obj/basic-frame.bin"
slicks_basic_palette:
	incbin	"obj/basic-palette.bin"
	even
slicks_title_counter:
	dc.b	0
	even
slicks_title_third_color:
	dc.w	0
slicks_title_render_state:
	dc.b	0,0,0,0,0,8
	dc.b	0,0,0,0,0,0,0,0
	even
slicks_title_fallback_color:
	dc.w	0
slicks_title_phase:
	dc.w	0
