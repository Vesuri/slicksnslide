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
CPPFLAGS = $(CCFLAGS) -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fcheck-new \
	-std=gnu++17 -fpermissive -Wno-write-strings -Wno-pointer-arith \
	-Wno-ignored-qualifiers -Wno-sign-compare \
	-Wno-parentheses -Wno-unused-parameter \
	-include "$(REPO)/src/platform/amiga/framework/SASCCompat.h" \
	-I"$(REPO)/src/platform/amiga/framework"
ASFLAGS  = -mcpu=68020 -g --register-prefix-optional -I"$(SDKDIR)"
VASMFLAGS = -m68020 -Felf -quiet -x -no-opt -nowarn=62 -I"$(SDKDIR)"
LDFLAGS  = -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map

OBJECTS = obj/slicks_diag.o obj/amiga_saved_files.o obj/amiga_platform.o obj/framework_runtime.o \
	obj/framework_amiga_hardware.o obj/framework_bitmap.o \
	obj/framework_copper_list.o obj/framework_palette24.o \
	obj/resource_archive.o obj/amiga_audio.o obj/amiga_player_menu.o obj/amiga_shop.o obj/amiga_setup_storage.o obj/track_scene.o \
	obj/race_runtime.o obj/particle_runtime.o obj/particle_draw.o obj/actor_allocate.o obj/car_draw.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o \
	obj/sgfx_plot_plane.o obj/sgfx_span_fill.o \
	obj/sgfx_planar_blit.o obj/sgfx_transparent_blit.o \
	obj/sgfx_planar_subrect_blit.o \
	obj/sgfx_title_pages.o obj/sgfx_title_crop.o obj/sutil_palette_nearest.o \
	obj/sui_title_step.o obj/sui_color_slot.o obj/sui_bevel.o obj/sui_text.o \
	obj/sui_title_menu.o obj/sui_title_tail.o \
	obj/sui_title_dispatch.o \
	obj/sui_font_glyph.o obj/sui_font_measure.o obj/sui_font_string.o \
	obj/sui_font_glyph_planar.o obj/sui_font_string_planar.o \
	obj/sui_menu_bridge.o \
	obj/sgfx_mult320.o \
	obj/sgfx_chunky_transparent_blit.o \
	obj/title_state.o obj/native_bridge.o obj/vga_to_chunky.o \
	obj/sgfx_chunky_asset_to_planar.o \
	obj/c2p1x1_8_c5_bm.o obj/sgfx_clear_full.o obj/sgfx_mode_setup.o

.PHONY: all clean verify-runtime

all: $(OUT).exe

verify-runtime: obj/gcc8_c_support.o
	$(PYTHON) $(REPO)/tools/verify_amiga_runtime.py $(OBJDUMP) $<

obj out:
	@mkdir -p $@

$(REPO)/src/gen/setup_defaults.h: $(REPO)/tools/export_setup_defaults.c \
        $(REPO)/src/game/configuration.h $(REPO)/disasm/runtime.bin
	$(MAKE) -C $(REPO) setup-defaults

$(REPO)/disasm/runtime.bin: $(REPO)/ref/SLICKS.EXE $(REPO)/tools/unpack_compack.c
	$(MAKE) -C $(REPO) unpack

obj/slicks_diag.o: $(REPO)/src/platform/amiga/slicks_diag.c $(REPO)/src/gen/setup_defaults.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_platform.o: $(REPO)/src/platform/amiga/amiga_platform.cpp \
		$(REPO)/src/platform/amiga/amiga_platform.h | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/framework_runtime.o: $(REPO)/src/platform/amiga/framework_runtime.cpp | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/framework_amiga_hardware.o: \
		$(REPO)/src/platform/amiga/framework/AmigaHardware.cpp | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/framework_bitmap.o: $(REPO)/src/platform/amiga/framework/Bitmap.cpp | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/framework_copper_list.o: \
		$(REPO)/src/platform/amiga/framework/CopperList.cpp | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/framework_palette24.o: \
		$(REPO)/src/platform/amiga/framework/Palette24Bit.cpp | obj
	$(CC) $(CPPFLAGS) -c -o $@ $<

obj/resource_archive.o: $(REPO)/src/platform/amiga/resource_archive.c \
		$(REPO)/src/platform/amiga/resource_archive.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_audio.o: $(REPO)/src/platform/amiga/amiga_audio.c \
		$(REPO)/src/platform/amiga/amiga_audio.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_player_menu.o: $(REPO)/src/platform/amiga/amiga_player_menu.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_shop.o: $(REPO)/src/platform/amiga/amiga_shop.c $(REPO)/src/platform/amiga/amiga_shop.h $(wildcard $(REPO)/src/ui/*.h) $(REPO)/src/game/weapon_shop.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_setup_storage.o: $(REPO)/src/platform/amiga/amiga_setup_storage.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/amiga_saved_files.o: $(REPO)/src/platform/amiga/amiga_saved_files.c | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/sui_menu_bridge.o: $(REPO)/src/ui/sui_menu_bridge.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/track_scene.o: $(REPO)/src/game/track_scene.c \
		$(REPO)/src/game/track_scene.h | obj
	$(CC) $(CCFLAGS) -c -o $@ $<

obj/race_runtime.o: $(REPO)/src/game/race_runtime.c \
		$(REPO)/src/game/race_runtime.h \
		$(REPO)/src/ui/font_resource.h \
		$(REPO)/src/game/track_scene.h | obj
	$(CC) $(CCFLAGS) -O3 -c -o $@ $<

obj/particle_runtime.o: $(REPO)/src/game/particle_runtime.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/particle_draw.o: $(REPO)/src/game/particle_draw.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/actor_allocate.o: $(REPO)/src/game/actor_allocate.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/car_draw.o: $(REPO)/src/game/car_draw.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c Makefile | obj
	# Runtime implementations must not be folded into calls to themselves.
	$(CC) $(CCFLAGS) -fno-builtin -c -o $@ $<

obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s | obj
	$(AS) $(ASFLAGS) -o $@ $<

obj/sgfx_plot_plane.o: $(REPO)/src/graphics/sgfx_plot_plane.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_checker_fill.o: $(REPO)/src/graphics/sgfx_checker_fill.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_planar_blit.o: $(REPO)/src/graphics/sgfx_planar_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_transparent_blit.o: $(REPO)/src/graphics/sgfx_transparent_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_planar_subrect_blit.o: \
		$(REPO)/src/graphics/sgfx_planar_subrect_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_title_pages.o: $(REPO)/src/graphics/sgfx_title_pages.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_title_crop.o: $(REPO)/src/graphics/sgfx_title_crop.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sutil_palette_nearest.o: $(REPO)/src/util/sutil_palette_nearest.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_step.o: $(REPO)/src/ui/sui_title_step.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_color_slot.o: $(REPO)/src/ui/sui_color_slot.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_span_fill.o: $(REPO)/src/graphics/sgfx_span_fill.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_bevel.o: $(REPO)/src/ui/sui_bevel.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_text.o: $(REPO)/src/ui/sui_text.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_font_string_planar.o: $(REPO)/src/ui/sui_font_string.s | obj
	$(VASM) $(VASMFLAGS) -DSLICKS_FONT_PLANAR=1 -o $@ $<
obj/sui_font_glyph_planar.o: $(REPO)/src/ui/sui_font_glyph_planar.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_font_glyph.o: $(REPO)/src/ui/sui_font_glyph.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_mult320.o: $(REPO)/src/graphics/sgfx_mult320.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_chunky_transparent_blit.o: $(REPO)/src/graphics/sgfx_chunky_transparent_blit.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_font_measure.o: $(REPO)/src/ui/sui_font_measure.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_font_string.o: $(REPO)/src/ui/sui_font_string.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_menu.o: $(REPO)/src/ui/sui_title_menu.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_tail.o: $(REPO)/src/ui/sui_title_tail.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sui_title_dispatch.o: $(REPO)/src/ui/sui_title_dispatch.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/title_state.o: $(REPO)/src/platform/amiga/title_state.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/native_bridge.o: $(REPO)/src/platform/amiga/native_bridge.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/vga_to_chunky.o: $(REPO)/src/platform/amiga/vga_to_chunky.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_chunky_asset_to_planar.o: \
		$(REPO)/src/graphics/sgfx_chunky_asset_to_planar.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/c2p1x1_8_c5_bm.o: \
		$(REPO)/src/platform/amiga/c2p1x1_8_c5_bm.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_clear_full.o: $(REPO)/src/graphics/sgfx_clear_full.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sgfx_mode_setup.o: $(REPO)/src/graphics/sgfx_mode_setup.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

$(OUT).elf: $(OBJECTS) | out verify-runtime
	$(CC) $(CCFLAGS) $(LDFLAGS) $(OBJECTS) -o $@
	$(OBJDUMP) --disassemble --no-show-raw-ins -S $@ > $(OUT).s 2>/dev/null || true

$(OUT).exe: $(OUT).elf
	elf2hunk $< $@

clean:
	$(RM) -r obj out

-include obj/*.d
