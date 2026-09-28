; Full rectangle/grid conflict walk. C ABI: left, top, right, bottom, priority.
; Keep clipped bounds and the priority threshold resident across cell/entry
; scans. A shared cell checks every candidate against the whole rectangle,
; so no subsequent cell can add a conflict after that scan.
    section .text,code
    include "race_offsets.i"
    xdef slicks_retention_touch
    xref slicks_retention
slicks_retention_touch:
    movem.l d2-d7/a2-a3,-(sp)
    movem.l 36(sp),d0-d4
    tst.l d0
    bpl.s .left
    moveq #0,d0
.left:
    tst.l d1
    bpl.s .top
    moveq #0,d1
.top:
    cmpi.l #320,d2
    ble.s .right
    move.l #320,d2
.right:
    cmpi.l #184,d3
    ble.s .bottom
    move.l #184,d3
.bottom:
    cmp.l d2,d0
    bge.w .done
    cmp.l d3,d1
    bge.w .done
; Convert signed priority to an inclusive unsigned byte threshold. Negative
; priorities touch every eligible entry; >=255 cannot touch any entry.
    tst.l d4
    bmi.s .any_priority
    cmpi.l #255,d4
    bge.w .done
    addq.w #1,d4
    bra.s .grid
.any_priority:
    moveq #0,d4
.grid:
    lea slicks_retention,a0
    move.w d0,d5
    lsr.w #3,d5
    move.w d2,d6
    subq.w #1,d6
    lsr.w #3,d6
    sub.w d5,d6
    addq.w #1,d6                ; number of columns
    move.w d1,d7
    lsr.w #3,d7
    mulu.w #40,d7
    add.w d5,d7
    lea RET_CELLS(a0,d7.w),a1
    lea (a1,d6.w),a2            ; exclusive end of this cell row
    neg.w d6
    addi.w #40,d6               ; gap between the rectangle's cell rows
    moveq #0,d7
    move.w d3,d7
    subq.w #1,d7
    lsr.w #3,d7
    move.w d1,d5
    lsr.w #3,d5
    sub.w d5,d7                 ; row DBF counter; bit 31 means shared scan
.cell:
    moveq #0,d5
    move.b (a1)+,d5
    beq.s .next_cell
    cmpi.b #255,d5
    beq.s .shared
    subq.w #1,d5
    mulu.w #ENTRY_SIZE,d5
    lea RET_ENTRIES(a0,d5.l),a3
    moveq #0,d5
    bra.s .entry
.shared:
    bset #31,d7
    lea RET_ENTRIES(a0),a3
    moveq #0,d5
    move.b RET_COUNT(a0),d5
    subq.w #1,d5
    bmi.s .done
.entry:
    btst #0,ENTRY_FLAGS(a3)
    beq.s .next_entry
    btst #1,ENTRY_FLAGS(a3)
    bne.s .next_entry
    cmp.b ENTRY_PRIORITY(a3),d4
    bhi.s .next_entry
    cmp.w ENTRY_RIGHT(a3),d0
    bge.s .next_entry
    cmp.w ENTRY_LEFT(a3),d2
    ble.s .next_entry
    cmp.w ENTRY_BOTTOM(a3),d1
    bge.s .next_entry
    cmp.w ENTRY_TOP(a3),d3
    ble.s .next_entry
    bset #1,ENTRY_FLAGS(a3)
.next_entry:
    lea ENTRY_SIZE(a3),a3
    dbf d5,.entry
    tst.l d7
    bmi.s .done
.next_cell:
    cmpa.l a2,a1
    bcs.s .cell
    adda.w d6,a1
    lea 40(a2),a2
    dbf d7,.cell
.done:
    movem.l (sp)+,d2-d7/a2-a3
    rts
