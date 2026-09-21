	section	code
	xdef	sgfx_clear_full

; Native replacement for the VGA full-plane clear at runtime offset 2AD92h.
; The original enables all four VGA write planes, waits across a retrace edge,
; and clears the complete 64 KiB aperture.  The four-bank logical store makes
; that side effect a contiguous 256 KiB clear; VGA timing has no native-side
; semantic effect.
;
; In: a0 = base of four consecutive 64 KiB logical VGA planes
; Preserves: d0-d7/a0-a6
sgfx_clear_full:
	movem.l	d0-d7/a0-a6,-(sp)
	movea.l	a0,a1
	moveq	#0,d0
	move.w	#$3fff,d1
.clear:
	move.l	d0,(a1)+
	move.l	d0,(a1)+
	move.l	d0,(a1)+
	move.l	d0,(a1)+
	dbra	d1,.clear
	movem.l	(sp)+,d0-d7/a0-a6
	rts
