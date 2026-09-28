# Native host cross-build: no Amiga assembler or build utilities required.
WHDLOAD ?= $(HOME)/.local/share/amiga/WHDLoad
NDK ?= $(HOME)/.local/opt/m68k-amiga-elf/sys-include
VASM ?= $(HOME)/.local/vasmm68k_mot
OUT := ../build/whdload

.PHONY: all test smoke boot-test load-test race-test exit-test FORCE
all: $(OUT)/Slicks.slave
test: $(OUT)/SlicksTest.slave
smoke: $(OUT)/Smoke.slave
boot-test: $(OUT)/BootTest.slave
load-test: $(OUT)/LoadTest.slave
race-test: $(OUT)/RaceTest.slave
exit-test: $(OUT)/ExitTest.slave
$(OUT)/RaceTest.slave: VASMFLAGS += -DRACETEST=1
$(OUT)/ExitTest.slave: VASMFLAGS += -DEXITTEST=1
$(OUT)/BootTest.slave: VASMFLAGS += -DBOOTONLY=1
$(OUT)/LoadTest.slave: VASMFLAGS += -DLOADONLY=1

$(OUT)/Smoke.slave: SmokeSlave.s FORCE
	@mkdir -p $(OUT)
	$(VASM) -m68000 -pic -x -devpac -Fhunkexe -nosym -I"$(WHDLOAD)/Include" -I"$(NDK)" -o $@ $<

$(OUT)/SlicksTest.slave: VASMFLAGS += -DTEST=1

# Reassemble the small slave on every invocation: SDK includes and assembler
# options must never leave an old diagnostic slave in a release package.
$(OUT)/Slicks.slave $(OUT)/SlicksTest.slave $(OUT)/BootTest.slave $(OUT)/LoadTest.slave $(OUT)/RaceTest.slave $(OUT)/ExitTest.slave: SlicksSlave.s FORCE
	@mkdir -p $(OUT)/include/lvo
	@for lib in dos exec graphics expansion; do ln -sf "$(NDK)/lvo/$${lib}_lib.i" "$(OUT)/include/lvo/$$lib.i"; done
	$(VASM) -m68020 -pic -x -devpac -Fhunkexe -nosym $(VASMFLAGS) -I$(OUT)/include -I"$(WHDLOAD)/Include" -I"$(WHDLOAD)/Src/sources" -I"$(NDK)" -o $@ $<
