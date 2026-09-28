    include "race_offsets.i"
    section code,code
    xdef slicks_restore_actor_chain
    xref slicks_restore_actor_sprite_regs
    xref mult320
    ifnd SLICKS_SPRITE_TEST_BASE
SLICKS_SPRITE_TEST_BASE equ 0
    endif
    ifne (PARTICLE_SIZE-24)
    fail "Actor restoration requires the production particle layout"
    endif
; C ABI: race, first reverse-order handle. Deferred sprite dirtiness only.
; Traverse points and sprites without returning to C at type boundaries.
; Unsupported sprites return their handle after clearing retain; the caller
; executes the unchanged general restoration and resumes at previous[handle].
; a4 race, a5 reverse links, a6 chunky, d5 handle, d6 point base, d7 index base
; survive the private sprite renderer. No stacked per-sprite arguments.
slicks_restore_actor_chain:
    movem.l d2-d7/a2-a6,-(sp)
    movea.l 48(sp),a4
    move.l 52(sp),d5
    lea RACE_ACTOR_ORDER_PREVIOUS(a4),a5
    movea.l RACE_CHUNKY(a4),a6
    lea RACE_TRAIL_PARTICLES(a4),a0
    move.l a0,d6
    lea RACE_TRAIL_INDEX(a4),a0
    move.l a0,d7
.actor:
    tst.w d5
    beq.w .done
    movea.l d7,a0
    move.w (a0,d5.w*2),d0
    bmi.s .sprite
    move.w d0,d1
    add.w d1,d1
    add.w d1,d0
    movea.l d6,a0
    lea (a0,d0.w*8),a2
    btst #0,20(a2)
    beq.s .next
    move.l 12(a2),d1
    move.w d1,d2
    lea mult320,a0
    move.l (a0,d2.w*4),d2
    swap d1
    add.w d1,d2
    move.b 16(a2),(a6,d2.l)
    move.b #2,20(a2)
.next:
    move.b (a5,d5.w),d5
    bra.s .actor
.sprite:
    move.w .offsets(pc,d5.w*2),d0
    lea RACE_ACTORS(a4),a2
    adda.w d0,a2
    move.w d5,d0
    add.w d0,d0
    add.w d5,d0
    lea RACE_SPRITE_DIRTY_PREVIOUS(a4),a3
    lea (a3,d0.w*4),a3
    btst #0,33(a2)
    beq.s .restore
    tst.b 32(a2)
    beq.s .restore
    move.b 21(a2),6(a3)
    move.b #2,33(a2)
    bra.s .next
.restore:
    clr.b 33(a2)
    movea.l a6,a0
    jsr slicks_restore_actor_sprite_regs+SLICKS_SPRITE_TEST_BASE
    tst.l d0
    beq.s .done
    moveq #0,d0
    move.w RACE_SPRITE_DIRTY_COUNT(a4),d0
    lea RACE_SPRITE_DIRTY_HANDLES(a4),a0
    move.b d5,(a0,d0.w)
    addq.w #1,RACE_SPRITE_DIRTY_COUNT(a4)
    bra.s .next
.done:
    move.l d5,d0
    movem.l (sp)+,d2-d7/a2-a6
    rts
.offsets:
offset set 0
    rept ACTOR_CAPACITY
    dc.w offset
offset set offset+ACTOR_SIZE
    endr
