	section	data,data
	xdef	slicks_basic_frame
	xdef	slicks_basic_palette
	xdef	slicks_title_counter
	xdef	slicks_title_third_color

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
