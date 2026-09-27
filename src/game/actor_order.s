    include "race_offsets.i"
    section code,code
    xdef slicks_build_draw_order
; C ABI: race. Only the forward draw chains; initial reverse construction
; remains the independent C path. Scan descending handles and prepend so
; equal-priority actors draw in ascending original handle order.
slicks_build_draw_order:
    movem.l d2-d3/a2-a6,-(sp)
    movea.l 32(sp),a0
    lea RACE_ACTOR_ORDER_HEAD(a0),a5
    moveq #0,d0
; Clear the odd-aligned head array with aligned body stores.
; The race itself has long alignment (checked by the C caller).
    ifne ((RACE_ACTOR_ORDER_HEAD&3)-1)
    fail "Update actor-order clear prefix for changed head alignment"
    endif
    ifne (PARTICLE_SIZE-24)
    fail "Update particle index scaling for changed record size"
    endif
    move.b d0,(a5)+
    move.w d0,(a5)+
    rept 31
    move.l d0,(a5)+
    endr
; Current layout is 1 mod 4: one byte + one word + 31 longs + one byte.
    move.b d0,(a5)
    lea RACE_ACTOR_ORDER_HEAD(a0),a5
    clr.b RACE_ACTOR_ORDER_DRAWN(a0)
    lea RACE_WEAPON_SLOTS(a0),a1
    lea RACE_TRAIL_INDEX(a0),a2
    lea RACE_TRAIL_PARTICLES+PARTICLE_PRIORITY(a0),a3
    lea RACE_ACTORS(a0),a4
    lea RACE_ACTOR_ORDER_NEXT(a0),a6
    moveq #0,d3
    moveq #0,d2
    move.w SLOTS_HIGH_WATER(a1),d2
    subq.w #1,d2
    ble.s .done
.handle:
    tst.b (a1,d2.w)
    ble.s .next
    move.w (a2,d2.w*2),d0
    bmi.s .sprite
    move.w d0,d1
    add.w d1,d1
    add.w d1,d0
    moveq #0,d1
    move.b (a3,d0.w*8),d1
.priority:
    bmi.s .next
    cmp.b d3,d1
    bls.s .maximum
    move.b d1,d3
.maximum:
    move.b (a5,d1.w),(a6,d2.w)
    move.b d2,(a5,d1.w)
.next:
    subq.w #1,d2
    bne.s .handle
.done:
    move.b d3,RACE_ACTOR_ORDER_MAX(a0)
    move.b #1,RACE_ACTOR_ORDER_READY(a0)
    movem.l (sp)+,d2-d3/a2-a6
    rts
.sprite:
    move.w d2,d0
    mulu.w #ACTOR_SIZE,d0
    tst.b ACTOR_KIND(a4,d0.l)
    beq.s .next
    moveq #0,d1
    move.b ACTOR_PRIORITY(a4,d0.l),d1
    bra.s .priority
