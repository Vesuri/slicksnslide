# dA JoRMaS Amiga framework

These sources come from `dA JoRMaS/Productions/DanceDiverse3/Source`. Slicks
uses the framework's `AmigaHardware`, `Bitmap`, `CopperList`, and
`Palette24Bit` classes directly for its AGA display. The `Palette.h`,
`Sprite.h`, and `Util.h` headers are kept because those classes include them;
their unused implementation files are not part of the port (the originals
remain in git history).

`SASCCompat.h` and the small conditional declarations in the headers adapt
the original SAS/C interfaces to the project's `m68k-amiga-elf-g++` build.
The few C++ source edits only make SAS/C's old for-loop scoping and const
permissiveness explicit; they do not change the framework's Amiga behavior.
