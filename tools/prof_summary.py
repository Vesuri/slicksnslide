#!/usr/bin/env python3
"""Summarize amiga/prof_sample.sh PC samples from the measured race window.

  tools/prof_summary.py tmp/prof-A.log [tmp/prof-B.log ...]
      [--elf amiga/out/SlicksDiag.elf] [--lines FUNC] [--callers FUNC]
      [--frames LO:HI] [--top N]

Samples are wall-clock spaced SIGINT stops, so shares estimate emulated
time including DMA contention. Wait loops are reported, not hidden.
"""
import argparse, bisect, collections, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
TC = os.path.expanduser('~/.local/opt/bin/m68k-amiga-elf-')

def load_symbols(elf):
    out = subprocess.run([TC + 'objdump', '-t', elf], capture_output=True,
                         text=True, check=True).stdout
    syms, sections = [], {}
    for line in out.splitlines():
        m = re.match(r'^([0-9a-f]{8}) (.{7}) (\S+)\s+([0-9a-f]{8}) (.*)$', line)
        if not m: continue
        addr, flags, sect, size, name = m.groups()
        if sect not in ('.text', 'code'): continue
        if 'd' in flags or 'f' in flags or name.startswith('.L'): continue
        syms.append((int(addr, 16), name, sect))
    syms.sort()
    hdr = subprocess.run([TC + 'objdump', '-h', elf], capture_output=True,
                         text=True, check=True).stdout
    for m in re.finditer(r'^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)', hdr, re.M):
        sections[m.group(1)] = (int(m.group(3), 16), int(m.group(2), 16))
    return syms, sections

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('logs', nargs='+')
    ap.add_argument('--elf', default=None,
                    help='defaults to LOG.elf beside the first log, else the current build')
    ap.add_argument('--lines', action='append', default=[])
    ap.add_argument('--callers', action='append', default=[])
    ap.add_argument('--inlined', action='append', default=[],
                    help='split FUNC samples by innermost inlined function')
    ap.add_argument('--frames', default='1:602')
    ap.add_argument('--top', type=int, default=45)
    ap.add_argument('--work', default=None,
                    help='LO:HI measured work lines filter')
    ap.add_argument('--particles', default=None,
                    help='LO:HI particle count filter (measured frame samples)')
    a = ap.parse_args()
    lo, hi = (int(x) for x in a.frames.split(':'))
    if a.elf is None:
        paired = os.path.splitext(a.logs[0])[0] + '.elf'
        a.elf = paired if os.path.exists(paired) else os.path.join(ROOT, 'amiga/out/SlicksDiag.elf')
    syms, sections = load_symbols(a.elf)
    addrs = [s[0] for s in syms]
    by_name = {s[1]: s[0] for s in syms}
    tstart, tsize = sections['.text']; cstart, csize = sections['code']

    def symbolize(elf_addr):
        i = bisect.bisect_right(addrs, elf_addr) - 1
        return syms[i][1] if i >= 0 else '?'

    samples = []  # (elf_pc or None, [elf return candidates])
    for log in a.logs:
        binary = log.endswith('.bin')
        text = open(log[:-4] + '.log' if binary else log, errors='replace').read()
        tb = re.search(r'TEXT_BASE=0x([0-9a-f]+)', text)
        cb = re.search(r'CODE_BASE=0x([0-9a-f]+)', text)
        toff = int(tb.group(1), 16) - by_name['slicks_race_step']
        coff = int(cb.group(1), 16) - by_name['slicks_advance_particles'] if cb else toff
        def to_elf(rt):
            if tstart <= rt - toff < tstart + tsize: return rt - toff
            if cstart <= rt - coff < cstart + csize: return rt - coff
            return None
        if binary:
            import struct
            data = open(log, 'rb').read()
            parts = {int(m.group(1)): int(m.group(2)) for m in
                     re.finditer(r'WORK_SAMPLE index=(\d+) lines=\d+ particles=(\d+)', text)}
            works = {int(m.group(1)): int(m.group(2)) for m in
                     re.finditer(r'WORK_SAMPLE index=(\d+) lines=(\d+)', text)}
            wlo, whi = (int(x) for x in a.work.split(':')) if a.work else (0, 1 << 30)
            plo, phi = (int(x) for x in a.particles.split(':')) if a.particles else (0, 1 << 30)
            for k in range(0, len(data) - 7, 8):
                pc, f, off, srhi = struct.unpack('>IHBB', data[k:k+8])
                # f is the index of the update being measured.
                if a.particles and not (plo <= parts.get(f, -1) <= phi): continue
                if a.work and not (wlo <= works.get(f, -1) <= whi): continue
                f += 1
                if lo <= f <= hi:
                    samples.append((to_elf(pc), [], srhi))
            continue
        lines = text.replace('(gdb) ', '').splitlines()
        i = 0
        while i < len(lines):
            m = re.search(r'S\d+ f=(\d+) pc=0x([0-9a-f]+)', lines[i])
            if not m: i += 1; continue
            f = int(m.group(1)); pc = int(m.group(2), 16)
            words = []
            j = i + 1
            while j < len(lines) and re.match(r'^0x[0-9a-f]+:\s', lines[j]):
                words += [int(w, 16) for w in re.findall(r'0x([0-9a-f]{8})', lines[j].split(':', 1)[1])]
                j += 1
            i = j
            if not (lo <= f <= hi): continue
            rets = [e for e in (to_elf(w) for w in words) if e is not None]
            samples.append((to_elf(pc), rets, 0))
    n = len(samples)
    if not n: sys.exit('no samples in frame window')
    funcs = collections.Counter()
    for pc, _, sr in samples:
        funcs[symbolize(pc) if pc is not None else '<ROM/other>'] += 1
    print(f'samples={n} (frames {lo}..{hi}, {len(a.logs)} log(s))')
    for name, c in funcs.most_common(a.top):
        print(f'{100.0*c/n:6.2f}% {c:5d}  {name}')
    for fn in a.callers:
        ctr = collections.Counter()
        for pc, rets, _ in samples:
            if pc is None or symbolize(pc) != fn: continue
            callers = [symbolize(r) for r in rets if symbolize(r) != fn]
            ctr[callers[0] if callers else '?'] += 1
        tot = sum(ctr.values())
        print(f'\ncallers of {fn} ({tot} samples; first foreign text word on stack):')
        for name, c in ctr.most_common(15):
            print(f'{100.0*c/max(tot,1):6.2f}% {c:5d}  {name}')
    for fn in a.inlined:
        pcs = collections.Counter(pc for pc, _, _ in samples
                                  if pc is not None and symbolize(pc) == fn)
        keys = list(pcs)
        out = subprocess.run([TC + 'addr2line', '-f', '-i', '-e', a.elf] +
                             ['@@'.join([]) or hex(p) for p in keys] + ['0'],
                             capture_output=True, text=True).stdout.split('\n')
        # addr2line -i emits (function, location) pairs per frame; a trailing
        # sentinel address 0 separates the final chain.
        chains, cur = [], []
        it = iter(range(0, len(out) - 1, 2))
        for k in it:
            cur.append(out[k])
            if out[k + 1].startswith('??') or True:
                pass
        # Recompute robustly: query one address at a time in batch mode.
        inner = collections.Counter()
        for p in keys:
            r = subprocess.run([TC + 'addr2line', '-f', '-i', '-e', a.elf, hex(p)],
                               capture_output=True, text=True).stdout.split('\n')
            inner[r[0]] += pcs[p]
        tot = sum(pcs.values())
        print(f'\n{fn}: {tot} samples by innermost inlined function')
        for name, c in inner.most_common(40):
            print(f'{100.0*c/tot:6.2f}% {c:5d}  {name}')
    for fn in a.lines:
        pcs = collections.Counter(pc for pc, _, _ in samples
                                  if pc is not None and symbolize(pc) == fn)
        if not pcs: print(f'\n{fn}: no samples'); continue
        res = subprocess.run([TC + 'addr2line', '-e', a.elf] +
                             [hex(p) for p in pcs], capture_output=True, text=True).stdout.split('\n')
        byline = collections.Counter()
        for (p, c), loc in zip(pcs.items(), res):
            byline[os.path.basename(loc) if '??' not in loc else hex(p)] += c
        tot = sum(pcs.values())
        print(f'\n{fn}: {tot} samples by source line')
        for loc, c in byline.most_common(30):
            print(f'{100.0*c/tot:6.2f}% {c:5d}  {loc}')

main()
