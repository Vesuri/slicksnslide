# Source and external-surface inventory

## Supplied artifact

The supplied `SLICKS.EXE` is a Compack-compressed DOS MZ executable. Its exact
identity and the derived runtime identity are recorded in `PROJECT.md`. Original
and byte-derived files are ignored by Git.

## Companion files named by the runtime

Static string evidence includes these required or potentially required files:

- `SLICKS.DAT`, `SLICKS.CFG`, `SLICKS.PLR`, `SLICKS.TRK`, and `SLICKS.REK`
- `SLICKS.000` and `HELP.TXT`
- `*.SS` track files
- bitmap/resource names including `loading.bmp` and `players.bmp`

The complete local distribution is now present under ignored `ref/`; no
original game material is committed. It contains the matching executable,
`SLICKS.DAT`, `SLICKS.000`, the track editor and its data, and 195 `*.SS` track
files. Reference runs use a disposable writable copy under `tmp/pc-root`, since
the game attempts to create files such as `KEYB.OUT`.

## Statically visible machine interfaces

The initial Ghidra sweep finds calls or instructions for:

- DOS `INT 21h`
- video BIOS `INT 10h`
- keyboard BIOS `INT 16h`
- mouse `INT 33h`
- PIT ports `40h` and `43h`
- additional `IN`/`OUT` operations whose devices require live classification

This is an upper-bound inventory, not proof that every site executes. Runtime
traces decide which services enter the Amiga compatibility layer.

## Instruction-level caveat

The unpacked Borland runtime includes a CPU dispatch and a reachable optional
386 implementation using operand-size overrides, `PUSHAD`/`POPAD`, `PUSHFD`,
and FS/GS. The older-CPU branch remains 16-bit. Translation coverage is based on
the selected reference CPU and observed control flow, while the static map keeps
both branches classified so data is not mistaken for code.
