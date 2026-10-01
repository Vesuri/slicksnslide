; WHDLoad/Kickstart 3.1 launcher for the Amiga port. Cross-assembled with vasm.
; The executable uses Exec/graphics/DOS; kickfs supplies ordinary file access.

        INCLUDE whdload.i
        INCLUDE whdmacros.i

CHIPMEMSIZE = $200000
        IFND FASTMEMSIZE
FASTMEMSIZE = 0                 ; game targets 2 MiB Chip only, as standalone
        ENDC
NUMDRIVES = 0
WPDRIVES = 0
BLACKSCREEN
INITAGA
BOOTDOS
CACHECHIP
HDINIT
SEGTRACKER
NO68020                         ; portable kickemu patches; game still needs 020
; Do not use kick31.s STACKSIZE: its A600 patch at $2305c overwrites
; the MOVE.L opcode (the immediate starts at $2305e).
        IFD TEST
BOOTEARLY
DEBUG
        ENDC

        IFD SNOOPFS
slv_Version = 18
        ELSE
slv_Version = 17
        ENDC
slv_Flags = WHDLF_NoError|WHDLF_ReqAGA|WHDLF_Req68020
slv_keyexit = $59                 ; F10
        INCLUDE whdload/kick31.s

slv_CurrentDir dc.b "data",0
slv_name dc.b "Slicks",0
slv_copy dc.b "1993-1997 Timo Kauppinen",0
slv_info dc.b "Amiga port by Vesuri",10
        dc.b "Version 0.1 (28.09.2026)",10
        dc.b "F10 quits",0
slv_config dc.b 0
        dc.b "$VER: Slicks.slave 0.1 (30.09.2026)",0
_program dc.b "Slicks",0
_args
        dc.b "WHDLOAD "
        IFD RECORDTEST
        dc.b "NATURALF",10,0
        ELSE
        IFD CHAMPTEST
        dc.b "CHAMPSAVE",10,0
        ELSE
        IFD EDITTEST
        dc.b "CHAMPEDIT",10,0
        ELSE
        IFD RACETEST
        dc.b "NATURALOBQ",10,0
        ELSE
        IFD EXITTEST
        dc.b "REGCHECK",10,0
        ELSE
        dc.b 10,0
        ENDC
        ENDC
        ENDC
        ENDC
        ENDC
_argsend
        EVEN

_bootdos
        move.l (_resload,pc),a2
        IFD BOOTONLY
        pea TDREASON_OK
        jmp (resload_Abort,a2)
        ENDC
        IFD TEST
        lea (_bootmark,pc),a0
        bsr _mark
        ENDC
        lea (_dosname,pc),a1
        move.l 4.w,a6
        jsr (_LVOOldOpenLibrary,a6)
        move.l d0,a6
        tst.l d0
        beq .oserror
        ; Slicks reads DOS GetArgStr, as a normal Shell-launched program does.
        lea (_args,pc),a0
        move.l a0,d1
        jsr (_LVOSetArgStr,a6)
        lea (_oldargs,pc),a0
        move.l d0,(a0)
        lea (_program,pc),a0
        move.l a0,d1
        jsr (_LVOLoadSeg,a6)
        move.l d0,d7
        beq .readerror
        bsr _patch_storage
        IFD TEST
        lea (_loadmark,pc),a0
        bsr _mark
        ENDC
        jsr (resload_FlushCache,a2)
        ; Establish PROGDIR as a Shell would. Calling a LoadSeg entry alone
        ; does not set pr_HomeDir, which the game's resource loader uses.
        lea (_current,pc),a0
        move.l a0,d1
        moveq #-2,d2              ; ACCESS_READ, lock the actual current drawer
        jsr (_LVOLock,a6)
        tst.l d0
        beq .readerror
        move.l d0,d1
        jsr (_LVOSetProgramDir,a6)
        lea (_oldhome,pc),a0
        move.l d0,(a0)
        IFD LOADONLY
        pea TDREASON_OK
        jmp (resload_Abort,a2)
        ENDC
        ; Use Exec's public API instead of modifying Kickstart's CLI code.
        move.l a6,-(sp)
        move.l 4.w,a6
        move.l #16384,d0
        moveq #0,d1
        jsr (_LVOAllocMem,a6)
        tst.l d0
        beq .oserror
        lea (_stackmem,pc),a0
        move.l d0,(a0)
        lea (_stack,pc),a0
        move.l d0,(a0)
        add.l #16384,d0
        move.l d0,(4,a0)
        move.l d0,(8,a0)
        jsr (_LVOStackSwap,a6)
        move.l d7,a1
        add.l a1,a1
        add.l a1,a1
        move.l #_argsend-_args-1,d0
        lea (_args,pc),a0
        jsr (4,a1)
        move.l d0,d6
        ; Preserve transaction counters before UnLoadSeg for the dump auditor.
        move.l (_storage_stats,pc),a0
        lea (_saved_stats,pc),a1
        moveq #4,d0
.stats  move.l (a0)+,(a1)+
        dbf d0,.stats
        move.l 4.w,a6
        lea (_stack,pc),a0
        jsr (_LVOStackSwap,a6)
        move.l (_stackmem,pc),a1
        move.l #16384,d0
        jsr (_LVOFreeMem,a6)
        move.l (sp)+,a6
        move.l (_oldhome,pc),d1
        jsr (_LVOSetProgramDir,a6)
        move.l d0,d1
        jsr (_LVOUnLock,a6)
        move.l d7,d1
        jsr (_LVOUnLoadSeg,a6)
        move.l (_oldargs,pc),d1
        jsr (_LVOSetArgStr,a6)
        move.l a6,a1
        move.l 4.w,a6
        jsr (_LVOCloseLibrary,a6)
        tst.l d6
        bne .gameerror
        IFD TEST
        lea (_exitmark,pc),a0
        bsr _mark
        ENDC
        pea TDREASON_OK
        bra .abort
.readerror
        jsr (_LVOIoErr,a6)
        pea (_program,pc)
        move.l d0,-(sp)
        pea TDREASON_DOSREAD
        bra .abort
.oserror
        clr.l -(sp)
        clr.l -(sp)
        pea TDREASON_OSEMUFAIL
        bra .abort
.gameerror
        pea (_failed,pc)
        pea TDREASON_FAILMSG
.abort
        move.l (_resload,pc),a2
        jmp (resload_Abort,a2)
_failed dc.b "Slicks could not start. Check the installed original data files.",0
_current dc.b 0
        EVEN
_stackmem dc.l 0
_stack dc.l 0,0,0
_oldhome dc.l 0
_oldargs dc.l 0
_storage_stats dc.l 0
        dc.b "SLKSTAT1"
_saved_stats dc.l 0,0,0,0,0
_switches dc.l 0
_switch_tags dc.l WHDLTAG_CBSWITCH_SET
        dc.l 0,0

_patch_storage
        move.l d7,d0
.seg    tst.l d0
        beq .missing
        add.l d0,d0
        add.l d0,d0
        move.l d0,a0
        move.l (-4,a0),d1
        move.l (a0)+,d0
        sub.l #48,d1          ; segment overhead plus complete 40-byte block
        bmi .seg
        move.l a0,a1
        add.l d1,a1
.scan   cmpa.l a1,a0
        bhi .seg
        cmp.l #$534c4b53,(a0)+
        bne .scan
        cmp.l #$494f3031,(a0)
        bne .scan
        cmp.l #$00010028,(4,a0)
        bne .scan
        tst.l (8,a0)
        bne .missing
        lea (_whole_save,pc),a1
        move.l a1,(8,a0)
        lea (_switches,pc),a1
        move.l a1,(12,a0)
        lea (16,a0),a0
        lea (_storage_stats,pc),a1
        move.l a0,(a1)
        lea (_switch_tags,pc),a0
        lea (_count_switch,pc),a1
        move.l a1,(4,a0)
        jsr (resload_Control,a2)
        rts
.missing
        pea (_storage_missing,pc)
        pea TDREASON_FAILMSG
        jmp (resload_Abort,a2)

; C ABI: name, bytes, size, error pointer. One resload operation creates and
; writes the complete file: no KickFS empty-file creation or partial writes.
_whole_save
        move.l a2,-(sp)
        move.l (8,sp),a0
        move.l (12,sp),a1
        move.l (16,sp),d0
        move.l (_resload,pc),a2
        jsr (resload_SaveFile,a2)
        move.l (20,sp),a0
        move.l d1,(a0)
        move.l (sp)+,a2
        rts
_count_switch
        ; CBSWITCH has no usable stack and returns through A0, not RTS.
        ; Only D0/D1 may be clobbered; preserve the borrowed address register.
        move.l a1,d1
        lea (_switches,pc),a1
        addq.l #1,(a1)
        move.l d1,a1
        jmp (a0)
_storage_missing dc.b "Slicks whole-file save interface missing or invalid.",0
        EVEN
        IFD TEST
_bootearly
        move.l (_resload,pc),a2
        lea (_earlymark,pc),a0
        bra _mark
_mark
        movem.l d0-d1/a0-a1,-(sp)
        lea (_marker,pc),a1
        moveq #4,d0
        jsr (resload_SaveFile,a2)
        movem.l (sp)+,d0-d1/a0-a1
        rts
_marker dc.b "PASS"
_earlymark dc.b "test-early",0
_bootmark dc.b "test-bootdos",0
_loadmark dc.b "test-loaded",0
_exitmark dc.b "test-returned",0
        EVEN
        ENDC
