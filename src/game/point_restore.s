    include "race_offsets.i"
    section code,code
    xdef slicks_restore_point_chain
    xref mult320
; C ABI: race, first handle. Restore consecutive points, returning the first sprite
; handle unchanged (or zero). Saved points have valid old screen positions.
; Stride/field offsets are checked by the C caller and the native oracle.
slicks_restore_point_chain:
    movem.l d2-d3/a2-a5,-(sp)
    movea.l 28(sp),a5
    lea RACE_TRAIL_PARTICLES(a5),a0
    movea.l RACE_CHUNKY(a5),a1
    lea RACE_ACTOR_ORDER_NEXT(a5),a2
    lea RACE_TRAIL_INDEX(a5),a3
    move.l 32(sp),d3
    lea mult320,a4
    tst.l d3
    beq.s .done
.point:
    move.w (a3,d3.w*2),d0
    bmi.s .done
    move.w d0,d1
    add.w d1,d1
    add.w d1,d0
    lea (a0,d0.w*8),a5
    btst #0,20(a5)
    beq.s .next
    move.l 12(a5),d1
    move.w d1,d2
    move.l (a4,d2.w*4),d2
    swap d1
    add.w d1,d2
    move.b 16(a5),(a1,d2.l)
    move.b #2,20(a5)
.next:
    move.b (a2,d3.w),d3
    bne.s .point
.done:
    move.l d3,d0
    movem.l (sp)+,d2-d3/a2-a5
    rts
