OUT      = out/SlicksDiag
CC       = m68k-amiga-elf-gcc
AS       = m68k-amiga-elf-as
VASM     = vasmm68k_mot
OBJDUMP  = m68k-amiga-elf-objdump
SDKDIR   = $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  = $(abspath $(dir $(shell which $(CC)))../../support)
REPO     = ..

CCFLAGS  = -g -MP -MMD -m68020 -msoft-float -O2 -nostdlib \
	-fomit-frame-pointer -fno-tree-loop-distribution \
	-ffunction-sections -fdata-sections -Wall -Wextra -Werror \
	-Wno-volatile-register-var
ASFLAGS  = -mcpu=68020 -g --register-prefix-optional -I"$(SDKDIR)"
VASMFLAGS = -m68020 -Felf -quiet -x -no-opt -nowarn=62
LDFLAGS  = -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

OBJECTS = obj/slicks_diag.o obj/gcc8_c_support.o obj/gcc8_a_support.o \
	obj/sgfx_plot_plane.o

.PHONY: all clean

all: $(OUT).exe

obj out:
	@mkdir -p $@

obj/slicks_diag.o: $(REPO)/src/platform/amiga/slicks_diag.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s | obj
	$(AS) $(ASFLAGS) -o $@ $<

obj/sgfx_plot_plane.o: $(REPO)/native/sgfx_plot_plane.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

$(OUT).elf: $(OBJECTS) | out
	$(CC) $(CCFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s 2>/dev/null || true

$(OUT).exe: $(OUT).elf
	elf2hunk $< $@

clean:
	$(RM) -r obj out

-include obj/*.d
