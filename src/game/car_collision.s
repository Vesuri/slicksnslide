; Isolated native candidate, not yet linked into gameplay.
; C ABI: slicks_resolve_car_pairs(race, current unsigned short).
; One save frame covers all four opponents; no helper calls or stacked
; parameters inside the loop. Positions are never separated.
    section .text,code
    xdef slicks_resolve_car_pairs
    include "race_offsets.i"
slicks_resolve_car_pairs:
    movem.l d2-d7/a2-a6,-(sp)
    subq.l #4,sp
    movea.l 52(sp),a4
    move.w 58(sp),(sp)       ; current index
    clr.w 2(sp)              ; hit, cached probe valid
    tst.b RACE_PAIR_DISABLED(a4)
    bne.w .done
    lea RACE_PARTICIPATION(a4),a2
    moveq #0,d0
    move.w (sp),d0
    tst.b RACE_PARTICIPATION_READY(a4)
    beq.s .current
    tst.b (a2,d0.w)
    beq.w .done
.current:
    mulu.w #CAR_SIZE,d0
    lea RACE_CARS(a4,d0.l),a5
    lea RACE_CARS(a4),a6
    lea RACE_PROPERTIES(a4),a3
    moveq #0,d0
    move.b CAR_VEHICLE(a5),d0
    mulu.w #PROPERTY_SIZE,d0
    lea (a3,d0.l),a0
    moveq #0,d6
    move.b PROPERTY_COLLISION_RADIUS(a0),d6
    mulu.w #50,d6
    moveq #0,d7
    move.b PROPERTY_COLLISION_WEIGHT(a0),d7
    move.l CAR_VELOCITY_X(a5),d5
    bpl.s .abs_x
    neg.l d5
.abs_x:
    move.l CAR_VELOCITY_Y(a5),d0
    bpl.s .abs_y
    neg.l d0
.abs_y:
    add.l d0,d5
    divs.l #2,d5
    addq.l #1,d5             ; fixed original speed denominator
    moveq #0,d4
.opponent:
    cmp.w (sp),d4
    beq.w .next
    tst.b RACE_PARTICIPATION_READY(a4)
    beq.s .layer
    tst.b (a2,d4.w)
    beq.w .next
.layer:
    move.b CAR_ACTOR_LAYER(a5),d0
    cmp.b CAR_ACTOR_LAYER(a6),d0
    bne.w .next
    tst.b 3(sp)
    bne.s .bounds
    move.l CAR_VELOCITY_X(a5),d2
    muls.l #10,d2
    divs.l d5,d2
    add.l CAR_X(a5),d2
    move.l CAR_VELOCITY_Y(a5),d3
    muls.l #10,d3
    divs.l d5,d3
    add.l CAR_Y(a5),d3
    move.b #1,3(sp)
.bounds:
    move.l CAR_X(a6),d0
    sub.l d6,d0
    cmp.l d0,d2
    blt.w .next
    add.l d6,d0
    add.l d6,d0
    cmp.l d0,d2
    bgt.w .next
    move.l CAR_Y(a6),d0
    sub.l d6,d0
    cmp.l d0,d3
    blt.w .next
    add.l d6,d0
    add.l d6,d0
    cmp.l d0,d3
    bgt.w .next
    move.b #1,2(sp)
    tst.b CAR_TOUCHING_CAR(a5)
    bne.w .latch
    move.b #1,CAR_ACTOR_CONTACT(a5)
    move.b #1,CAR_ACTOR_CONTACT(a6)
    move.l CAR_VELOCITY_X(a5),d2
    sub.l CAR_VELOCITY_X(a6),d2
    move.l CAR_VELOCITY_Y(a5),d3
    sub.l CAR_VELOCITY_Y(a6),d3
    move.l d2,d0
    bpl.s .delta_x
    neg.l d0
.delta_x:
    move.l d3,d1
    bpl.s .delta_y
    neg.l d1
.delta_y:
    add.l d1,d0
    movea.l d0,a0            ; magnitude survives both impulses
    moveq #0,d0
    move.b CAR_VEHICLE(a6),d0
    mulu.w #PROPERTY_SIZE,d0
    moveq #0,d1
    move.b PROPERTY_COLLISION_WEIGHT(a3,d0.l),d1
    movea.l d1,a1            ; opponent weight, zero extended
    move.l d1,d0
    muls.l #100,d0
    divs.l d7,d0
    move.l d2,d1
    muls.l d0,d1
    divs.l #100,d1
    sub.l d1,CAR_VELOCITY_X(a5)
    move.l d3,d1
    muls.l d0,d1
    divs.l #100,d1
    sub.l d1,CAR_VELOCITY_Y(a5)
    move.l d7,d0
    muls.l #100,d0
    move.l a1,d1
    divs.l d1,d0
    muls.l d0,d2
    divs.l #100,d2
    add.l d2,CAR_VELOCITY_X(a6)
    muls.l d0,d3
    divs.l #100,d3
    add.l d3,CAR_VELOCITY_Y(a6)
    clr.b 3(sp)              ; current velocity changed: invalidate probe
    move.l a0,d0
    muls.l d1,d0
    divs.l #2,d0
    divs.l d7,d0
    divs.l #5,d0
    move.l d0,CAR_PAIR_IMPACT(a5)
    move.l d0,CAR_PENDING_DAMAGE_IMPACT(a5)
    cmp.l RACE_PAIR_IMPACT(a4),d0
    bls.s .other_impact
    move.l d0,RACE_PAIR_IMPACT(a4)
.other_impact:
    move.l a0,d0
    muls.l d7,d0
    divs.l #2,d0
    divs.l d1,d0
    divs.l #5,d0
    move.l d0,CAR_PAIR_IMPACT(a6)
    move.l d0,CAR_PENDING_DAMAGE_IMPACT(a6)
    cmp.l RACE_PAIR_IMPACT(a4),d0
    bls.s .count
    move.l d0,RACE_PAIR_IMPACT(a4)
.count:
    addq.l #1,RACE_PAIR_COUNT(a4)
.latch:
    move.b #1,CAR_TOUCHING_CAR(a5)
    move.b #1,CAR_TOUCHING_CAR(a6)
    move.b d4,CAR_COLLISION_PARTNER(a5)
.next:
    lea CAR_SIZE(a6),a6
    addq.w #1,d4
    cmpi.w #4,d4
    bcs.w .opponent
    tst.b 2(sp)
    bne.s .done
    clr.b CAR_TOUCHING_CAR(a5)
.done:
    addq.l #4,sp
    movem.l (sp)+,d2-d7/a2-a6
    rts
