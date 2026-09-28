; Eight-pixel edges with sixteen-pixel interiors. Same register ABI as
; c2p16_interleaved. No source or plane access outside the input rectangle.
    section code,code
    xdef c2p8_16_interleaved
    xref c2p8_interleaved,c2p16_interleaved
c2p8_16_interleaved:
    tst.w d0
    beq.s .empty
    tst.w d1
    beq.s .empty
    ; Already aligned rectangles retain the original single converter call.
    btst #3,d2
    bne.s .split
    btst #3,d0
    bne.s .split
    jmp c2p16_interleaved
.empty:
    rts
.split:
    movem.l d2-d7/a2-a4,-(sp)
    move.w d0,d4
    move.w d1,d5
    move.w d2,d6
    move.l d3,d7
    movea.l a0,a2
    movea.l a1,a3
    btst #3,d6
    beq.s .middle
    moveq #8,d0
    bsr.s .eight
    addq.l #8,a2
    addq.w #8,d6
    subq.w #8,d4
.middle:
    move.w d4,d0
    andi.w #$fff0,d0
    beq.s .tail
    movea.w d0,a4
    move.w d5,d1
    move.w d6,d2
    move.l d7,d3
    movea.l a2,a0
    movea.l a3,a1
    jsr c2p16_interleaved
    adda.w a4,a2
    add.w a4,d6
    sub.w a4,d4
.tail:
    tst.w d4
    beq.s .done
    moveq #8,d0
    bsr.s .eight
.done:
    movem.l (sp)+,d2-d7/a2-a4
    rts
.eight:
    move.w d5,d1
    move.w d6,d2
    move.l d7,d3
    movea.l a2,a0
    movea.l a3,a1
    jmp c2p8_interleaved
