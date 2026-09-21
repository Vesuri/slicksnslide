	section	data,data
	xdef	slicks_basic_frame
	xdef	slicks_basic_palette

; Generated inputs remain under obj/ and are extracted from ignored reference
; captures by the Makefile. Only their measured hashes are checked in.
slicks_basic_frame:
	incbin	"obj/basic-frame.bin"
slicks_basic_palette:
	incbin	"obj/basic-palette.bin"
