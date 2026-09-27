	section .text,code
	xdef slicks_restore_hud_cell
; C ABI: destination/background at cell top-left, width (50 or 52).
; Exact original 14 visible rows, 320-byte strides. Four cells only:
; three 52-pixel cells and one clipped 50-pixel cell. No shadow or new
; retention assumptions; the caller keeps the full original dirty bounds.
slicks_restore_hud_cell:
	movea.l 4(sp),a0
	movea.l 8(sp),a1
	moveq #13,d0
	cmpi.l #50,12(sp)
	beq.s .narrow
.wide:
	rept 13
	move.l (a1)+,(a0)+
	endr
	lea 268(a0),a0
	lea 268(a1),a1
	dbf d0,.wide
	rts
.narrow:
	rept 12
	move.l (a1)+,(a0)+
	endr
	move.w (a1)+,(a0)+
	lea 270(a0),a0
	lea 270(a1),a1
	dbf d0,.narrow
	rts
