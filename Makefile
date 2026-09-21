PYTHON ?= python3
SOURCE ?= ref/SLICKS.EXE
REFERENCE_ROOT ?= tmp/pc-root
REFERENCE_CAPTURE ?= $(REFERENCE_ROOT)/slicks-handoff
REFERENCE_FIXED_ROOT ?= tmp/pc-fixed
REFERENCE_RACE_VIDEO ?=
REFERENCE_TRACE_BITMAP ?= $(REFERENCE_FIXED_ROOT)/slicks-executed.bin
REFERENCE_TRACE_EDGES ?= $(REFERENCE_FIXED_ROOT)/slicks-edges.csv
CC ?= cc
UNICORN_PREFIX ?= /opt/homebrew/opt/unicorn
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
	unpack rebuild-mz verify-runtime trace-summary \
	ghidra ghidra-normalized \
	todo clean

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

trace-summary:
	$(PYTHON) tools/summarize_dosbox_x.py $(REFERENCE_ROOT)/dosbox-x.log

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

todo:
	@sed -n '/^## Immediate next step/,$$p' PROJECT.md
	@printf '\nTracked work markers:\n'
	@! git grep -nE 'TODO|FIXME|HACK' -- ':!PROJECT.md' ':!docs/open-work.md' || true

clean:
	rm -rf build
