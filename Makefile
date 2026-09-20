PYTHON ?= python3
SOURCE ?= SLICKS.EXE
CC ?= cc
UNICORN_PREFIX ?= /opt/homebrew/opt/unicorn

GHIDRA ?= tools/ghidra/ghidra_12.1_PUBLIC
ABS_ROOT := $(abspath .)

.PHONY: inspect hash unpack rebuild-mz ghidra todo clean

inspect:
	$(PYTHON) tools/mz_info.py $(SOURCE)

hash:
	shasum -a 256 $(SOURCE)

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

ghidra: unpack
	@mkdir -p tools/ghidra-proj
	$(GHIDRA)/support/analyzeHeadless tools/ghidra-proj Slicks \
		-import disasm/runtime.bin \
		-processor "x86:LE:16:Real Mode" \
		-loader BinaryLoader -loader-baseAddr 0x10100 \
		-scriptPath ghidra_scripts \
		-preScript MarkEntries.java \
		-postScript ExportListing.java $(ABS_ROOT)/disasm/listing.txt

todo:
	@sed -n '/^## Immediate next step/,$$p' PROJECT.md
	@printf '\nTracked work markers:\n'
	@! git grep -nE 'TODO|FIXME|HACK' -- ':!PROJECT.md' ':!docs/open-work.md' || true

clean:
	rm -rf build
