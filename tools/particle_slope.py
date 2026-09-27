#!/usr/bin/env python3
"""Per-function cost against live particle count from CIA-B PC samples.

  tools/particle_slope.py tmp/pcprof-LABEL.bin [more.bin ...] [--top N]

Each .bin pairs with its .log (WORK_SAMPLE lines, TEXT_BASE/CODE_BASE) and
.elf, as written by amiga/pc_profile.sh. For every function it prints the
mean raster lines per update, the zero-particle intercept and the slope per
100 live particles (one sample is about 25.4 lines: the sampler period
averages 1155 E-clock ticks). Wait loops are listed but excluded from totals.
"""
import argparse, bisect, collections, os, re, struct, subprocess

TC = os.path.expanduser('~/.local/opt/bin/m68k-amiga-elf-')
LINES_PER_SAMPLE = 25.4

def load_symbols(elf):
    out = subprocess.run([TC + 'objdump', '-t', elf], capture_output=True,
                         text=True, check=True).stdout
    syms = []
    for line in out.splitlines():
        m = re.match(r'^([0-9a-f]{8}) (.{7}) (\S+)\s+([0-9a-f]{8}) (.*)$', line)
        if not m or m.group(3) not in ('.text', 'code'): continue
        if 'd' in m.group(2) or 'f' in m.group(2) or m.group(5).startswith('.L'): continue
        syms.append((int(m.group(1), 16), m.group(5), m.group(3)))
    syms.sort()
    hdr = subprocess.run([TC + 'objdump', '-h', elf], capture_output=True,
                         text=True, check=True).stdout
    sections = {m.group(1): (int(m.group(3), 16), int(m.group(2), 16)) for m in
                re.finditer(r'^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)', hdr, re.M)}
    return syms, sections

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('bins', nargs='+')
    ap.add_argument('--top', type=int, default=30)
    a = ap.parse_args()
    for path in a.bins:
        stem = path[:-4]
        syms, sections = load_symbols(stem + '.elf')
        addrs = [s[0] for s in syms]
        by_name = {s[1]: s[0] for s in syms}
        text = open(stem + '.log', errors='replace').read()
        toff = int(re.search(r'TEXT_BASE=0x([0-9a-f]+)', text).group(1), 16) - by_name['slicks_race_step']
        cb = re.search(r'CODE_BASE=0x([0-9a-f]+)', text)
        coff = int(cb.group(1), 16) - by_name['slicks_advance_particles'] if cb else toff
        parts = {int(m.group(1)): int(m.group(2)) for m in
                 re.finditer(r'WORK_SAMPLE index=(\d+) lines=\d+ particles=(\d+)', text)}
        (ts, tl), (cs, cl) = sections['.text'], sections['code']
        per = collections.defaultdict(collections.Counter)
        data = open(path, 'rb').read()
        for k in range(0, len(data) - 7, 8):
            pc, frame, _, _ = struct.unpack('>IHBB', data[k:k + 8])
            name = '<ROM/other>'
            for off, (start, size) in ((toff, (ts, tl)), (coff, (cs, cl))):
                e = pc - off
                if start <= e < start + size:
                    name = syms[bisect.bisect_right(addrs, e) - 1][1]
                    break
            per[frame][name] += 1
        frames = sorted(parts)
        xs = [parts[f] for f in frames]
        n = len(xs); mx = sum(xs) / n
        sxx = sum((x - mx) ** 2 for x in xs) or 1
        names = collections.Counter()
        for f in per: names.update(per[f])
        rows = []
        for name in names:
            ys = [per[f][name] for f in frames]
            my = sum(ys) / n
            b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx
            rows.append((my * LINES_PER_SAMPLE, (my - b * mx) * LINES_PER_SAMPLE,
                         b * LINES_PER_SAMPLE * 100, name))
        rows.sort(reverse=True)
        print(f'== {path}: {n} updates, mean {mx:.1f} particles')
        print('   mean  zero-pt  /100pt  function')
        work = [r for r in rows if 'wait_display' not in r[3]]
        for m, i, s, name in rows[:a.top]:
            print(f'{m:7.1f} {i:8.1f} {s:7.1f}  {name}')
        print(f'work total {sum(r[0] for r in work):.0f}, intercept {sum(r[1] for r in work):.0f}, '
              f'per 100 points {sum(r[2] for r in work):.0f}')

if __name__ == '__main__':
    main()
