# SlicksInstallData

Self-contained Amiga/host extraction helper for the publisher's Slix151.zip.
See ../../docs/install-original-data.md for input validation, memory use,
installation/update semantics and tests. The helper creates a new destination
only and refuses to overwrite an existing one. No registration file is read,
generated or extracted.

Portable I/O/startup/SHA-256 scaffolding is adapted from the user's Vette
installer project. `puff.c` and `puff.h` are Mark Adler's puff 2.3 from zlib
v1.3.1 (https://github.com/madler/zlib/tree/v1.3.1/contrib/puff).
The original notice in puff.h applies to that decoder. The marked Amiga-only
change selects GCC built-in non-local jumps instead of requiring a C library.
No StuffIt/NDIF/HFS decoder or LGPL component from Vette is used here.

`make` builds the host helper; `make amiga` requires the shared cross-toolchain
environment; `make test` checks extraction against an independent ZIP reader.
The Amiga helper uses about 958 KiB static working storage and no heap image.
Native extraction was checked with a 4096-byte stack (1246 bytes spare).
Choose a disk staging drawer on stock 2 MiB machines.
