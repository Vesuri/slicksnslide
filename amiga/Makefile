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
VASMFLAGS = -m68020 -Felf -quiet -x -no-opt -nowarn=62 -I"$(SDKDIR)"
LDFLAGS  = -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

OBJECTS = obj/slicks_diag.o obj/resource_archive.o obj/track_scene.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o \
	obj/sgfx_plot_plane.o obj/sgfx_span_fill.o \
	obj/sgfx_planar_blit.o obj/sgfx_transparent_blit.o \
	obj/sgfx_planar_subrect_blit.o \
	obj/sgfx_title_pages.o obj/sgfx_title_crop.o obj/sutil_palette_nearest.o \
	obj/sui_title_step.o obj/sui_color_slot.o obj/sui_bevel.o obj/sui_text.o \
	obj/sui_title_menu.o obj/sui_title_tail.o \
	obj/sui_title_dispatch.o \
	obj/title_state.o obj/native_bridge.o obj/vga_to_chunky.o \
	obj/sgfx_chunky_asset_to_planar.o \
	obj/c2p1x1_8_c5_bm.o obj/sgfx_clear_full.o obj/sgfx_mode_setup.o

.PHONY: all clean

all: $(OUT).exe

obj out:
	@mkdir -p $@

obj/slicks_diag.o: $(REPO)/src/platform/amiga/slicks_diag.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/resource_archive.o: $(REPO)/src/platform/amiga/resource_archive.c \
		$(REPO)/src/platform/amiga/resource_archive.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/track_scene.o: $(REPO)/src/game/track_scene.c \
		$(REPO)/src/game/track_scene.h | obj
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

obj/sgfx_transparent_blit.o: $(REPO)/native/sgfx_transparent_blit.s | obj
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

obj/sui_color_slot.o: $(REPO)/native/sui_color_slot.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_span_fill.o: $(REPO)/native/sgfx_span_fill.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_bevel.o: $(REPO)/native/sui_bevel.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_text.o: $(REPO)/native/sui_text.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_menu.o: $(REPO)/native/sui_title_menu.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_tail.o: $(REPO)/native/sui_title_tail.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_dispatch.o: $(REPO)/native/sui_title_dispatch.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/title_state.o: title_state.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/native_bridge.o: native_bridge.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/vga_to_chunky.o: $(REPO)/src/platform/amiga/vga_to_chunky.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_chunky_asset_to_planar.o: \
		$(REPO)/native/sgfx_chunky_asset_to_planar.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/c2p1x1_8_c5_bm.o: \
		$(REPO)/src/platform/amiga/c2p1x1_8_c5_bm.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_clear_full.o: $(REPO)/native/sgfx_clear_full.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_mode_setup.o: $(REPO)/native/sgfx_mode_setup.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

$(OUT).elf: $(OBJECTS) | out
	$(CC) $(CCFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s 2>/dev/null || true

$(OUT).exe: $(OUT).elf
	elf2hunk $< $@

clean:
	$(RM) -r obj out

-include obj/*.d
