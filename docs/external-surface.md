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

## Direct port I/O

The aggregate tracer reduced 10,290,225 observed operations to 885 unique
`(direction, width, port, value, resume offset)` rows. The volume is dominated
by polling and inner-loop VGA programming; reproducing those operations one by
one on the Amiga would defeat the purpose of ahead-of-time translation.

| Ports | Observed role |
|---|---|
| `20h`, `21h` | PIC acknowledgement and interrupt mask |
| `40h`, `43h` | PIT channel 0 reads and control writes |
| `60h`, `61h` | Keyboard data and PC speaker/PPI control |
| `00h`-`0Fh`, `83h` | DMA channel 1 programming for digital audio |
| `201h` | Joystick polling |
| `226h`, `22Ch` | Sound Blaster DSP reset, command, and status |
| `3C0h`, `3C4h`-`3C5h`, `3CEh`-`3CFh` | VGA attribute, sequencer, and graphics-controller programming |
| `3C8h`-`3C9h` | VGA palette index and data |
| `3D4h`-`3D5h` | VGA CRTC programming |
| `3DAh` | VGA status/retrace polling |

The BASIC race alone performs millions of PIT and VGA-status reads and
hundreds of thousands of graphics-controller writes. The native design should
recognize and replace the surrounding timing, palette, and drawing routines;
it should not implement these as generic per-port calls. The sequencer and
graphics-controller traffic also proves that treating mode `13h` as only a
flat 320x200 framebuffer would be insufficient for faithful translation.

## Still unmeasured

- VGA memory reads/writes and dirty-region behavior.
- Writes into the runtime code image.
- Additional paths reached by other tracks, menus, multiplayer modes, and
  failure cases.
