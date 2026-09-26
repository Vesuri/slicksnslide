; Standard runtime references include PC-relative relocations from C .text.
; Keep these entries in that section for elf2hunk's relocation support.
	section .text,code
	xdef memcpy
	xdef memset
; 68020 C ABI. Exact bounds, arbitrary alignment, 32-bit lengths.
; Only caller-saved d0/d1/a0/a1 are clobbered. memcpy requires no overlap.
memcpy:
	movea.l 4(sp),a1
	movea.l 8(sp),a0
	move.l 12(sp),d1
	beq.s .done
	btst #0,7(sp)
	beq.s .blocks
	move.b (a0)+,(a1)+
	subq.l #1,d1
.blocks:
	cmpi.l #16,d1
	bcs.s .tail
.loop:
	move.l (a0)+,(a1)+
	move.l (a0)+,(a1)+
	move.l (a0)+,(a1)+
	move.l (a0)+,(a1)+
	subi.l #16,d1
	cmpi.l #16,d1
	bcc.s .loop
.tail:
	btst #3,d1
	beq.s .four
	move.l (a0)+,(a1)+
	move.l (a0)+,(a1)+
.four:
	btst #2,d1
	beq.s .two
	move.l (a0)+,(a1)+
.two:
	btst #1,d1
	beq.s .one
	move.w (a0)+,(a1)+
.one:
	btst #0,d1
	beq.s .done
	move.b (a0)+,(a1)+
.done:
	move.l 4(sp),d0
	rts

memset:
	movea.l 4(sp),a0
	tst.l 12(sp)
	beq.s .done
	moveq #0,d0
	move.b 11(sp),d0
	beq.s .pattern_ready
	move.w d0,d1
	lsl.w #8,d1
	or.w d1,d0
	move.w d0,d1
	swap d0
	move.w d1,d0
.pattern_ready:
	move.l 12(sp),d1
	btst #0,7(sp)
	beq.s .blocks
	move.b d0,(a0)+
	subq.l #1,d1
.blocks:
	cmpi.l #16,d1
	bcs.s .tail
.loop:
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	move.l d0,(a0)+
	subi.l #16,d1
	cmpi.l #16,d1
	bcc.s .loop
.tail:
	btst #3,d1
	beq.s .four
	move.l d0,(a0)+
	move.l d0,(a0)+
.four:
	btst #2,d1
	beq.s .two
	move.l d0,(a0)+
.two:
	btst #1,d1
	beq.s .one
	move.w d0,(a0)+
.one:
	btst #0,d1
	beq.s .done
	move.b d0,(a0)+
.done:
	move.l 4(sp),d0
	rts
