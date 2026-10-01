#!/usr/bin/env bash
# Test real assembly writes independently of beam timing and final checksums.
set -euo pipefail
cd "$(dirname "$0")/.."
. amiga/env.sh
unicorn_prefix="${UNICORN_PREFIX:-/opt/homebrew/opt/unicorn}"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/slicks-planar.XXXXXX")"
sdk_dir="$(cd "$(dirname "$(command -v m68k-amiga-elf-gcc)")/../m68k-amiga-elf/sys-include" && pwd)"
for name in vga_to_chunky c2p1x1_8_c5_bm c2p16_interleaved; do
  vasmm68k_mot -m68020 -Felf -quiet -x -no-opt -nowarn=62 \
    -I"$sdk_dir" -o "$test_dir/$name.o" "src/platform/amiga/$name.s"
done
vasmm68k_mot -m68020 -Felf -quiet -x -no-opt \
  -o "$test_dir/particle_runtime.o" src/game/particle_runtime.s
vasmm68k_mot -m68020 -Felf -quiet -x -no-opt \
  -o "$test_dir/sgfx_mult320.o" src/graphics/sgfx_mult320.s
objects=("$test_dir/vga_to_chunky.o" "$test_dir/c2p1x1_8_c5_bm.o" "$test_dir/c2p16_interleaved.o" "$test_dir/particle_runtime.o" "$test_dir/sgfx_mult320.o")
m68k-amiga-elf-ld --section-start=code=0x1000 \
  -e slicks_chunky_rect_to_amiga --oformat=binary \
  -o "$test_dir/writers.bin" "${objects[@]}"
m68k-amiga-elf-ld --section-start=code=0x1000 \
  -e slicks_chunky_rect_to_amiga -o "$test_dir/writers.elf" "${objects[@]}"
symbols="$(m68k-amiga-elf-objdump -t "$test_dir/writers.elf")"
rect_address="$(awk '$NF == "slicks_chunky_rect_to_amiga" {print $1}' <<< "$symbols")"
pixels_address="$(awk '$NF == "slicks_chunky_pixels_to_amiga" {print $1}' <<< "$symbols")"
particles_address="$(awk '$NF == "slicks_advance_particles" {print $1}' <<< "$symbols")"
rows_address="$(awk '$NF == "slicks_chunky_rows_to_amiga" {print $1}' <<< "$symbols")"
test -n "$rect_address" && test -n "$pixels_address"
"${CC:-cc}" -O2 -Wall -Wextra -Werror -I"$unicorn_prefix/include" \
  tools/verify_planar_writes.c -L"$unicorn_prefix/lib" -lunicorn \
  -o "$test_dir/verify"
"$test_dir/verify" "$test_dir/writers.bin" "$rect_address" "$pixels_address" "$particles_address" disasm/runtime.bin "$rows_address"
printf 'Verifier artifacts: %s\n' "$test_dir"
