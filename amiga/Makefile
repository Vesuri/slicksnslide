OUT      = out/SlicksDiag
CC       = m68k-amiga-elf-gcc
AS       = m68k-amiga-elf-as
VASM     = vasmm68k_mot
OBJDUMP  = m68k-amiga-elf-objdump
PYTHON   = python3
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
	obj/sgfx_planar_blit.o obj/sgfx_planar_subrect_blit.o \
	obj/sgfx_title_pages.o obj/sgfx_title_crop.o obj/sutil_palette_nearest.o \
	obj/sui_title_step.o obj/basic_frame.o obj/native_bridge.o

CAPTURE_DIR = $(REPO)/tmp/pc-fixed
FRAME_HASH = a4fc8a1cbea08a30
PALETTE_HASH = 9b17b223ef7f93e3

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

obj/sgfx_checker_fill.o: $(REPO)/native/sgfx_checker_fill.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_planar_blit.o: $(REPO)/native/sgfx_planar_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_planar_subrect_blit.o: \
		$(REPO)/native/sgfx_planar_subrect_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_title_pages.o: $(REPO)/native/sgfx_title_pages.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_title_crop.o: $(REPO)/native/sgfx_title_crop.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sutil_palette_nearest.o: $(REPO)/native/sutil_palette_nearest.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_step.o: $(REPO)/native/sui_title_step.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/basic-frame.bin: $(CAPTURE_DIR)/slicks-sprite-blobs.csv \
		$(CAPTURE_DIR)/slicks-sprite-blobs.bin \
		$(REPO)/tools/extract_capture_blob.py | obj
	$(PYTHON) $(REPO)/tools/extract_capture_blob.py \
		$(CAPTURE_DIR)/slicks-sprite-blobs.csv \
		$(CAPTURE_DIR)/slicks-sprite-blobs.bin $(FRAME_HASH) $@

obj/basic-palette.bin: $(CAPTURE_DIR)/slicks-palettes.csv \
		$(CAPTURE_DIR)/slicks-palettes.bin \
		$(REPO)/tools/extract_capture_blob.py | obj
	$(PYTHON) $(REPO)/tools/extract_capture_blob.py \
		$(CAPTURE_DIR)/slicks-palettes.csv \
		$(CAPTURE_DIR)/slicks-palettes.bin $(PALETTE_HASH) $@

obj/basic_frame.o: basic_frame.s obj/basic-frame.bin obj/basic-palette.bin | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/native_bridge.o: native_bridge.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

$(OUT).elf: $(OBJECTS) | out
	$(CC) $(CCFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s 2>/dev/null || true

$(OUT).exe: $(OUT).elf
	elf2hunk $< $@

clean:
	$(RM) -r obj out

-include obj/*.d
