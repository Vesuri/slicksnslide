	section	data,data
	xdef	slicks_basic_race_frame
	xdef	slicks_basic_race_palette

; Generated from the corrected sustained BASIC.SS DOS reference capture.
; The source-derived binaries remain ignored under obj/.
slicks_basic_race_frame:
	incbin	"obj/basic-race-frame.bin"
slicks_basic_race_palette:
	incbin	"obj/basic-race-palette.bin"
	even
