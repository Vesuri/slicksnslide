PYTHON ?= python3
SOURCE ?= ref/SLICKS.EXE
REFERENCE_ROOT ?= tmp/pc-root
REFERENCE_CAPTURE ?= $(REFERENCE_ROOT)/slicks-handoff
REFERENCE_FIXED_ROOT ?= tmp/pc-fixed
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
	verify-memory-trace verify-primitive-trace analyze-vga-sites unpack rebuild-mz \
	verify-runtime verify-native-graphics trace-summary \
	ghidra ghidra-normalized ghidra-live ghidra-live-normalized \
	amiga amiga-run amiga-debug amiga-check todo clean

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
	cp ref/TRACKS/BASIC.SS $(REFERENCE_FIXED_ROOT)/TRACKS/

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
		-c "autotype -w 15 enter" \
		-c "dx-capture /v /-a /-d slicks.exe"

reference-trace: prepare-fixed-reference
	cd $(REFERENCE_FIXED_ROOT) && \
		env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
		$(abspath $(DOSBOX_X_TRACE)) \
		-conf $(ABS_ROOT)/reference/dosbox-x-286.conf \
		-set "cpu cycles=12000" -set "log logfile=basic-trace.log" \
		-nogui -nomenu -silent -fastlaunch -time-limit 36 \
		-c "mount c $(abspath $(REFERENCE_FIXED_ROOT))" -c "c:" \
		-c "autotype -w 15 enter" \
		-c "dx-capture /v /-a /-d slicks.exe"

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

build/sgfx_plot_plane.bin: native/sgfx_plot_plane.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_plot.bin: native/sgfx_plot.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_checker_fill.bin: tools/sgfx_checker_fill_test.s \
		native/sgfx_checker_fill.s native/sgfx_plot_plane.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sgfx_title_pages.bin: tools/sgfx_title_pages_test.s \
		native/sgfx_title_pages.s native/sgfx_planar_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sgfx_title_crop.bin: tools/sgfx_title_crop_test.s \
		native/sgfx_title_crop.s native/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sutil_palette_nearest.bin: native/sutil_palette_nearest.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sui_title_step.bin: tools/sui_title_step_test.s \
		native/sui_title_step.s native/sutil_palette_nearest.s \
		native/sgfx_title_crop.s native/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -I. -o $@ $<

build/sui_color_slot.bin: native/sui_color_slot.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_span_fill.bin: native/sgfx_span_fill.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_read_pixel.bin: native/sgfx_read_pixel.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_planar_blit.bin: native/sgfx_planar_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_transparent_blit.bin: native/sgfx_transparent_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_readback.bin: native/sgfx_readback.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

build/sgfx_planar_subrect_blit.bin: native/sgfx_planar_subrect_blit.s
	@mkdir -p build
	$(VASM) -quiet -m68020 -Fbin -o $@ $<

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
		build/sgfx_span_fill.bin \
		build/verify_native_graphics
	build/verify_native_graphics disasm/runtime.bin \
		build/sgfx_plot_plane.bin build/sgfx_read_pixel.bin \
		build/sgfx_planar_blit.bin build/sgfx_transparent_blit.bin \
		build/sgfx_readback.bin build/sgfx_planar_subrect_blit.bin \
		build/sgfx_plot.bin build/sgfx_checker_fill.bin \
		build/sgfx_title_pages.bin build/sgfx_title_crop.bin \
		build/sutil_palette_nearest.bin build/sui_title_step.bin \
		build/sui_color_slot.bin build/sgfx_span_fill.bin

trace-summary:
	$(PYTHON) tools/summarize_dosbox_x.py $(REFERENCE_ROOT)/dosbox-x.log

amiga:
	cd amiga && . ./env.sh && $(MAKE)

amiga-run: amiga
	cd amiga && . ./env.sh && ./run.sh

amiga-debug: amiga
	cd amiga && . ./env.sh && ./debug.sh

amiga-check: amiga
	cd amiga && . ./env.sh && ./diag_run.sh

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

clean:
	rm -rf build
