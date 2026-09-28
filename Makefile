PYTHON ?= python3
SOURCE ?= ref/SLICKS.EXE
REFERENCE_ROOT ?= tmp/pc-root
REFERENCE_CAPTURE ?= $(REFERENCE_ROOT)/slicks-handoff
REFERENCE_FIXED_ROOT ?= tmp/pc-fixed
REFERENCE_TRACK ?= ref/TRACKS/BASIC.SS
REFERENCE_RACE_VIDEO ?=
REFERENCE_TRACE_BITMAP ?= $(REFERENCE_FIXED_ROOT)/slicks-executed.bin
REFERENCE_TRACE_EDGES ?= $(REFERENCE_FIXED_ROOT)/slicks-edges.csv
REFERENCE_INTERRUPTS ?= $(REFERENCE_FIXED_ROOT)/slicks-interrupts.csv
REFERENCE_PORTS ?= $(REFERENCE_FIXED_ROOT)/slicks-ports.csv
REFERENCE_MEMORY ?= $(REFERENCE_FIXED_ROOT)/slicks-memory.csv
REFERENCE_PRIMITIVES ?= $(REFERENCE_FIXED_ROOT)/slicks-primitive-calls.csv
LIVE_ENTRYPOINTS ?= disasm/live-entrypoints.csv
LIVE_VGA_SITES ?= disasm/live-vga-sites.csv
CC ?= cc
UNICORN_PREFIX ?= /opt/homebrew/opt/unicorn
VASM ?= $(HOME)/.local/vasmm68k_mot
M68K_CC ?= $(HOME)/.local/opt/bin/m68k-amiga-elf-gcc
JAVA_HOME ?= /opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home
export JAVA_HOME
export PATH := $(JAVA_HOME)/bin:$(PATH)

GHIDRA ?= tools/ghidra/ghidra_12.1_PUBLIC
DOSBOX_STAGING ?= dosbox-staging
DOSBOX_X ?= dosbox-x
DOSBOX_X_TRACE ?= tools/vendor/dosbox-x/src/dosbox-x
FFMPEG ?= ffmpeg
REFERENCE_VIDEO ?=
REFERENCE_FRAME_TIME ?= 8
ABS_ROOT := $(abspath .)

.PHONY: inspect hash prepare-reference prepare-fixed-reference \
	reference-staging reference-286 reference-race reference-trace \
	reference-frame-hash verify-reference-race verify-execution-trace \
	verify-interrupt-trace verify-port-trace analyze-execution-trace \
	verify-memory-trace verify-primitive-trace verify-race-state \
	analyze-race-velocity \
	analyze-vga-sites unpack rebuild-mz \
	verify-runtime verify-native-graphics trace-summary \
	verify-car-collision verify-drive-physics verify-surface-effects \
	verify-native-tracks \
	ghidra ghidra-normalized ghidra-live ghidra-live-normalized \
	amiga amiga-run amiga-debug amiga-check amiga-race-check \
	amiga-track-check amiga-lap-check amiga-results-check \
	amiga-ice-check amiga-zone-check amiga-restore-check todo clean

inspect:
	$(PYTHON) tools/mz_info.py $(SOURCE)

hash:
	shasum -a 256 $(SOURCE)

prepare-reference:
	@mkdir -p $(REFERENCE_ROOT)
	rsync -a ref/ $(REFERENCE_ROOT)/

prepare-fixed-reference:
	@mkdir -p $(REFERENCE_FIXED_ROOT)/TRACKS
	rsync -a --exclude TRACKS ref/ $(REFERENCE_FIXED_ROOT)/
	cp $(REFERENCE_TRACK) $(REFERENCE_FIXED_ROOT)/TRACKS/

reference-staging: prepare-reference
	$(DOSBOX_STAGING) --noprimaryconf --nolocalconf \
		--conf reference/dosbox-staging.conf \
		$(REFERENCE_ROOT)/SLICKS.EXE

reference-286: prepare-reference
	$(DOSBOX_X) -conf reference/dosbox-x-286.conf -nogui -nomenu -fastlaunch \
		-c "mount c $(abspath $(REFERENCE_ROOT))" -c "c:" \
		-c "slicks.exe"

reference-race: prepare-fixed-reference
	env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy $(DOSBOX_X) \
		-conf reference/dosbox-x-286.conf -set "cpu cycles=12000" \
		-nogui -nomenu -silent -fastlaunch -time-limit 36 \
		-c "mount c $(abspath $(REFERENCE_FIXED_ROOT))" -c "c:" \
		-c "autotype -w 15 -p 0.2 down enter , enter , enter , esc , up enter" \
		-c "dx-capture /v /-a /-d slicks.exe"

reference-trace: prepare-fixed-reference
	cd $(REFERENCE_FIXED_ROOT) && \
		env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
		$(abspath $(DOSBOX_X_TRACE)) \
		-conf $(ABS_ROOT)/reference/dosbox-x-286.conf \
		-set "cpu cycles=12000" -set "log logfile=basic-trace.log" \
		-nogui -nomenu -silent -fastlaunch -time-limit 75 \
		-c "mount c $(abspath $(REFERENCE_FIXED_ROOT))" -c "c:" \
		-c "autotype -w 15 -p 0.2 down enter , enter , enter , esc , up enter" \
		-c "dx-capture /v /-a /-d slicks.exe"

.PHONY: reference-layer-trace
REFERENCE_KEY_PACE ?= 1
reference-layer-trace: prepare-fixed-reference
	cd $(REFERENCE_FIXED_ROOT) && \
		env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SLICKS_TRACE_LAYERS=1 \
		$(abspath $(DOSBOX_X_TRACE)) \
		-conf $(ABS_ROOT)/reference/dosbox-x-286.conf \
		-set "cpu cycles=12000" -set "log logfile=layers-paced.log" \
		-nogui -nomenu -silent -fastlaunch -time-limit 75 \
		-c "mount c $(abspath $(REFERENCE_FIXED_ROOT))" -c "c:" \
		-c "autotype -w 15 -p $(REFERENCE_KEY_PACE) down enter , enter , enter , esc , up enter" \
		-c "slicks.exe"

reference-frame-hash:
	@test -n "$(REFERENCE_VIDEO)" || \
		(echo "set REFERENCE_VIDEO to a DOSBox-X AVI capture" >&2; exit 2)
	$(FFMPEG) -v error -ss $(REFERENCE_FRAME_TIME) -i $(REFERENCE_VIDEO) \
		-frames:v 1 -f rawvideo -pix_fmt rgb24 - | shasum -a 256

verify-reference-race:
	@test -n "$(REFERENCE_RACE_VIDEO)" || \
		(echo "set REFERENCE_RACE_VIDEO to the VGA AVI capture" >&2; exit 2)
	$(PYTHON) tools/verify_reference_race.py $(REFERENCE_RACE_VIDEO)

verify-execution-trace:
	$(PYTHON) tools/summarize_execution_trace.py \
		$(REFERENCE_TRACE_BITMAP) $(REFERENCE_TRACE_EDGES)

verify-interrupt-trace:
	$(PYTHON) tools/summarize_interrupt_trace.py $(REFERENCE_INTERRUPTS)

verify-port-trace:
	$(PYTHON) tools/summarize_port_trace.py $(REFERENCE_PORTS)

verify-memory-trace: unpack
	$(PYTHON) tools/summarize_memory_trace.py $(REFERENCE_MEMORY) \
		disasm/runtime.bin $(REFERENCE_TRACE_BITMAP)

verify-primitive-trace:
	$(PYTHON) tools/summarize_primitive_calls.py $(REFERENCE_PRIMITIVES)

verify-race-state:
	$(PYTHON) tools/verify_race_state.py \
		$(REFERENCE_FIXED_ROOT)/slicks-race-state.csv

analyze-race-velocity:
	$(PYTHON) tools/analyze_race_velocity.py \
		$(REFERENCE_FIXED_ROOT)/slicks-race-state.csv

analyze-vga-sites: verify-memory-trace
	$(PYTHON) tools/analyze_vga_sites.py disasm/runtime.bin \
		$(REFERENCE_MEMORY) --listing disasm/listing.txt \
		--output $(LIVE_VGA_SITES)

analyze-execution-trace: verify-execution-trace verify-interrupt-trace unpack
	$(PYTHON) tools/analyze_execution_trace.py disasm/runtime.bin \
		$(REFERENCE_TRACE_BITMAP) $(REFERENCE_TRACE_EDGES) \
		--interrupts $(REFERENCE_INTERRUPTS) \
		--entries $(LIVE_ENTRYPOINTS)

build/unpack_compack: tools/unpack_compack.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

unpack: build/unpack_compack
	@mkdir -p disasm
	build/unpack_compack $(SOURCE) disasm/runtime.bin \
		disasm/runtime-state.json disasm/runtime-relocations.csv

rebuild-mz: unpack
	$(PYTHON) tools/rebuild_runtime_mz.py \
		disasm/runtime.bin disasm/runtime-state.json \
		disasm/runtime-relocations.csv disasm/runtime.exe

verify-runtime: rebuild-mz
	$(PYTHON) tools/normalize_runtime_dump.py \
		disasm/runtime.bin disasm/runtime-state.json \
		disasm/runtime-relocations.csv disasm/runtime-normalized.bin
	$(PYTHON) tools/normalize_runtime_dump.py \
		$(REFERENCE_CAPTURE).bin $(REFERENCE_CAPTURE)-state.json \
		disasm/runtime-relocations.csv $(REFERENCE_CAPTURE)-normalized.bin
	cmp disasm/runtime-normalized.bin $(REFERENCE_CAPTURE)-normalized.bin
	@echo "independent normalized runtime: byte-exact match"

build/sgfx_plot_plane.bin: src/graphics/sgfx_plot_plane.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_car_collision: tools/verify_car_collision.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip $< src/game/race_runtime.c src/game/track_scene.c -o $@

verify-car-collision: build/verify_car_collision
	build/verify_car_collision

build/verify_drive_physics: tools/verify_drive_physics.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip $< src/game/track_scene.c -o $@

verify-drive-physics: build/verify_drive_physics
	build/verify_drive_physics

.PHONY: verify-dos-steering
build/verify_dos_steering: tools/verify_dos_steering.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c \
		-L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-dos-steering: build/verify_dos_steering
	build/verify_dos_steering disasm/runtime.bin

.PHONY: verify-dos-ai
build/verify_dos_ai: tools/verify_dos_ai.c src/game/race_runtime.c src/game/signed_division.h \
        src/game/race_runtime.h src/game/track_scene.h src/game/track_scene.c \
        src/ui/service_options.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c \
		-L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-dos-ai: build/verify_dos_ai
	build/verify_dos_ai disasm/runtime.bin

.PHONY: verify-dos-damage
.PHONY: verify-dos-hud
.PHONY: verify-weapon-actions
.PHONY: verify-weapon-shop
.PHONY: verify-moving-probe
.PHONY: verify-actor-slots
.PHONY: verify-weapon-fire
.PHONY: verify-weapon-simulation
.PHONY: verify-projectile-map
build/verify_projectile_map: tools/verify_projectile_map.c src/game/track_scene.c src/game/track_scene.h src/game/track_material_sample.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-projectile-map: build/verify_projectile_map
	./build/verify_projectile_map

build/verify_track_actor_motion build/verify_surface_effects build/verify_dos_damage build/verify_drive_trajectory build/verify_offroad_pool build/verify_weapon_actors_native build/verify_track_actor_render_native: src/game/track_material_sample.h
build/verify_weapon_simulation: tools/verify_weapon_simulation.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_simulation.inc src/game/weapon_actors.inc src/game/weapon_runtime.h src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-weapon-simulation: build/verify_weapon_simulation
	./build/verify_weapon_simulation
build/verify_weapon_fire: tools/verify_weapon_fire.c src/game/weapon_fire.h src/game/weapon_rules.h src/game/weapon_actions.h src/game/weapon_projectile.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-weapon-fire: build/verify_weapon_fire
	./build/verify_weapon_fire
build/verify_actor_slots: tools/verify_actor_slots.c src/game/actor_slots.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/actor_allocate.bin: tools/actor_allocate_test.s src/game/actor_allocate.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<
verify-actor-slots: build/verify_actor_slots build/actor_allocate.bin
	build/verify_actor_slots
build/verify_moving_probe: tools/verify_moving_probe.c src/game/moving_probe.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-moving-probe: build/verify_moving_probe
	build/verify_moving_probe
build/verify_weapon_shop: tools/verify_weapon_shop.c src/game/weapon_shop.h src/ui/shop_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-weapon-shop: build/verify_weapon_shop
	build/verify_weapon_shop
build/verify_weapon_actions: tools/verify_weapon_actions.c src/game/weapon_actions.h src/game/weapon_state.h src/game/weapon_projectile.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-weapon-actions: build/verify_weapon_actions
	build/verify_weapon_actions
.PHONY: verify-profile-setup
.PHONY: verify-configuration
.PHONY: verify-race-options
.PHONY: verify-profile-palette
.PHONY: verify-setup-session
build/verify_setup_session: tools/verify_setup_session.c src/game/setup_session.h src/game/profile_setup.h src/game/configuration.h src/game/race_options.h src/game/weapon_state.h src/game/race_rewards.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-setup-session: build/verify_setup_session
	build/verify_setup_session

build/verify_profile_palette: tools/verify_profile_palette.c src/game/profile_palette.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-profile-palette: build/verify_profile_palette
	build/verify_profile_palette

build/verify_race_options: tools/verify_race_options.c src/game/race_options.h src/game/configuration.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-race-options: build/verify_race_options
	build/verify_race_options

.PHONY: verify-intermission-menu
build/verify_intermission_menu: tools/verify_intermission_menu.c src/ui/intermission_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-intermission-menu: build/verify_intermission_menu
	build/verify_intermission_menu

.PHONY: verify-intermission-draw
build/verify_intermission_draw: tools/verify_intermission_draw.c tools/verify_profile_setup.c src/ui/intermission_draw.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-intermission-draw: build/verify_intermission_draw
	build/verify_intermission_draw

.PHONY: verify-intermission-prepare
build/verify_intermission_prepare: tools/verify_intermission_prepare.c tools/verify_palette_remap.c src/ui/intermission_prepare.h src/ui/palette_remap.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-intermission-prepare: build/verify_intermission_prepare
	build/verify_intermission_prepare

.PHONY: verify-intermission-renderer
build/verify_intermission_renderer: tools/verify_intermission_renderer.c $(wildcard src/ui/*.h) | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-intermission-renderer: build/verify_intermission_renderer
	build/verify_intermission_renderer

.PHONY: verify-change-cars-dialog
build/verify_change_cars_dialog: tools/verify_change_cars_dialog.c src/ui/change_cars_dialog.h src/game/profile_setup.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-change-cars-dialog: build/verify_change_cars_dialog
	build/verify_change_cars_dialog

.PHONY: verify-change-cars-renderer
build/verify_change_cars_renderer: tools/verify_change_cars_renderer.c src/ui/change_cars_renderer.h src/ui/change_cars_dialog.h src/ui/player_menu_renderer.h src/ui/saved_rectangle.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-change-cars-renderer: build/verify_change_cars_renderer
	build/verify_change_cars_renderer

.PHONY: verify-change-cars-pixels verify-intermission-pixels
build/verify_change_cars_pixels: tools/verify_change_cars_pixels.c tools/verify_palette_remap.c $(wildcard src/ui/*.h) tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-change-cars-pixels: build/verify_change_cars_pixels build/font_string_test.bin build/hud_icon_test.bin
	build/verify_change_cars_pixels

# Shared asset/68020 harness also checks the complete intermission UI layer.
verify-intermission-pixels: verify-change-cars-pixels

build/export_setup_defaults: tools/export_setup_defaults.c src/game/configuration.h src/ui/options_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

disasm/runtime.bin: $(SOURCE) build/unpack_compack
	$(MAKE) unpack

src/gen/setup_defaults.h: build/export_setup_defaults disasm/runtime.bin
	@mkdir -p src/gen
	build/export_setup_defaults disasm/runtime.bin $@

.PHONY: setup-defaults
setup-defaults: src/gen/setup_defaults.h

.PHONY: verify-setup-storage
build/verify_setup_storage: tools/verify_setup_storage.c src/game/setup_storage.h src/game/configuration.h src/game/player_profiles.h src/game/profile_setup.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-setup-storage: build/verify_setup_storage
	build/verify_setup_storage

build/verify_native_configuration: tools/verify_native_configuration.c src/game/configuration.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

.PHONY: verify-setup-load
build/verify_setup_load: tools/verify_setup_load.c src/platform/amiga/amiga_setup_storage.c src/platform/amiga/amiga_setup_storage.h src/game/configuration.h src/game/player_profiles.h src/game/setup_storage.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-setup-load: build/verify_setup_load
	build/verify_setup_load

.PHONY: verify-options-menu
build/verify_options_menu: tools/verify_options_menu.c src/ui/options_menu.h src/game/configuration.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-options-menu: build/verify_options_menu
	build/verify_options_menu

.PHONY: verify-options-draw
build/verify_options_draw: tools/verify_options_draw.c tools/verify_profile_setup.c src/ui/options_menu_draw.h src/ui/options_menu_renderer.h src/ui/options_menu.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-options-draw: build/verify_options_draw
	build/verify_options_draw

.PHONY: verify-controllers
build/verify_controllers: tools/verify_controllers.c tools/verify_options_menu.c src/ui/controllers_dialog.h src/ui/options_menu.h src/game/configuration.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-controllers: build/verify_controllers
	build/verify_controllers

.PHONY: verify-controllers-draw
build/verify_controllers_draw: tools/verify_controllers_draw.c tools/verify_profile_setup.c src/ui/controllers_dialog_draw.h src/ui/controllers_dialog.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-controllers-draw: build/verify_controllers_draw
	build/verify_controllers_draw

build/verify_configuration: tools/verify_configuration.c src/game/configuration.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-configuration: build/verify_configuration
	build/verify_configuration

.PHONY: verify-player-profiles
build/verify_player_profiles: tools/verify_player_profiles.c src/game/player_profiles.h src/game/profile_setup.h src/ui/profile_editor.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-player-profiles: build/verify_player_profiles
	build/verify_player_profiles

.PHONY: verify-text-entry
build/verify_text_entry: tools/verify_text_entry.c src/ui/text_entry.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-text-entry: build/verify_text_entry
	build/verify_text_entry

.PHONY: verify-colour-picker
build/verify_colour_picker: tools/verify_colour_picker.c src/ui/colour_picker.h src/ui/chunky_ui.h src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-colour-picker: build/verify_colour_picker
	build/verify_colour_picker

.PHONY: verify-menu-bitmap
build/verify_menu_bitmap: tools/verify_menu_bitmap.c src/ui/menu_bitmap.h tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-menu-bitmap: build/verify_menu_bitmap
	build/verify_menu_bitmap

.PHONY: verify-font-resource
.PHONY: verify-menu-restore
.PHONY: verify-menu-icon
build/verify_menu_icon: tools/verify_menu_icon.c tools/host_archive.h src/ui/menu_icon.h src/ui/chunky_ui.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-menu-icon: build/verify_menu_icon build/hud_icon_test.bin
	build/verify_menu_icon

.PHONY: verify-indexed-menu-icon
.PHONY: verify-amiga-key-scan
.PHONY: verify-driver-device
.PHONY: verify-amiga-joystick
.PHONY: verify-resource-archive
.PHONY: verify-help-index
.PHONY: verify-track-record-write
.PHONY: verify-track-storage
.PHONY: verify-track-menu
.PHONY: verify-track-menu-draw
.PHONY: verify-shop-draw
build/verify_shop_draw: tools/verify_shop_draw.c tools/verify_profile_setup.c src/ui/shop_draw.h src/ui/shop_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-shop-draw: build/verify_shop_draw
	build/verify_shop_draw
.PHONY: verify-shop-pixels
build/verify_shop_pixels: tools/verify_shop_pixels.c tools/verify_palette_remap.c src/ui/shop_draw.h src/ui/shop_menu.h $(wildcard src/ui/*.h) | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-shop-pixels: build/verify_shop_pixels build/font_string_test.bin
	build/verify_shop_pixels
.PHONY: verify-weapon-actors
.PHONY: verify-track-actor-assets
build/verify_track_actor_assets: tools/verify_track_actor_assets.c tools/verify_menu_icon.c src/game/track_scene.c src/game/track_actor_assets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-actor-assets: build/verify_track_actor_assets
	build/verify_track_actor_assets
.PHONY: verify-track-actors
build/verify_track_actor_motion: tools/verify_track_actor_motion.c tools/verify_dos_damage.c src/game/race_runtime.c src/game/signed_division.h src/game/track_actor_motion.inc src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_track_actor_render: tools/verify_track_actor_render.c tools/verify_menu_icon.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_offroad_pool: tools/verify_offroad_pool.c tools/verify_dos_damage.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_track_actor_setup: tools/verify_track_actor_setup.c tools/verify_dos_damage.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-actors: verify-track-actor-assets build/verify_track_actor_motion build/verify_track_actor_render build/verify_offroad_pool build/verify_track_actor_setup
	build/verify_track_actor_motion
	build/verify_track_actor_render
	build/verify_offroad_pool
	build/verify_track_actor_setup
build/verify_weapon_actors: tools/verify_weapon_actors.c tools/verify_menu_icon.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_weapon_actors_native: tools/verify_weapon_actors.c tools/native_sprite_oracle.h tools/verify_menu_icon.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -DSLICKS_NATIVE_SPRITE_TEST -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_track_actor_render_native: tools/verify_track_actor_render.c tools/native_sprite_oracle.h tools/verify_menu_icon.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -DSLICKS_NATIVE_SPRITE_TEST -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-weapon-actors: build/verify_weapon_actors build/verify_weapon_actors_native build/verify_track_actor_render_native build/car_draw.bin build/sprite_opaque.bin
	build/verify_weapon_actors
	build/verify_weapon_actors_native
	build/verify_track_actor_render_native
build/verify_track_menu_draw: tools/verify_track_menu_draw.c tools/verify_profile_setup.c src/ui/track_menu_draw.h src/ui/track_menu.h src/game/track_playlist.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-menu-draw: build/verify_track_menu_draw
	build/verify_track_menu_draw
.PHONY: verify-track-playlist
.PHONY: verify-track-lists
.PHONY: verify-race-rewards
build/verify_race_rewards: tools/verify_race_rewards.c src/game/race_rewards.h src/game/finish_rank.h src/game/setup_session.h src/game/profile_setup.h src/game/configuration.h src/game/race_options.h src/game/weapon_state.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-race-rewards: build/verify_race_rewards
	build/verify_race_rewards
.PHONY: verify-track-list-storage
.PHONY: verify-saved-game
.PHONY: verify-championship
.PHONY: verify-saved-files
build/verify_saved_files: tools/verify_saved_files.c src/platform/amiga/amiga_saved_files.c src/platform/amiga/amiga_saved_files.h | build
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -o $@ $<
verify-saved-files: build/verify_saved_files
	./build/verify_saved_files
build/verify_championship: tools/verify_championship.c src/game/championship.h src/game/setup_session.h src/game/saved_game.h src/game/saved_game_resume.h | build
	$(CC) -std=c11 -Wall -Wextra -Werror -O2 -o $@ $<
verify-championship: build/verify_championship
	./build/verify_championship
.PHONY: verify-saved-game-resume
build/verify_saved_game_resume: tools/verify_saved_game_resume.c tools/verify_configuration.c src/game/saved_game_resume.h src/game/saved_game.h src/game/player_profiles.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-saved-game-resume: build/verify_saved_game_resume
	build/verify_saved_game_resume
.PHONY: verify-saved-file-dialog
build/verify_saved_file_dialog: tools/verify_saved_file_dialog.c src/ui/saved_file_dialog.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-saved-file-dialog: build/verify_saved_file_dialog
	build/verify_saved_file_dialog
.PHONY: verify-saved-game-storage
build/verify_saved_game_storage: tools/verify_saved_game_storage.c tools/verify_track_storage.c src/platform/amiga/amiga_setup_storage.c src/platform/amiga/amiga_setup_storage.h src/game/saved_game.h src/game/setup_storage.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-saved-game-storage: build/verify_saved_game_storage
	build/verify_saved_game_storage
build/verify_saved_game: tools/verify_saved_game.c src/game/saved_game.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-saved-game: build/verify_saved_game
	build/verify_saved_game
.PHONY: verify-track-list-dialog
build/verify_track_list_dialog: tools/verify_track_list_dialog.c src/ui/track_list_dialog.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-list-dialog: build/verify_track_list_dialog
	build/verify_track_list_dialog
build/verify_track_list_storage: tools/verify_track_list_storage.c tools/verify_track_storage.c src/platform/amiga/amiga_setup_storage.c src/platform/amiga/amiga_setup_storage.h src/game/track_lists.h src/game/track_playlist.h src/game/setup_storage.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-track-list-storage: build/verify_track_list_storage
	build/verify_track_list_storage
build/verify_track_lists: tools/verify_track_lists.c src/game/track_lists.h src/game/track_playlist.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-lists: build/verify_track_lists
	build/verify_track_lists
build/verify_track_playlist: tools/verify_track_playlist.c tools/verify_options_menu.c src/game/track_playlist.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-playlist: build/verify_track_playlist
	build/verify_track_playlist
build/verify_track_menu: tools/verify_track_menu.c tools/verify_options_menu.c src/ui/track_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-menu: build/verify_track_menu
	build/verify_track_menu
.PHONY: verify-audio-volume
.PHONY: verify-amiga-audio-volume
build/verify_amiga_audio_volume: tools/verify_amiga_audio_volume.c src/platform/amiga/amiga_audio.c src/platform/amiga/amiga_audio.h src/game/audio_volume.h src/game/audio_channels.h src/game/audio_pitch.h src/game/audio_sample.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-amiga-audio-volume: build/verify_amiga_audio_volume
	build/verify_amiga_audio_volume
build/verify_audio_volume: tools/verify_audio_volume.c tools/verify_configuration.c src/game/audio_volume.h src/game/configuration.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-audio-volume: build/verify_audio_volume
	build/verify_audio_volume
.PHONY: verify-audio-pitch
build/verify_audio_pitch: tools/verify_audio_pitch.c tools/verify_configuration.c src/game/audio_pitch.h src/game/audio_sample.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-audio-pitch: build/verify_audio_pitch
	build/verify_audio_pitch
build/verify_cleared_tracks: tools/verify_cleared_tracks.c src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
build/verify_track_storage: tools/verify_track_storage.c src/platform/amiga/amiga_setup_storage.c src/platform/amiga/amiga_setup_storage.h src/game/setup_storage.h src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-track-storage: build/verify_track_storage
	build/verify_track_storage
build/verify_track_record_write: tools/verify_track_record_write.c tools/verify_options_menu.c src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-track-record-write: build/verify_track_record_write
	build/verify_track_record_write
.PHONY: verify-title-help
build/verify_title_help: tools/verify_title_help.c tools/verify_options_menu.c src/ui/title_help.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-title-help: build/verify_title_help
	build/verify_title_help
build/verify_help_index: tools/verify_help_index.c tools/verify_options_menu.c tools/host_archive.h src/ui/help_index.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-help-index: build/verify_help_index
	build/verify_help_index

.PHONY: verify-help-builder
build/verify_help_builder: tools/verify_help_builder.c tools/verify_options_menu.c tools/host_archive.h src/ui/help_index.h src/ui/help_text.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-help-builder: build/verify_help_builder
	build/verify_help_builder

.PHONY: verify-help-line
build/verify_help_line: tools/verify_help_line.c tools/verify_options_menu.c tools/host_archive.h src/ui/help_line.h src/ui/help_text.h src/ui/help_index.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-help-line: build/verify_help_line
	build/verify_help_line

.PHONY: verify-help-pixels
build/verify_help_pixels: tools/verify_help_pixels.c tools/verify_palette_remap.c tools/host_archive.h $(wildcard src/ui/*.h) | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-help-pixels: build/verify_help_pixels build/font_string_test.bin
	build/verify_help_pixels

.PHONY: verify-help-navigation
build/verify_help_navigation: tools/verify_help_navigation.c tools/verify_options_menu.c src/ui/help_navigation.h src/ui/help_line.h src/ui/help_index.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-help-navigation: build/verify_help_navigation
	build/verify_help_navigation

.PHONY: verify-help-refresh
build/verify_help_refresh: tools/verify_help_refresh.c tools/verify_help_navigation.c tools/verify_options_menu.c $(wildcard src/ui/help_*.h) | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-help-refresh: build/verify_help_refresh
	build/verify_help_refresh

build/verify_resource_archive: tools/verify_resource_archive.c tools/host_archive.h src/platform/amiga/resource_archive.c src/platform/amiga/resource_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-resource-archive: build/verify_resource_archive
	build/verify_resource_archive

build/verify_amiga_joystick: tools/verify_amiga_joystick.c src/platform/amiga/amiga_joystick.h src/game/driver_device.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-amiga-joystick: build/verify_amiga_joystick
	build/verify_amiga_joystick

build/verify_driver_device: tools/verify_driver_device.c tools/verify_options_menu.c src/game/driver_device.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-driver-device: build/verify_driver_device
	build/verify_driver_device

build/verify_amiga_key_scan: tools/verify_amiga_key_scan.c src/platform/amiga/amiga_key_scan.h src/ui/controllers_dialog.h src/game/driver_input.h src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-amiga-key-scan: build/verify_amiga_key_scan
	build/verify_amiga_key_scan

build/verify_indexed_menu_icon: tools/verify_indexed_menu_icon.c tools/verify_menu_icon.c tools/host_archive.h src/ui/menu_icon.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-indexed-menu-icon: build/verify_indexed_menu_icon
	build/verify_indexed_menu_icon

build/verify_menu_restore: tools/verify_menu_restore.c src/ui/menu_background.h src/ui/chunky_ui.h src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-menu-restore: build/verify_menu_restore
	build/verify_menu_restore

.PHONY: verify-palette-remap
.PHONY: verify-track-prepare
build/verify_track_prepare: tools/verify_track_prepare.c tools/verify_palette_remap.c src/ui/track_menu_prepare.h $(wildcard src/ui/*.h) tools/host_archive.h src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-prepare: build/verify_track_prepare build/font_string_test.bin
	build/verify_track_prepare

build/verify_palette_remap: tools/verify_palette_remap.c src/ui/profile_editor_draw.h src/ui/profile_editor_renderer.h src/ui/profile_editor.h src/ui/palette_remap.h src/ui/player_menu_prepare.h src/ui/player_menu_renderer.h src/ui/player_menu_draw.h src/ui/menu_background.h src/ui/menu_icon.h src/ui/chunky_ui.h src/ui/colour_picker.h src/ui/font_resource.h src/ui/menu_bitmap.h tools/host_archive.h src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-palette-remap: build/verify_palette_remap build/font_string_test.bin build/hud_icon_test.bin
	build/verify_palette_remap

build/verify_font_resource: tools/verify_font_resource.c src/ui/font_resource.h tools/host_archive.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-font-resource: build/verify_font_resource
	build/verify_font_resource

.PHONY: verify-wheel-geometry
build/verify_wheel_geometry: tools/verify_wheel_geometry.c src/game/wheel_geometry.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-wheel-geometry: build/verify_wheel_geometry
	build/verify_wheel_geometry

build/verify_wheel_geometry build/verify_vehicle_properties build/verify_dos_hud \
build/verify_surface_effects build/verify_dirty_tracking build/verify_drive_physics \
build/verify_car_collision build/verify_dos_ai build/verify_dos_damage \
build/verify_dos_steering build/verify_dos_points build/verify_actor_layers: src/game/wheel_geometry.h

.PHONY: verify-vehicle-properties
build/verify_vehicle_properties: tools/verify_vehicle_properties.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.c src/game/driver_input.h tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-vehicle-properties: build/verify_vehicle_properties
	build/verify_vehicle_properties

build/verify_profile_setup: tools/verify_profile_setup.c src/game/profile_setup.h src/game/player_profiles.h src/ui/player_menu.h src/ui/player_menu_draw.h src/ui/profile_editor_draw.h src/ui/profile_editor.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-profile-setup: build/verify_profile_setup
	build/verify_profile_setup

.PHONY: verify-list-dialog
build/verify_list_dialog: tools/verify_list_dialog.c src/ui/list_dialog.h src/ui/list_dialog_draw.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-list-dialog: build/verify_list_dialog
	build/verify_list_dialog

.PHONY: verify-list-captions
build/verify_list_captions: tools/verify_list_captions.c src/ui/list_captions.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-list-captions: build/verify_list_captions
	build/verify_list_captions

.PHONY: verify-saved-rectangle
build/verify_saved_rectangle: tools/verify_saved_rectangle.c src/ui/saved_rectangle.h src/ui/chunky_ui.h src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-saved-rectangle: build/verify_saved_rectangle
	build/verify_saved_rectangle

.PHONY: verify-list-renderer
build/make_large_track_lists: tools/make_large_track_lists.c src/game/track_lists.h src/game/track_playlist.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

build/verify_list_renderer: tools/verify_list_renderer.c $(wildcard src/ui/*.h) src/graphics/row_offsets.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

verify-list-renderer: build/verify_list_renderer
	build/verify_list_renderer

.PHONY: verify-list-pixels
.PHONY: verify-track-records-draw
.PHONY: verify-track-records-pixels
.PHONY: verify-track-info
.PHONY: verify-arcade-setup
build/verify_arcade_setup: tools/verify_arcade_setup.c src/game/arcade_setup.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-arcade-setup: build/verify_arcade_setup
	build/verify_arcade_setup

.PHONY: verify-race-completion
.PHONY: verify-post-race-records
.PHONY: verify-championship-standings
.PHONY: verify-standings-draw
.PHONY: verify-palette-fade
.PHONY: verify-result-wait
build/verify_result_wait: tools/verify_result_wait.c tools/verify_post_race_records.c src/ui/result_wait.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-result-wait: build/verify_result_wait
	build/verify_result_wait

build/verify_palette_fade: tools/verify_palette_fade.c tools/verify_post_race_records.c src/ui/palette_fade.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-palette-fade: build/verify_palette_fade
	build/verify_palette_fade

build/verify_standings_draw: tools/verify_standings_draw.c tools/verify_track_records_draw.c src/ui/championship_standings_draw.h src/game/championship_standings.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-standings-draw: build/verify_standings_draw
	build/verify_standings_draw

build/verify_championship_standings: tools/verify_championship_standings.c tools/verify_post_race_records.c src/game/championship_standings.h src/game/player_profiles.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-championship-standings: build/verify_championship_standings
	build/verify_championship_standings

build/verify_target_records: tools/verify_target_records.c src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

build/verify_post_race_records: tools/verify_post_race_records.c src/game/post_race_records.h src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-post-race-records: build/verify_post_race_records
	build/verify_post_race_records

build/verify_race_completion: tools/verify_race_completion.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/arcade_setup.h src/game/finish_rank.h src/game/setup_session.h src/game/race_rewards.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-race-completion: build/verify_race_completion
	build/verify_race_completion

.PHONY: verify-arcade-hud
build/verify_arcade_hud: tools/verify_arcade_hud.c src/ui/arcade_hud.h src/game/arcade_setup.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-arcade-hud: build/verify_arcade_hud
	build/verify_arcade_hud

.PHONY: verify-race-lap-limit
build/verify_race_lap_limit: tools/verify_race_lap_limit.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/race_timing.h src/game/arcade_setup.h src/game/finish_rank.h src/game/track_scene.c src/gen/setup_defaults.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip $< src/game/track_scene.c -o $@

verify-race-lap-limit: build/verify_race_lap_limit
	build/verify_race_lap_limit

.PHONY: verify-track-preview-pixels
.PHONY: verify-track-preview-scene
.PHONY: verify-race-menu
.PHONY: verify-speed-dialog

.PHONY: verify-language-table
.PHONY: verify-race-timing
build/verify_race_timing: tools/verify_race_timing.c src/game/race_timing.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-race-timing: build/verify_race_timing
	build/verify_race_timing
build/verify_language_table: tools/verify_language_table.c src/ui/language_table.h tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-language-table: build/verify_language_table
	build/verify_language_table
build/verify_speed_dialog: tools/verify_speed_dialog.c tools/verify_track_info.c src/ui/speed_dialog.h src/ui/track_records_renderer.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-speed-dialog: build/verify_speed_dialog
	build/verify_speed_dialog

build/verify_race_menu: tools/verify_race_menu.c tools/verify_track_info.c src/ui/race_menu.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-race-menu: build/verify_race_menu
	build/verify_race_menu

.PHONY: verify-track-preview-failure
build/verify_track_preview_failure: tools/verify_track_preview_failure.c tools/verify_track_info.c src/ui/track_info.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-preview-failure: build/verify_track_preview_failure
	build/verify_track_preview_failure

.PHONY: verify-track-preview-shimmer
build/verify_track_preview_shimmer: tools/verify_track_preview_shimmer.c tools/verify_track_info.c src/ui/track_info.h src/game/track_playlist.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-preview-shimmer: build/verify_track_preview_shimmer
	build/verify_track_preview_shimmer

build/verify_track_preview_scene: tools/verify_track_preview_scene.c tools/verify_track_info.c src/game/track_scene.c src/game/track_scene.h src/ui/track_info.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-preview-scene: build/verify_track_preview_scene
	build/verify_track_preview_scene

build/verify_track_preview_pixels: tools/verify_track_preview_pixels.c tools/verify_palette_remap.c $(wildcard src/ui/*.h) | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-preview-pixels: build/verify_track_preview_pixels
	build/verify_track_preview_pixels

build/verify_track_info: tools/verify_track_info.c src/ui/track_info.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-info: build/verify_track_info
	build/verify_track_info

build/verify_track_records_pixels: tools/verify_track_records_pixels.c tools/verify_palette_remap.c $(wildcard src/ui/*.h) src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-records-pixels: build/verify_track_records_pixels build/font_string_test.bin build/hud_icon_test.bin
	build/verify_track_records_pixels

build/verify_track_records_draw: tools/verify_track_records_draw.c src/ui/track_records_draw.h src/ui/track_records_renderer.h src/ui/race_hud.h src/game/track_records.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-track-records-draw: build/verify_track_records_draw
	build/verify_track_records_draw

build/verify_list_pixels: tools/verify_list_pixels.c tools/verify_palette_remap.c $(wildcard src/ui/*.h) src/gen/setup_defaults.h tools/host_archive.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-list-pixels: build/verify_list_pixels build/font_string_test.bin
	build/verify_list_pixels

.PHONY: verify-profile-actions
build/verify_profile_actions: tools/verify_profile_actions.c src/ui/profile_actions.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-profile-actions: build/verify_profile_actions
	build/verify_profile_actions

build/verify_dos_hud: tools/verify_dos_hud.c src/ui/race_hud.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/ui/hud_background.h src/graphics/row_offsets.h src/game/track_records.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/verify_dos_hud: src/game/weapon_state.h
verify-dos-hud: build/verify_dos_hud
	build/verify_dos_hud

.PHONY: verify-hud-background
build/verify_hud_background: tools/verify_hud_background.c src/ui/hud_background.h tools/host_archive.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
build/hud_icon_test.bin: tools/hud_icon_test.s src/graphics/sgfx_chunky_transparent_blit.s src/graphics/sgfx_mult320.s
	@mkdir -p build
	$(VASM) -m68020 -Fbin -quiet -no-opt -o $@ $<
verify-hud-background: build/verify_hud_background build/hud_icon_test.bin
	build/verify_hud_background

.PHONY: verify-font-glyph
.PHONY: verify-font-planar
.PHONY: verify-track-visuals
build/verify_track_visuals: tools/verify_track_visuals.c src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip $< -o $@
verify-track-visuals: build/verify_track_visuals
	build/verify_track_visuals
build/verify_track_mask_fast: tools/verify_track_mask_fast.c src/game/track_scene.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip $< -o $@
.PHONY: verify-track-mask-fast
verify-track-mask-fast: build/verify_track_mask_fast
	build/verify_track_mask_fast
build/font_planar_test.bin: tools/font_planar_test.s src/ui/sui_font_glyph_planar.s
	@mkdir -p build
	$(VASM) -m68020 -Fbin -quiet -no-opt -o $@ $<
verify-font-planar: build/verify_font_glyph build/font_planar_test.bin
	build/verify_font_glyph build/font_planar_test.bin unused unused iso.@f planar
build/sui_font_glyph.bin: tools/font_glyph_test.s src/ui/sui_font_glyph.s src/graphics/sgfx_mult320.s
	@mkdir -p build
	$(VASM) -m68020 -Fbin -quiet -no-opt -o $@ $<
build/sui_font_measure.bin: src/ui/sui_font_measure.s
	@mkdir -p build
	$(VASM) -m68020 -Fbin -quiet -no-opt -o $@ $<
build/font_string_test.bin: tools/font_string_test.s src/ui/sui_font_string.s src/ui/sui_font_measure.s src/ui/sui_font_glyph.s src/ui/sui_menu_bridge.s src/graphics/sgfx_mult320.s
	@mkdir -p build
	$(VASM) -m68020 -Fbin -quiet -no-opt -o $@ $<
build/verify_font_glyph: tools/verify_font_glyph.c tools/host_archive.h src/ui/font_resource.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-font-glyph: build/verify_font_glyph build/sui_font_glyph.bin build/sui_font_measure.bin build/font_string_test.bin
	build/verify_font_glyph build/sui_font_glyph.bin build/sui_font_measure.bin build/font_string_test.bin
	build/verify_font_glyph build/sui_font_glyph.bin build/sui_font_measure.bin build/font_string_test.bin kirj.@f
	build/verify_font_glyph build/sui_font_glyph.bin build/sui_font_measure.bin build/font_string_test.bin iso.@f

build/verify_dos_damage: tools/verify_dos_damage.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h tools/host_archive.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip \
		-I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

verify-dos-damage: build/verify_dos_damage
	build/verify_dos_damage disasm/runtime.bin

.PHONY: verify-animated-boundary
build/verify_animated_boundary: tools/verify_animated_boundary.c tools/verify_dos_damage.c \
		src/game/animated_boundary.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip \
		-I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-animated-boundary: build/verify_animated_boundary
	build/verify_animated_boundary

.PHONY: verify-drive-trajectory
build/verify_drive_trajectory: tools/verify_drive_trajectory.c tools/verify_dos_damage.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.c tools/host_archive.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip \
		-I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@
verify-drive-trajectory: build/verify_drive_trajectory
	@for scenario in 0 1 2 3 4 5 6 7 8; do build/verify_drive_trajectory $$scenario || exit $$?; done
	build/verify_drive_trajectory 4 ref/TRACKS/BRIDGES.SS
	build/verify_drive_trajectory 4 ref/TRACKS/BUMPS.SS

build/verify_surface_effects: tools/verify_surface_effects.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip $< src/game/track_scene.c -o $@

verify-surface-effects: build/verify_surface_effects
	build/verify_surface_effects

.PHONY: verify-dirty-tracking verify-planar-writes
build/verify_dirty_tracking: tools/verify_dirty_tracking.c \
		src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip $< src/game/track_scene.c -o $@

build/verify_car_collision build/verify_drive_physics build/verify_dos_steering \
build/verify_dos_damage build/verify_surface_effects build/verify_dirty_tracking: src/game/track_scene.c

build/verify_car_collision build/verify_drive_physics build/verify_dos_steering \
build/verify_dos_damage build/verify_surface_effects build/verify_dirty_tracking: \
		src/ui/race_hud.h src/ui/arcade_hud.h src/game/arcade_setup.h src/game/finish_rank.h src/ui/hud_background.h src/ui/font_resource.h src/game/track_records.h src/graphics/row_offsets.h

verify-dirty-tracking: build/verify_dirty_tracking
	build/verify_dirty_tracking

verify-planar-writes:
	bash tools/verify_planar_writes.sh

.PHONY: verify-dos-particle-expiry
.PHONY: verify-dos-points
build/verify_dos_points: tools/verify_dos_points.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_scene.c src/game/track_scene.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip -I/opt/homebrew/opt/unicorn/include $< \
		src/game/track_scene.c -L/opt/homebrew/opt/unicorn/lib -lunicorn -o $@

verify-dos-points: build/verify_dos_points
	build/verify_dos_points disasm/runtime.bin

build/verify_dos_particle_expiry: tools/verify_dos_particle_expiry.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I/opt/homebrew/opt/unicorn/include $< \
		-L/opt/homebrew/opt/unicorn/lib -lunicorn -o $@

verify-dos-particle-expiry: build/verify_dos_particle_expiry
	build/verify_dos_particle_expiry disasm/runtime.bin

build/verify_actor_layers: tools/verify_actor_layers.c src/game/race_runtime.c src/game/signed_division.h src/game/track_scene.c \
        src/game/race_runtime.h src/game/track_scene.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections \
		-Wl,-dead_strip $< src/game/track_scene.c -o $@

build/verify_actor_mask: tools/verify_actor_mask.c tools/host_archive.h src/game/track_scene.c \
        src/game/track_scene.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		tools/verify_actor_mask.c src/game/track_scene.c -o $@

build/scan_track_materials: tools/scan_track_materials.c tools/host_archive.h \
		src/game/track_scene.c src/game/track_scene.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		tools/scan_track_materials.c src/game/track_scene.c -o $@

verify-native-tracks: build/scan_track_materials
	SLICKS_MASK_ARCHIVE=ref/SLICKS.000 build/scan_track_materials ref/SLICKS.DAT ref/TRACKS/*.SS
	SLICKS_MASK_ARCHIVE=ref/SLICKS.000 SLICKS_SERVICE=1 build/scan_track_materials ref/SLICKS.DAT ref/TRACKS/*.SS

build/scan_track_materials build/verify_actor_mask build/verify_actor_layers \
build/verify_dos_ai build/verify_dos_points: src/graphics/row_offsets.h

build/verify_dos_hud: src/game/track_scene.c

build/sgfx_plot.bin: src/graphics/sgfx_plot.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_checker_fill.bin: tools/sgfx_checker_fill_test.s \
		src/graphics/sgfx_checker_fill.s src/graphics/sgfx_plot_plane.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sgfx_title_pages.bin: tools/sgfx_title_pages_test.s \
		src/graphics/sgfx_title_pages.s src/graphics/sgfx_planar_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sgfx_title_crop.bin: tools/sgfx_title_crop_test.s \
		src/graphics/sgfx_title_crop.s src/graphics/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sutil_palette_nearest.bin: src/util/sutil_palette_nearest.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sui_title_step.bin: tools/sui_title_step_test.s \
		src/ui/sui_title_step.s src/util/sutil_palette_nearest.s \
		src/graphics/sgfx_title_crop.s src/graphics/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sui_color_slot.bin: src/ui/sui_color_slot.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sui_title_tail.bin: tools/sui_title_tail_test.s \
		src/ui/sui_title_tail.s src/util/sutil_palette_nearest.s \
		src/ui/sui_color_slot.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sui_title_dispatch.bin: src/ui/sui_title_dispatch.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_title_dispatch: tools/verify_title_dispatch.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

.PHONY: verify-title-bridge
build/title_bridge.bin: tools/title_bridge_test.s src/platform/amiga/native_bridge.s src/ui/sui_title_dispatch.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/verify_title_bridge: tools/verify_title_bridge.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include \
		-L$(UNICORN_PREFIX)/lib $< -lunicorn -o $@

verify-title-bridge: build/title_bridge.bin build/verify_title_bridge
	build/verify_title_bridge build/title_bridge.bin

verify-native-graphics: verify-title-bridge

build/sgame_post_title_init.bin: tools/sgame_post_title_init_test.s \
		src/game/sgame_post_title_init.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/verify_post_title_init: tools/verify_post_title_init.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

build/sgfx_span_fill.bin: src/graphics/sgfx_span_fill.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_remap_copy.bin: src/graphics/sgfx_remap_copy.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_clear_full.bin: tools/sgfx_clear_full_test.s \
		src/graphics/sgfx_clear_full.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/verify_clear_full: tools/verify_clear_full.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

build/sgfx_mode_setup.bin: tools/sgfx_mode_setup_test.s \
		src/graphics/sgfx_clear_full.s src/graphics/sgfx_mode_setup.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/verify_mode_setup: tools/verify_mode_setup.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

build/sutil_fill_bytes.bin: tools/sutil_fill_bytes_test.s \
		src/util/sutil_fill_bytes.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/verify_fill_bytes: tools/verify_fill_bytes.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

build/sui_bevel.bin: tools/sui_bevel_test.s src/ui/sui_bevel.s \
		src/util/sutil_palette_nearest.s src/graphics/sgfx_span_fill.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sgfx_read_pixel.bin: src/graphics/sgfx_read_pixel.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_planar_blit.bin: src/graphics/sgfx_planar_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_transparent_blit.bin: src/graphics/sgfx_transparent_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_readback.bin: src/graphics/sgfx_readback.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_planar_subrect_blit.bin: src/graphics/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_planar_subrect_blit_far.bin: src/graphics/sgfx_planar_subrect_blit_far.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_subrect_far: tools/verify_subrect_far.c tools/verify_native_graphics.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib $< -lunicorn -o $@

.PHONY: verify-subrect-far
build/sprite_opaque.bin: tools/sprite_opaque_test.s src/game/sprite_opaque.s src/game/track_sprite_fast.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_sprite_opacity: tools/verify_sprite_opacity.c src/game/sprite_opacity.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-sprite-opacity
verify-sprite-opacity: build/sprite_opaque.bin build/verify_sprite_opacity
	build/verify_sprite_opacity build/sprite_opaque.bin

build/particle_draw.bin: tools/particle_draw_test.s src/game/particle_draw.s | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -o $@ $<

build/particle_compact_draw.bin: tools/particle_compact_draw_test.s src/game/particle_draw.s | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -I. -o $@ $<

.PHONY: verify-particle-compact-draw
verify-particle-compact-draw: build/particle_compact_draw.bin build/verify_particle_draw
	build/verify_particle_draw build/particle_compact_draw.bin compact

build/verify_particle_draw: tools/verify_particle_draw.c tools/verify_surface_effects.c src/game/race_runtime.c src/game/signed_division.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-particle-draw
build/particle_advance.bin: tools/particle_advance_test.s src/game/particle_runtime.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_particle_advance: tools/verify_particle_advance.c tools/verify_dos_particle_expiry.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-particle-advance
verify-particle-advance: unpack build/particle_advance.bin build/verify_particle_advance
	build/verify_particle_advance disasm/runtime.bin build/particle_advance.bin

# Isolated representation experiment; intentionally not linked by amiga/Makefile.
build/particle_compact_trial.bin: tools/particle_compact_trial_test.s src/game/particle_compact_trial.s | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -I. -o $@ $<

build/particle_compact_legacy.bin: tools/particle_advance_test.s src/game/particle_runtime.s | build
	$(VASM) -quiet -m68020 -DSLICKS_PARTICLE_WORD_COORDINATES=1 -Fbin -o $@ $<

.PHONY: verify-particle-compact-trial
verify-particle-compact-trial: unpack build/particle_advance.bin build/particle_compact_trial.bin build/particle_compact_legacy.bin build/verify_particle_advance
	build/verify_particle_advance disasm/runtime.bin build/particle_advance.bin build/particle_compact_trial.bin build/particle_compact_legacy.bin

# Target structure offsets from the 68020 compiler for native-routine tests.
build/offsets/race_offsets.i: src/game/race_offsets.c src/game/race_runtime.h | build
	mkdir -p build/offsets
	$(M68K_CC) -m68020 -O2 -S -o build/offsets/race_offsets.s $<
	sed -n 's/^@@//p' build/offsets/race_offsets.s > $@

build/dirty_rect.bin: src/game/dirty_rect.s build/offsets/race_offsets.i | build
	printf '\tinclude "src/game/dirty_rect.s"\n' > build/offsets/dirty_rect_test.s
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ build/offsets/dirty_rect_test.s

build/verify_dirty_rect: tools/verify_dirty_rect.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-dirty-rect
verify-dirty-rect: build/dirty_rect.bin build/verify_dirty_rect build/offsets/race_offsets.i
	build/verify_dirty_rect build/dirty_rect.bin build/offsets/race_offsets.i

build/dirty_prune.bin: src/game/dirty_prune.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -o $@ $<

build/verify_dirty_prune: tools/verify_dirty_prune.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-dirty-prune
verify-dirty-prune: build/dirty_prune.bin build/verify_dirty_prune build/offsets/race_offsets.i
	build/verify_dirty_prune build/dirty_prune.bin build/offsets/race_offsets.i

build/actor_advance.bin: tools/actor_advance_test.s src/game/track_motion.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_actor_advance: tools/verify_actor_advance.c src/game/race_runtime.c src/game/signed_division.h src/game/weapon_actors.inc src/game/actor_slots.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-actor-advance
verify-actor-advance: build/actor_advance.bin build/verify_actor_advance build/offsets/race_offsets.i
	build/verify_actor_advance build/actor_advance.bin build/offsets/race_offsets.i

build/emission_scan.bin: tools/emission_scan_test.s src/game/car_emission.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/emission_add.bin: tools/emission_add_test.s src/game/car_emission.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/emission_compact_add.bin: tools/emission_add_test.s src/game/car_emission.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -DSLICKS_PARTICLE_WORD_COORDINATES=1 -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_emission_add: tools/verify_emission_add.c src/game/actor_slots.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-emission-add
verify-emission-add: build/emission_add.bin build/emission_compact_add.bin build/actor_allocate.bin build/verify_emission_add
	build/verify_emission_add build/emission_add.bin build/offsets/race_offsets.i build/actor_allocate.bin 24
	build/verify_emission_add build/emission_compact_add.bin build/offsets/race_offsets.i build/actor_allocate.bin 20

build/verify_emission_scan: tools/verify_emission_scan.c src/game/actor_slots.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-emission-scan
verify-emission-scan: build/emission_scan.bin build/verify_emission_scan
	build/verify_emission_scan build/emission_scan.bin

build/car_draw.bin: tools/car_draw_test.s src/game/car_draw.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/car_probe.bin: tools/car_probe_test.s src/game/car_motion.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_car_probe: tools/verify_car_probe.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-car-probe
verify-car-probe: build/car_probe.bin build/verify_car_probe
	build/verify_car_probe build/car_probe.bin

build/verify_car_draw: tools/verify_car_draw.c tools/verify_surface_effects.c src/game/race_runtime.c src/game/signed_division.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-car-draw
build/verify_car_render_cache: tools/verify_car_render_cache.c tools/verify_surface_effects.c tools/native_sprite_oracle.h src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-car-render-cache
verify-car-render-cache: build/car_draw.bin build/sprite_opaque.bin build/verify_car_render_cache
	build/verify_car_render_cache

build/verify_sprite_restore_chain: tools/verify_sprite_restore_chain.c tools/verify_surface_effects.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/weapon_actors.inc | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-sprite-restore-chain
verify-sprite-restore-chain: build/sprite_opaque.bin build/verify_sprite_restore_chain
	build/verify_sprite_restore_chain

build/verify_sprite_draw_chain: tools/verify_sprite_draw_chain.c tools/verify_sprite_restore_chain.c tools/verify_surface_effects.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/weapon_actors.inc | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-sprite-draw-chain
verify-sprite-draw-chain: build/sprite_opaque.bin build/verify_sprite_draw_chain
	build/verify_sprite_draw_chain

build/verify_copper_palette: tools/verify_copper_palette.c src/platform/amiga/copper_palette.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
.PHONY: verify-copper-palette
verify-copper-palette: build/verify_copper_palette
	build/verify_copper_palette

verify-car-draw: build/car_draw.bin build/verify_car_draw
	build/verify_car_draw build/car_draw.bin

verify-particle-draw: build/particle_draw.bin build/verify_particle_draw
	build/verify_particle_draw build/particle_draw.bin

verify-subrect-far: unpack build/sgfx_planar_subrect_blit_far.bin build/verify_subrect_far
	build/verify_subrect_far disasm/runtime.bin build/sgfx_planar_subrect_blit_far.bin

build/verify_screen_capture: tools/verify_screen_capture.c tools/verify_native_graphics.c src/ui/screen_capture.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib $< -lunicorn -o $@

.PHONY: verify-screen-capture
verify-screen-capture: unpack build/verify_screen_capture
	build/verify_screen_capture disasm/runtime.bin

build/verify_capture_storage: tools/verify_capture_storage.c tools/verify_track_storage.c src/platform/amiga/amiga_setup_storage.c src/ui/screen_capture.h
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

.PHONY: verify-capture-storage
verify-capture-storage: build/verify_capture_storage
	build/verify_capture_storage

build/verify_native_graphics: tools/verify_native_graphics.c
	@mkdir -p build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror \
		-I$(UNICORN_PREFIX)/include -L$(UNICORN_PREFIX)/lib \
		$< -lunicorn -o $@

verify-native-graphics: unpack build/sgfx_plot_plane.bin build/sgfx_plot.bin \
		build/sgfx_read_pixel.bin build/sgfx_planar_blit.bin \
		build/sgfx_transparent_blit.bin build/sgfx_readback.bin \
		build/sgfx_planar_subrect_blit.bin build/sgfx_checker_fill.bin \
		build/sgfx_title_pages.bin build/sgfx_title_crop.bin \
		build/sutil_palette_nearest.bin \
		build/sui_title_step.bin \
		build/sui_color_slot.bin \
		build/sui_title_tail.bin \
		build/sui_title_dispatch.bin build/verify_title_dispatch \
		build/sgame_post_title_init.bin build/verify_post_title_init \
		build/sgfx_span_fill.bin build/sgfx_remap_copy.bin \
		build/sgfx_clear_full.bin build/verify_clear_full \
		build/sgfx_mode_setup.bin build/verify_mode_setup \
		build/sutil_fill_bytes.bin build/verify_fill_bytes \
		build/sui_bevel.bin \
		build/verify_native_graphics
	build/verify_native_graphics disasm/runtime.bin \
		build/sgfx_plot_plane.bin build/sgfx_read_pixel.bin \
		build/sgfx_planar_blit.bin build/sgfx_transparent_blit.bin \
		build/sgfx_readback.bin build/sgfx_planar_subrect_blit.bin \
		build/sgfx_plot.bin build/sgfx_checker_fill.bin \
		build/sgfx_title_pages.bin build/sgfx_title_crop.bin \
		build/sutil_palette_nearest.bin build/sui_title_step.bin \
		build/sui_color_slot.bin build/sui_title_tail.bin \
		build/sgfx_span_fill.bin build/sgfx_remap_copy.bin \
		build/sui_bevel.bin
	build/verify_title_dispatch build/sui_title_dispatch.bin
	build/verify_post_title_init disasm/runtime.bin \
		build/sgame_post_title_init.bin
	build/verify_clear_full disasm/runtime.bin build/sgfx_clear_full.bin
	build/verify_mode_setup disasm/runtime.bin build/sgfx_clear_full.bin \
		build/sgfx_mode_setup.bin
	build/verify_fill_bytes disasm/runtime.bin build/sutil_fill_bytes.bin

trace-summary:
	$(PYTHON) tools/summarize_dosbox_x.py $(REFERENCE_ROOT)/dosbox-x.log

amiga:
	cd amiga && . ./env.sh && $(MAKE)

amiga-run: amiga
	cd amiga && . ./env.sh && ./run.sh

amiga-debug: amiga
	cd amiga && . ./env.sh && ./debug.sh

RELEASE_ARCHIVE ?= build/release/slicks-$(shell git rev-parse --short HEAD).zip
.PHONY: release-package
release-package: amiga
	$(PYTHON) tools/package_release.py $(RELEASE_ARCHIVE)

amiga-check: amiga
	cd amiga && . ./env.sh && ./diag_run.sh

amiga-race-check: amiga
	cd amiga && . ./env.sh && ./diag_race.sh

amiga-track-check: amiga
	cd amiga && . ./env.sh && ./diag_track.sh

amiga-ice-check: amiga
	cd amiga && . ./env.sh && ./diag_ice.sh

amiga-zone-check: amiga
	cd amiga && . ./env.sh && ./diag_zone.sh

amiga-lap-check: amiga
	cd amiga && . ./env.sh && ./diag_lap.sh

amiga-results-check: amiga
	cd amiga && . ./env.sh && ./diag_results.sh

amiga-restore-check: amiga
	cd amiga && . ./env.sh && ./diag_restore.sh

ghidra: unpack
	@mkdir -p tools/ghidra-proj
	$(GHIDRA)/support/analyzeHeadless tools/ghidra-proj Slicks \
		-import disasm/runtime.bin \
		-processor "x86:LE:16:Real Mode" \
		-loader BinaryLoader -loader-baseAddr 0x10100 \
		-scriptPath ghidra_scripts \
		-preScript MarkEntries.java \
		-postScript ExportListing.java $(ABS_ROOT)/disasm/listing.txt

ghidra-normalized: rebuild-mz
	@mkdir -p tools/ghidra-normalized-proj
	$(GHIDRA)/support/analyzeHeadless tools/ghidra-normalized-proj SlicksNormalized \
		-import disasm/runtime.exe \
		-scriptPath ghidra_scripts \
		-postScript ExportListing.java \
		$(ABS_ROOT)/disasm/normalized-listing.txt

ghidra-live: analyze-execution-trace analyze-vga-sites
	@mkdir -p tools/ghidra-live-proj
	$(GHIDRA)/support/analyzeHeadless tools/ghidra-live-proj SlicksLive \
		-import disasm/runtime.bin -overwrite \
		-processor "x86:LE:16:Real Mode" \
		-loader BinaryLoader -loader-baseAddr 0x10100 \
		-scriptPath ghidra_scripts \
		-preScript MarkEntries.java \
		-preScript SeedLiveMap.java $(ABS_ROOT)/$(LIVE_ENTRYPOINTS) \
			0x10100 $(ABS_ROOT)/ghidra_scripts/vga-symbols.csv \
		-postScript ExportListing.java $(ABS_ROOT)/disasm/live-listing.txt

ghidra-live-normalized: analyze-execution-trace analyze-vga-sites rebuild-mz
	@mkdir -p tools/ghidra-live-normalized-proj
	$(GHIDRA)/support/analyzeHeadless tools/ghidra-live-normalized-proj \
		SlicksLiveNormalized -import disasm/runtime.exe -overwrite \
		-scriptPath ghidra_scripts \
		-preScript SeedLiveMap.java $(ABS_ROOT)/$(LIVE_ENTRYPOINTS) \
			0x10000 $(ABS_ROOT)/ghidra_scripts/vga-symbols.csv \
		-postScript ExportListing.java \
			$(ABS_ROOT)/disasm/live-normalized-listing.txt

todo:
	@sed -n '/^## Immediate next step/,$$p' PROJECT.md
	@printf '\nTracked work markers:\n'
	@! git grep -nE 'TODO|FIXME|HACK' -- ':!PROJECT.md' ':!docs/open-work.md' || true

build/memory_test.bin: tools/memory_test.s src/platform/amiga/memory.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_memory: tools/verify_memory.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-memory
verify-memory: build/memory_test.bin build/verify_memory
	build/verify_memory build/memory_test.bin

build/hud_restore_test.bin: src/game/hud_restore.s | build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/verify_hud_restore: tools/verify_hud_restore.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-hud-restore
verify-hud-restore: build/hud_restore_test.bin build/verify_hud_restore
	build/verify_hud_restore build/hud_restore_test.bin

build/c2p16_test.bin: src/platform/amiga/c2p16_interleaved.s | build
	$(VASM) -quiet -m68020 -Fbin -I$(HOME)/.local/opt/m68k-amiga-elf/sys-include -o $@ $<

build/verify_c2p16: tools/verify_c2p16.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-c2p16
verify-c2p16: build/c2p16_test.bin build/verify_c2p16
	build/verify_c2p16 build/c2p16_test.bin

build/c2p8_test.bin: src/platform/amiga/c2p8_interleaved.s | build
	$(VASM) -quiet -m68020 -Fbin -I$(HOME)/.local/opt/m68k-amiga-elf/sys-include -o $@ $<

.PHONY: verify-c2p8
verify-c2p8: build/c2p8_test.bin build/verify_c2p16
	build/verify_c2p16 build/c2p8_test.bin 8

build/actor_order.bin: src/game/actor_order.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -o $@ $<

build/signed_div100.bin: tools/signed_div100_test.c tools/signed_div100_test.ld src/game/signed_division.h | build
	$(M68K_CC) -m68020 -msoft-float -O3 -ffunction-sections -fomit-frame-pointer -nostdlib -Wl,--oformat=binary,-T,tools/signed_div100_test.ld $< -o $@

build/verify_signed_div100: tools/verify_signed_div100.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-signed-div100
verify-signed-div100: build/signed_div100.bin build/verify_signed_div100
	build/verify_signed_div100 build/signed_div100.bin

build/velocity_division.bin: tools/velocity_division_test.s src/game/velocity_division.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -I. -o $@ $<

build/verify_velocity_division: tools/verify_velocity_division.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-velocity-division
verify-velocity-division: build/velocity_division.bin build/verify_velocity_division
	build/verify_velocity_division build/velocity_division.bin

build/actor_compact_order.bin: tools/actor_compact_order_test.s src/game/actor_order.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

.PHONY: verify-actor-compact-order
verify-actor-compact-order: build/actor_compact_order.bin build/verify_actor_order
	build/verify_actor_order build/actor_compact_order.bin build/offsets/race_offsets.i compact

build/verify_actor_order: tools/verify_actor_order.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/weapon_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-actor-order
verify-actor-order: build/actor_order.bin build/verify_actor_order
	build/verify_actor_order build/actor_order.bin build/offsets/race_offsets.i

build/car_integration.bin: tools/car_integration_test.s src/game/car_motion.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_car_integration: tools/verify_car_integration.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/track_material_sample.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip -I$(UNICORN_PREFIX)/include $< src/game/track_scene.c -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-car-integration
verify-car-integration: build/car_integration.bin build/verify_car_integration
	build/verify_car_integration build/car_integration.bin build/offsets/race_offsets.i

build/point_restore.bin: tools/point_restore_test.s src/game/point_restore.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/point_compact_restore.bin: tools/point_compact_restore_test.s src/game/point_restore.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

.PHONY: verify-point-compact-restore
verify-point-compact-restore: build/point_compact_restore.bin build/verify_point_restore
	build/verify_point_restore build/point_compact_restore.bin build/offsets/race_offsets.i compact

build/verify_point_restore: tools/verify_point_restore.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-point-restore
verify-point-restore: build/point_restore.bin build/verify_point_restore
	build/verify_point_restore build/point_restore.bin build/offsets/race_offsets.i

build/pc_sampler_test.bin: tools/pc_sampler_test.s src/platform/amiga/pc_sampler.s | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -I. -o $@ $<

build/verify_pc_sampler: tools/verify_pc_sampler.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-pc-sampler
verify-pc-sampler: build/pc_sampler_test.bin build/verify_pc_sampler
	build/verify_pc_sampler build/pc_sampler_test.bin

build/verify_retention_groups: tools/verify_retention_groups.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/game/sprite_retention.inc src/game/weapon_actors.inc | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip $< src/game/track_scene.c -o $@

.PHONY: verify-retention-groups
.PHONY: verify-retention-snapshot
build/verify_retention_snapshot: tools/verify_retention_snapshot.c src/platform/amiga/retention_snapshot.h src/game/race_runtime.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
verify-retention-snapshot: build/verify_retention_snapshot
	build/verify_retention_snapshot
build/retention_address.bin: tools/retention_address_test.s src/game/sprite_retention.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_retention_address: tools/verify_retention_address.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

build/retention_particle.bin: tools/retention_particle_test.s src/game/sprite_retention.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -Fbin -Ibuild/offsets -I. -o $@ $<

build/retention_compact_particle.bin: tools/retention_particle_test.s src/game/sprite_retention.s build/offsets/race_offsets.i | build
	$(VASM) -quiet -m68020 -no-opt -DSLICKS_PARTICLE_WORD_COORDINATES=1 -Fbin -Ibuild/offsets -I. -o $@ $<

build/verify_retention_particle: tools/verify_retention_particle.c | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -I$(UNICORN_PREFIX)/include $< -L$(UNICORN_PREFIX)/lib -lunicorn -o $@

.PHONY: verify-retention-particle
verify-retention-particle: build/retention_particle.bin build/retention_compact_particle.bin build/verify_retention_particle
	build/verify_retention_particle build/retention_particle.bin 24
	build/verify_retention_particle build/retention_compact_particle.bin 20

.PHONY: verify-retention-address
verify-retention-address: build/retention_address.bin build/verify_retention_address
	build/verify_retention_address build/retention_address.bin build/offsets/race_offsets.i

verify-retention-groups: build/verify_retention_groups
	build/verify_retention_groups

build/verify_status_cache: tools/verify_status_cache.c src/game/race_runtime.c src/game/signed_division.h src/game/race_runtime.h src/ui/race_hud.h | build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -Wl,-dead_strip $< src/game/track_scene.c -o $@

.PHONY: verify-status-cache
verify-status-cache: build/verify_status_cache
	build/verify_status_cache

clean:
	rm -rf build
