# Direct native translation architecture

This is the design for an automatic translator. The shipped port did not
complete it: each routine was instead recovered into native C/C++ or hand-written
68020 assembly under the same rules (byte-exact little-endian guest data,
oracle-tested against the original x86 code). See [phases.md](phases.md).

The production path never represents the running 286 as generated C operating
on a CPU structure. A context exists only as the interchange format at runtime
boundaries.

```text
MZ/runtime bytes
  -> decoded x86 instructions
  -> typed semantic IR
  -> basic blocks and superblocks
  -> liveness + flag-demand analysis
  -> 68020 register allocation
  -> direct assembly and block chaining
```

## Native-region invariants

- Guest values stay in native registers across compatible block edges.
- A direct guest branch is a direct native branch.
- Ordinary guest RAM does not pass through a generic memory helper.
- A complete x86 flags word is built only when a guest operation observes it.
- Parity and auxiliary carry are computed only when live.
- Runtime exits publish exactly the guest state required by their contract.
- Every write to a translated guest address is classified.

## Runtime boundaries

The native region may exit for an unresolved indirect target, DOS/BIOS call,
interrupt, fault, port I/O, special memory, overlay transition, or fallback.
Each boundary has an explicit input/output state contract so that it cannot
quietly grow into per-instruction emulation.

## Performance audits

The generated build will report or reject unexpected context loads/stores,
generic memory helpers, full flag materializations, dispatcher exits, and
interpreted blocks. These are architectural regressions, not merely profiling
observations.
