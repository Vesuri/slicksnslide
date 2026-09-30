#!/usr/bin/env python3
"""Render a dumped 320x200x8 AGA bitmap (Planes[0]..Planes[7] span) with a
dumped 768-byte 6-bit VGA palette to PNG. Usage: PLANES PALETTE BPR P0 P1 OUT
(plane pointers as printed by the capturing gdb script, decimal or 0x hex)."""
import sys, zlib, struct
planes, palette, bpr, p0, p1, out = sys.argv[1:7]
bpr = int(bpr, 0); p0 = int(p0, 0); p1 = int(p1, 0)
data = open(planes, 'rb').read(); pal = open(palette, 'rb').read()
step = p1 - p0          # plane stride inside the dump (interleaved or not)
rows = []
for y in range(200):
    row = bytearray([0])
    for x in range(320):
        c = 0
        for p in range(8):
            if data[p * step + y * bpr + x // 8] & (0x80 >> (x & 7)): c |= 1 << p
        r, g, b = (v << 2 | v >> 4 for v in pal[3 * c:3 * c + 3])
        row += bytes((r, g, b))
    rows.append(bytes(row))
def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d))
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 320, 200, 8, 2, 0, 0, 0)) + \
      chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b'')
open(out, 'wb').write(png)
