"""Inspect actual AGA planar output in a local WHDLoad debug dump, not reference imagery."""
from pathlib import Path
import struct
import sys
from PIL import Image
p=Path(sys.argv[1]);raw=(p/'.whdl_memory').read_bytes();seen=set()
for off in range(0,len(raw)-2232,2):
    if raw[off+24:off+36]!=bytes.fromhex('008e3881009000c101e42100'):continue
    pairs=struct.unpack_from('>1116H',raw,off)
    if tuple(pairs[26:58:2])!=tuple(range(0xe0,0x100,2)):continue
    pointers=[(pairs[27+i*4]<<16)|pairs[29+i*4] for i in range(8)]
    start=pointers[0]
    if pointers!=[start+i*40 for i in range(8)] or start+64000>len(raw) or start in seen:continue
    seen.add(start);colours=[[0,0,0] for _ in range(256)];bank=0;low=False
    for i in range(58,len(pairs),2):
        reg,value=pairs[i:i+2]
        if reg==0x106:bank=(value>>13)*32;low=bool(value&0x200)
        elif 0x180<=reg<0x1c0:
            c=colours[bank+(reg-0x180)//2]
            for j,shift in enumerate((8,4,0)):
                nibble=(value>>shift)&15;c[j]=(c[j]&0xf0)|nibble if low else (c[j]&15)|(nibble<<4)
    pixels=bytearray()
    for y in range(200):
        for x in range(320):
            index=sum(((raw[start+y*320+plane*40+x//8]>>(7-x%8))&1)<<plane for plane in range(8))
            pixels.extend(colours[index])
    target=p/f'planar-{off:06x}.png';Image.frombytes('RGB',(320,200),bytes(pixels)).save(target);print(target)
assert seen,'No verified Slicks AGA copper lists found'
