; C callers may tail-call this entry: keep it in the compiler .text section.
	section	.text,code
	xdef	slicks_mark_dirty_rect
	include	"race_offsets.i"

; C ABI: void slicks_mark_dirty_rect(race, left, top, right, bottom)
; Native race_runtime.c mark_dirty_rect(): clip, widen to 16-pixel columns,
; then repeatedly fold overlapping half-open rectangles (not edge contact); append,
; or union everything when all SLICKS_DIRTY_ROW_MAX entries are in use.
; Arguments are signed shorts promoted to int. Clobbers d0/d1/a0/a1.
slicks_mark_dirty_rect:
	movem.l	d2-d5/a2,-(sp)
	movea.l	24(sp),a0
	move.w	30(sp),d1		; left
	bpl.s	.left
	moveq	#0,d1
.left:
	move.w	34(sp),d2		; top
	bpl.s	.top
	moveq	#0,d2
.top:
	move.w	38(sp),d3		; right
	cmpi.w	#320,d3
	ble.s	.right
	move.w	#320,d3
.right:
	move.w	42(sp),d4		; bottom
	cmpi.w	#200,d4
	ble.s	.bottom
	move.w	#200,d4
.bottom:
	cmp.w	d3,d1
	bge.w	.done
	cmp.w	d4,d2
	bge.w	.done
	andi.w	#$fff0,d1
	addi.w	#15,d3
	andi.w	#$fff0,d3
	cmpi.w	#320,d3
	ble.s	.clipped
	move.w	#320,d3
.clipped:
	lea	RACE_DIRTY_ROWS(a0),a1
	moveq	#0,d5
	move.b	RACE_DIRTY_ROW_COUNT(a0),d5
.restart:
	movea.l	a1,a2
	move.w	d5,d0
	subq.w	#1,d0
	bmi.s	.append
.scan:
	cmp.w	(a2),d1			; contained: nothing to add
	bcs.s	.outside
	cmp.w	2(a2),d2
	bcs.s	.outside
	cmp.w	4(a2),d3
	bhi.s	.outside
	cmp.w	6(a2),d4
	bls.s	.store_count
.outside:
	cmp.w	(a2),d3			; separated or merely touching
	bls.s	.next
	cmp.w	4(a2),d1
	bcc.s	.next
	cmp.w	2(a2),d4
	bls.s	.next
	cmp.w	6(a2),d2
	bcc.s	.next
	cmp.w	(a2),d1
	bls.s	.union_top
	move.w	(a2),d1
.union_top:
	cmp.w	2(a2),d2
	bls.s	.union_right
	move.w	2(a2),d2
.union_right:
	cmp.w	4(a2),d3
	bcc.s	.union_bottom
	move.w	4(a2),d3
.union_bottom:
	cmp.w	6(a2),d4
	bcc.s	.remove
	move.w	6(a2),d4
.remove:
	subq.w	#1,d5			; existing = rows[--count]
	move.l	(a1,d5.w*8),(a2)
	move.l	4(a1,d5.w*8),4(a2)
	bra.s	.restart
.next:
	addq.l	#8,a2
	dbf	d0,.scan
.append:
	cmpi.w	#SLICKS_DIRTY_ROW_MAX_VALUE,d5
	bcc.s	.fallback
	lea	(a1,d5.w*8),a2
	addq.w	#1,d5
.write:
	move.w	d1,(a2)+
	move.w	d2,(a2)+
	move.w	d3,(a2)+
	move.w	d4,(a2)
.store_count:
	move.b	d5,RACE_DIRTY_ROW_COUNT(a0)
.done:
	movem.l	(sp)+,d2-d5/a2
	rts
.fallback:
	movea.l	a1,a2			; correctness fallback: one union rectangle
	move.w	d5,d0
	subq.w	#1,d0
.all:
	cmp.w	(a2),d1
	bls.s	.all_top
	move.w	(a2),d1
.all_top:
	cmp.w	2(a2),d2
	bls.s	.all_right
	move.w	2(a2),d2
.all_right:
	cmp.w	4(a2),d3
	bcc.s	.all_bottom
	move.w	4(a2),d3
.all_bottom:
	cmp.w	6(a2),d4
	bcc.s	.all_next
	move.w	6(a2),d4
.all_next:
	addq.l	#8,a2
	dbf	d0,.all
	movea.l	a1,a2
	moveq	#1,d5
	bra.s	.write
