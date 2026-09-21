	section	code
	xdef	sgfx_mode_setup
	xref	sgfx_clear_full

SMODE_MODE		equ	0
SMODE_PHYSICAL_WIDTH	equ	2
SMODE_HEIGHT		equ	4
SMODE_VIRTUAL_WIDTH	equ	6
SMODE_STRIDE		equ	8
SMODE_PAGE_BYTES	equ	10
SMODE_MAX_START_ROW	equ	12
SMODE_BOTTOM_START_ROW	equ	14
SMODE_ROW_PADDING	equ	16
SMODE_ACTIVE		equ	18
SMODE_LOW_RESOLUTION	equ	20

; Native observed-mode replacement for VGA setup at runtime offset 2ADB7h.
; BIOS, mouse, CRTC, sequencer, and retrace operations are platform concerns;
; this boundary retains the renderer geometry established by DOS mode 0.
;
; In:  a0 = base of four consecutive 64 KiB logical VGA planes
;      a1 = 22-byte native mode-state structure (word fields above)
;      d0.w = mode (the observed native contract supports mode 0)
;      d1.w = requested virtual width in pixels
; Out: d0.w = 0 on success, -1 for an unsupported mode
; Preserves: d1-d7/a0-a6
sgfx_mode_setup:
	movem.l	d1-d7/a0-a6,-(sp)
	tst.w	d0
	bne.s	.unsupported

	clr.w	SMODE_MODE(a1)
	move.w	#320,SMODE_PHYSICAL_WIDTH(a1)
	move.w	#200,SMODE_HEIGHT(a1)

	move.w	d1,d2
	cmpi.w	#320,d2
	bge.s	.width_ready
	move.w	#320,d2
.width_ready:
	lsr.w	#3,d2
	lsl.w	#1,d2
	move.w	d2,SMODE_STRIDE(a1)

	move.w	d2,d3
	lsl.w	#2,d3
	move.w	d3,SMODE_VIRTUAL_WIDTH(a1)

	move.w	d2,d3
	subi.w	#80,d3
	lsl.w	#2,d3
	move.w	d3,SMODE_ROW_PADDING(a1)

	moveq	#0,d3
	move.w	d2,d3
	mulu.w	#200,d3
	move.w	d3,SMODE_PAGE_BYTES(a1)

	move.l	#$0000ffff,d3
	divu.w	d2,d3
	move.w	d3,SMODE_MAX_START_ROW(a1)
	subi.w	#200,d3
	move.w	d3,SMODE_BOTTOM_START_ROW(a1)

	move.w	#1,SMODE_ACTIVE(a1)
	move.w	#1,SMODE_LOW_RESOLUTION(a1)
	jsr	sgfx_clear_full
	moveq	#0,d0
	bra.s	.done

.unsupported:
	clr.w	SMODE_ACTIVE(a1)
	moveq	#-1,d0
.done:
	movem.l	(sp)+,d1-d7/a0-a6
	rts
