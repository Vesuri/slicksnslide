OUT      = out/SlicksDiag
CC       = m68k-amiga-elf-gcc
AS       = m68k-amiga-elf-as
VASM     = vasmm68k_mot
OBJDUMP  = m68k-amiga-elf-objdump
PYTHON   = python3
SDKDIR   = $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
SUPPORT  = $(abspath $(dir $(shell which $(CC)))../../support)
REPO     = ..
GAMEPLAY_CFLAGS ?= -O3
INNER_PROFILE ?= 0
TITLEPROFILE ?= 0
ifneq ($(TITLEPROFILE),0)
ifneq ($(TITLEPROFILE),1)
$(error TITLEPROFILE must be 0 or 1)
endif
endif
# Opt-in title phase timestamps; absent from the normal binary.
ifeq ($(TITLEPROFILE),1)
obj/slicks_diag.o: CCFLAGS += -DSLICKS_TITLE_PROFILE
endif
DIV100CHECK ?= 0
ifneq ($(DIV100CHECK),0)
ifneq ($(DIV100CHECK),1)
$(error DIV100CHECK must be 0 or 1)
endif
endif
ifneq ($(INNER_PROFILE),0)
ifneq ($(INNER_PROFILE),1)
$(error INNER_PROFILE must be 0 or 1)
endif
endif

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
VASMFLAGS = -m68020 -Felf -quiet -x -no-opt -nowarn=62 -I"$(SDKDIR)" -Iobj
# SHADOW=1: diagnostic dual execution of native replacements (never release).
ifeq ($(SHADOW),1)
SHADOW_SITES ?= 0x3fe
CCFLAGS += -DSLICKS_SHADOW_CHECK -DSLICKS_SHADOW_SITES=$(SHADOW_SITES)
endif
# RETCHECK=1: compare every racing update with a non-retention reference.
ifeq ($(RETCHECK),1)
CCFLAGS += -DSLICKS_RETENTION_CHECK
endif
ifeq ($(DIV100CHECK),1)
CCFLAGS += -DSLICKS_DIV100_CHECK
endif
LDFLAGS  = -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map
ifeq ($(INNER_PROFILE),1)
CCFLAGS += -DSLICKS_INNER_PROFILE
endif

OBJECTS = obj/slicks_diag.o obj/amiga_saved_files.o obj/amiga_platform.o obj/framework_runtime.o \
	obj/framework_amiga_hardware.o obj/framework_bitmap.o \
	obj/framework_copper_list.o obj/framework_palette24.o \
	obj/resource_archive.o obj/amiga_audio.o obj/amiga_player_menu.o obj/amiga_shop.o obj/amiga_setup_storage.o obj/track_scene.o \
	obj/race_runtime.o obj/car_motion.o obj/car_emission.o obj/track_motion.o obj/dirty_rect.o obj/car_render.o obj/sprite_retention.o obj/particle_runtime.o obj/particle_draw.o obj/actor_allocate.o obj/car_draw.o obj/sprite_opaque.o obj/track_sprite_fast.o \
	obj/track_sprite_animation.o obj/track_sprite_animation_publish.o \
	obj/gcc8_c_support.o obj/gcc8_a_support.o obj/memory.o obj/version.o \
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
	obj/c2p1x1_8_c5_bm.o obj/sgfx_clear_full.o obj/sgfx_mode_setup.o \
	obj/pc_sampler.o

OBJECTS += obj/hud_restore.o
OBJECTS += obj/c2p16_interleaved.o
OBJECTS += obj/actor_order.o
OBJECTS += obj/point_restore.o
OBJECTS += obj/retention_touch.o
OBJECTS += obj/car_collision.o

.PHONY: all clean verify-runtime
# Never leave a truncated executable that looks up to date after elf2hunk fails.
.DELETE_ON_ERROR:

all: $(OUT).exe

# A missing mode stamp must force compilation even with equal one-second
# timestamps (older make/macOS). Publish the stamp only after compilation
# succeeds, so a failed switch cannot certify an object from the old mode.
ifeq ($(wildcard obj/inner-profile-$(INNER_PROFILE).stamp),)
INNER_PROFILE_FORCE = force-inner-profile
.PHONY: force-inner-profile
force-inner-profile:
endif

ifeq ($(wildcard obj/div100-check-$(DIV100CHECK).stamp),)
DIV100_CHECK_FORCE = force-div100-check
.PHONY: force-div100-check
force-div100-check:
endif

ifeq ($(wildcard obj/title-profile-$(TITLEPROFILE).stamp),)
TITLE_PROFILE_FORCE = force-title-profile
.PHONY: force-title-profile
force-title-profile:
endif

obj/point_restore.o: $(REPO)/src/game/point_restore.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/retention_touch.o: $(REPO)/src/game/retention_touch.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/car_collision.o: $(REPO)/src/game/car_collision.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/actor_order.o: $(REPO)/src/game/actor_order.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/c2p16_interleaved.o: $(REPO)/src/platform/amiga/c2p16_interleaved.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/hud_restore.o: $(REPO)/src/game/hud_restore.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

verify-runtime: obj/gcc8_c_support.o
	$(PYTHON) $(REPO)/tools/verify_amiga_runtime.py $(OBJDUMP) $<

obj out:
	@mkdir -p $@

$(REPO)/src/gen/setup_defaults.h: $(REPO)/tools/export_setup_defaults.c \
        $(REPO)/src/game/configuration.h $(REPO)/disasm/runtime.bin
	$(MAKE) -C $(REPO) setup-defaults

$(REPO)/disasm/runtime.bin: $(REPO)/ref/SLICKS.EXE $(REPO)/tools/unpack_compack.c
	$(MAKE) -C $(REPO) unpack

obj/slicks_diag.o: $(REPO)/src/platform/amiga/slicks_diag.c $(REPO)/src/gen/setup_defaults.h $(TITLE_PROFILE_FORCE) | obj
	$(CC) $(CCFLAGS) -c -o $@ $<
	rm -f obj/title-profile-0.stamp obj/title-profile-1.stamp
	touch obj/title-profile-$(TITLEPROFILE).stamp

obj/amiga_platform.o: $(REPO)/src/platform/amiga/amiga_platform.cpp \
        $(REPO)/src/platform/amiga/amiga_platform.h \
        $(REPO)/src/platform/amiga/amiga_key_scan.h | obj
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

obj/race_runtime.o: $(REPO)/src/game/race_runtime.c Makefile \
		$(INNER_PROFILE_FORCE) $(DIV100_CHECK_FORCE) \
		$(REPO)/src/game/sprite_retention.inc \
		$(REPO)/src/game/race_runtime.h \
		$(REPO)/src/ui/font_resource.h \
		$(REPO)/src/game/track_scene.h | obj
	$(CC) $(CCFLAGS) $(GAMEPLAY_CFLAGS) -c -o $@ $<
	rm -f obj/inner-profile-0.stamp obj/inner-profile-1.stamp
	touch obj/inner-profile-$(INNER_PROFILE).stamp
	rm -f obj/div100-check-0.stamp obj/div100-check-1.stamp
	touch obj/div100-check-$(DIV100CHECK).stamp

obj/particle_runtime.o: $(REPO)/src/game/particle_runtime.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sprite_opaque.o: $(REPO)/src/game/sprite_opaque.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/track_sprite_fast.o: $(REPO)/src/game/track_sprite_fast.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/track_sprite_animation.o: $(REPO)/src/game/track_sprite_animation.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/track_sprite_animation_publish.o: $(REPO)/src/game/track_sprite_animation_publish.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/particle_draw.o: $(REPO)/src/game/particle_draw.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/actor_allocate.o: $(REPO)/src/game/actor_allocate.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/race_offsets.i: $(REPO)/src/game/race_offsets.c $(REPO)/src/game/race_runtime.h $(REPO)/src/game/car_display.h Makefile | obj
	$(CC) $(CCFLAGS) -S -o obj/race_offsets.s $<
	sed -n 's/^@@//p' obj/race_offsets.s > $@

obj/car_motion.o: $(REPO)/src/game/car_motion.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/car_emission.o: $(REPO)/src/game/car_emission.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/track_motion.o: $(REPO)/src/game/track_motion.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

OBJECTS += obj/dirty_prune.o

obj/dirty_prune.o: $(REPO)/src/game/dirty_prune.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/dirty_rect.o: $(REPO)/src/game/dirty_rect.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/car_render.o: $(REPO)/src/game/car_render.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/sprite_retention.o: $(REPO)/src/game/sprite_retention.s obj/race_offsets.i | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/car_draw.o: $(REPO)/src/game/car_draw.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c Makefile | obj
	# Runtime implementations must not be folded into calls to themselves.
	$(CC) $(CCFLAGS) -fno-builtin -Dmemcpy=slicks_support_memcpy -Dmemset=slicks_support_memset -c -o $@ $<

obj/pc_sampler.o: $(REPO)/src/platform/amiga/pc_sampler.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/memory.o: $(REPO)/src/platform/amiga/memory.s | obj
	$(VASM) $(VASMFLAGS) -o $@ $<

obj/version.o: $(REPO)/src/platform/amiga/version.s | obj
	$(AS) $(ASFLAGS) -o $@ $<

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
