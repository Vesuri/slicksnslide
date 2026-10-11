# Slicks — DOS 286 to Amiga native static recompiler

Port the supplied `SLICKS.EXE` to an A1200 (68020, 2 MiB) by translating its
16-bit x86 machine code directly to native 68020 code. Fidelity comes before
improvement, and the original executable running under a PC emulator is the
ground truth.

## Shared environment and knowledge

The toolchain, emulators, Kickstarts, WHDLoad and the Python environment come from AmigaXDev
(`make setup` there); `amiga/env.sh` sources `~/.local/share/amiga/env.sh`. Lessons that hold
for any port live in AmigaXDev's docs: add or correct them there, and keep only what is
specific to this game in `docs/`.

| AmigaXDev doc | For |
| --- | --- |
| `docs/environment.md` | the shared variables and installed files |
| `docs/playbooks/new-port-playbook.md` | the phases of a port and their gates |
| `docs/reference/amiga-hardware.md` | Chip bus, takeover, interrupts, copper, blitter, Paula, AGA, the dA JoRMaS framework |
| `docs/reference/m68k-optimisation.md` | 68000/68020 cost model and code generation |
| `docs/reference/measurement-method.md` | profiling instruments, A/B discipline, pricing a change |
| `docs/reference/validation-harness.md` | faithfulness to the original: references, differentials, sabotage |
| `docs/reference/headless-fsuae.md` | the shared launcher, FS-UAE builds, gdb stub, diag runs |
| `docs/reference/port-toolchain.md` | flags, linking, link audits, WHDLoad slaves, release archives |
| `docs/native-port-validation.md` | crash dumps as regressions, installer updates, release archive checks |

## Standing rules

- Commit each coherent, verified piece of work before moving to the next;
  do not accumulate unrelated completed work in the working tree. Use the
  repository's Vesuri identity, disable hooks/signing, and omit co-author
  trailers. Keep current actionable work in `docs/open-work.md`; historical
  evidence and completed-item reports belong in separate documents.
- This file carries rules and pointers, never dated progress notes. Put measured
  findings and history in `docs/`.
- `SLICKS.EXE`, runtime dumps, traces, screenshots, and other byte-derived game
  material are local-only and must never be committed.
- The Compack entry point is an unpacker. No game disassembly is trusted until
  the post-unpack image and entry state have been captured from the reference
  machine and independently reproduced.
- There is no generated-C CPU-emulation stage. The production path is x86
  decode -> semantic IR -> optimized 68020 assembly.
- A CPU context may exist at runtime exits and in a fallback interpreter. Native
  translated regions must not routinely load and store a CPU structure.
- Direct guest edges become direct native edges. Do not return to a dispatcher
  after every instruction or every statically resolved block.
- Keep guest registers resident across translated blocks, keep flags virtual,
  and materialize full x86 state only at a demonstrated boundary.
- Guest RAM remains byte-exact and little-endian. Every native word/long access
  must make the endian and alignment rule explicit.
- Unknown control flow, writes to translated code, overlays, interrupts, port
  I/O, and device memory must fail loudly until deliberately supported.
- Every instruction semantic and every backend lowering needs differential
  tests against an independent x86 oracle. Tests, not agent confidence, settle
  correctness.
- Ground-truth comparisons use the original executable under the PC reference
  loop, never a host approximation or the Amiga port compared with itself.
- Profile an end-to-end target skeleton before choosing optimization work or
  promising a performance target.
- Fail performance experiments cheaply: quick correctness smoke tests, then a
  benchmark against the parent, before the expensive fidelity gates (AmigaXDev
  `docs/reference/measurement-method.md`, the experiment loop). Promising changes
  still pass every applicable fidelity gate and a final performance confirmation.
- Reuse the Amiga build, FS-UAE/GDB, measurement and documentation practices in
  AmigaXDev's docs; do not re-derive target-side lessons without evidence.

## Gates, in order

1. Build an unattended PC reference loop that drives the original and exposes
   guest state.
2. Capture and prove the post-Compack runtime image and true entry state.
3. Complete the combined static/live entry-point and hardware-surface map.
4. Prove x86 semantics and 68020 lowering on isolated instructions and blocks.
5. Run a translated end-to-end skeleton on the target before optimizing.

See `README.md` for the project overview, `docs/open-work.md` for current work
and `docs/phases.md` for how the gates were met.
