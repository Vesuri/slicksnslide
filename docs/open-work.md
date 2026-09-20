# Open work

1. Copy the complete local Slicks distribution into `ref/`; the executable
   references companion data, configuration, player, track, record, help,
   graphics, and `*.SS` track files.
2. Validate the host Compack capture against an independent Bochs or DOSBox run.
3. Choose a stable reference milestone and completion signal.
4. Trace executed blocks, indirect targets, DOS/BIOS calls, ports, video memory,
   and executable writes on a 286-class reference CPU.
5. Classify the optional 386 Borland-runtime path and prove it remains dormant
   under the selected CPU identity.
6. Cross-check the reconstructed MZ in an independent DOS loader before using
   it as the canonical address-sensitive analysis input.
