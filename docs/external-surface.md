# Measured external surface

This is the currently observed DOS/BIOS/interrupt boundary for the bounded
BASIC.SS reference race. It is evidence for the narrow native runtime, not yet
a claim of exhaustive coverage across every menu, track, and game mode.

## Hardware vectors

| Vector | Observed native target | Role |
|---|---:|---|
| `08h` | `27A24h` | PC timer interrupt |
| `09h` | `26D29h` | PC keyboard interrupt |

Explicit vector tracing accounts for every otherwise unexplained transition in
the measured run. The timer handler is entered both from game code and while an
external service is active, so the Amiga replacement must preserve its
asynchronous state effects rather than compile it as an ordinary call.

## BIOS video (`INT 10h`)

Observed AH functions: `00h`, `08h`, `0Fh`, and `12h`. The captured `AH=00h`
call uses `AL=13h`, establishing VGA mode 13h as the principal display mode.
The runtime contract will need set/get mode, character/attribute readback used
during setup, and the observed alternate-select queries; it does not imply a
general VGA BIOS implementation.

## DOS (`INT 21h`)

| AH | DOS service |
|---:|---|
| `1Ah` | Set disk transfer address |
| `25h` | Set interrupt vector |
| `29h` | Parse filename |
| `2Ah` | Get date |
| `2Ch` | Get time |
| `2Fh` | Get disk transfer address |
| `30h` | Get DOS version |
| `35h` | Get interrupt vector |
| `37h` | Get/set switch character |
| `38h` | Get country information |
| `3Bh` | Change current directory |
| `3Dh` | Open file |
| `3Eh` | Close file |
| `3Fh` | Read file/device |
| `41h` | Delete file |
| `42h` | Seek file |
| `43h` | Get/set file attributes |
| `44h` | Device I/O control |
| `4Ah` | Resize memory block |
| `4Bh` | Execute program |
| `4Dh` | Get child return code |
| `4Eh` | Find first file |
| `4Fh` | Find next file |

The trace retains AX, BX, CX, DX, SI, DI, BP, SP, and FLAGS for every event, so
subfunction and argument shapes can be derived before implementing each native
service. Counts are deliberately not contractual because live race duration
and interrupt timing vary between runs.

## Multiplex and mouse

- `INT 2Fh AX=4300h` and `AX=4310h`: XMS installation and entry-point queries.
- `INT 33h AX=0000h`, `0003h`, `0008h`, and `0009h`: mouse reset/status,
  position/buttons, vertical bounds, and graphics-cursor definition.

## Still unmeasured

- Direct I/O port reads and writes.
- VGA memory reads/writes and dirty-region behavior.
- Writes into the runtime code image.
- Additional paths reached by other tracks, menus, multiplayer modes, and
  failure cases.
