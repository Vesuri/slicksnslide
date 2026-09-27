; 16-pixel, eight-plane interleaved C2P.
; Adapted from Mikael Kalms' public-domain c5 butterfly transpose.
; Unlike the 32-pixel/nibble-pass version, retain both nibbles and
; exchange them, yielding two complete plane words in each data register.
; d0.w width, d1.w height, d2.w x, d3.l destination row-byte offset;
; a0 source rectangle origin, a1 bitmap. Preserves d2-d7/a2-a6.
; The caller reuses mult320[y] already fetched for its source address.
; Valid input: width multiple of 16, positive height, x multiple of 16,
; 320-byte source stride, eight planes at +40, row stride 320.
; Only final plane words are stored; never intermediate display data.
    include graphics/gfx.i
    section code,code
    xdef c2p16_interleaved
c2p16_interleaved:
    movem.l d2-d7/a2-a4,-(sp)
    tst.w d0
    beq .done
    tst.w d1
    beq .done
    move.w d1,d7
    subq.w #1,d7
    move.w d0,d6
    lsr.w #4,d6
    subq.w #1,d6
    move.w d6,a4
    move.w #320,d4
    sub.w d0,d4
    move.w d4,a2
    lsr.w #3,d0
    move.w #320,d4
    sub.w d0,d4
    move.w d4,a3
    lsr.w #3,d2
    add.w d2,d3
    movea.l bm_Planes(a1),a1
    adda.l d3,a1
.row:
    move.w a4,d6
.block:
    movem.l (a0)+,d0-d3
    move.l #$0f0f0f0f,d5
    move.l d1,d4
    lsr.l #4,d4
    eor.l d0,d4
    and.l d5,d4
    eor.l d4,d0
    lsl.l #4,d4
    eor.l d4,d1
    move.l d3,d4
    lsr.l #4,d4
    eor.l d2,d4
    and.l d5,d4
    eor.l d4,d2
    lsl.l #4,d4
    eor.l d4,d3
    move.w d0,d4
    swap d1
    move.w d1,d0
    move.w d4,d1
    swap d1
    move.w d2,d4
    swap d3
    move.w d3,d2
    move.w d4,d3
    swap d3
    move.l #$33333333,d5
    move.l d1,d4
    lsr.l #2,d4
    eor.l d0,d4
    and.l d5,d4
    eor.l d4,d0
    lsl.l #2,d4
    eor.l d4,d1
    move.l d3,d4
    lsr.l #2,d4
    eor.l d2,d4
    and.l d5,d4
    eor.l d4,d2
    lsl.l #2,d4
    eor.l d4,d3
    move.l #$00ff00ff,d5
    move.l d2,d4
    lsr.l #8,d4
    eor.l d0,d4
    and.l d5,d4
    eor.l d4,d0
    lsl.l #8,d4
    eor.l d4,d2
    move.l d3,d4
    lsr.l #8,d4
    eor.l d1,d4
    and.l d5,d4
    eor.l d4,d1
    lsl.l #8,d4
    eor.l d4,d3
    move.l #$55555555,d5
    move.l d2,d4
    lsr.l #1,d4
    eor.l d0,d4
    and.l d5,d4
    eor.l d4,d0
    add.l d4,d4
    eor.l d4,d2
    move.l d3,d4
    lsr.l #1,d4
    eor.l d1,d4
    and.l d5,d4
    eor.l d4,d1
    add.l d4,d4
    eor.l d4,d3
    move.w d0,120(a1)
    swap d0
    move.w d0,280(a1)
    move.w d1,40(a1)
    swap d1
    move.w d1,200(a1)
    move.w d2,80(a1)
    swap d2
    move.w d2,240(a1)
    move.w d3,0(a1)
    swap d3
    move.w d3,160(a1)
    addq.l #2,a1
    dbf d6,.block
    adda.w a2,a0
    adda.w a3,a1
    dbf d7,.row
.done:
    movem.l (sp)+,d2-d7/a2-a4
    rts
