	section	data,data
	xdef	slicks_status_negative
	xdef	slicks_status_positive

; Exact call-time planar sprites captured from the BASIC title-screen path.
slicks_status_negative:
	incbin	"obj/status-negative.bin"
slicks_status_positive:
	incbin	"obj/status-positive.bin"
	even
