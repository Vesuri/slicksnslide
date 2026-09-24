"""Join DOS construction writes to unambiguous placed DAT source pixels.

Ambiguous overlaps are excluded, not guessed. This reports evidence; it does
not generate production lookup tables or claim complete track equivalence.
"""
import csv
import sys
from collections import Counter, defaultdict
from pathlib import Path

dat, track, writes = map(Path, sys.argv[1:4])
types = set(map(int, sys.argv[4:]))
b = dat.read_bytes()
at = 0
sprites = []
for _ in range(110):
    fmt, width, height = b[at:at+3]
    at += 3
    width |= (fmt & 1) << 8
    pixels = []
    if fmt & 2:
        escape = b[at]
        at += 1
        while len(pixels) < width * height:
            value, count = b[at], 1
            at += 1
            if value == escape:
                count = b[at]
                at += 1
                if count:
                    value = b[at]
                    at += 1
                else:
                    count = 1
            pixels.extend([value] * count)
    else:
        pixels = list(b[at:at+width*height])
        at += width * height
    sprites.append((width, height, pixels))
b = track.read_bytes()
at = 6 + 0x165 + 15
for _ in range(2):
    at = b.index(0, at) + 1
at += 8
count = int.from_bytes(b[at:at+2], "big")
at += 2
source = defaultdict(set)
for _ in range(count):
    ox = int.from_bytes(b[at:at+2], "big")
    oy, kind, rotation = b[at+2:at+5]
    at += 5
    if kind not in types:
        continue
    width, height, pixels = sprites[kind]
    for sy in range(height):
        for sx in range(width):
            x, y = ((sx, sy), (height-1-sy, sx),
                    (width-1-sx, height-1-sy), (sy, width-1-sx))[rotation & 3]
            source[kind, ox+x, oy+y].add(pixels[sy*width+sx])
counts = Counter()
ambiguous = 0
with writes.open() as file:
    for row in csv.DictReader(file):
        key = tuple(int(row[k]) for k in ("object", "x", "y"))
        if key[0] not in types:
            continue
        pixels = source[key]
        if len(pixels) != 1:
            ambiguous += 1
            continue
        counts[key[0], next(iter(pixels)), int(row["surface"]), int(row["colour"])] += 1
print(f"Excluded ambiguous/missing source coordinates: {ambiguous}")
print("type palette mode1 mode0 writes")
for key, count in sorted(counts.items()):
    print(*key, count)
