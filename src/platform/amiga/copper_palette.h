#ifndef SLICKS_COPPER_PALETTE_H
#define SLICKS_COPPER_PALETTE_H
/* Locate palette MOVEs in the framework's actual copperlist output. */
static inline int slicks_copper_palette_map(const unsigned long *list,
    unsigned first,unsigned last,unsigned short words[2][256])
{
    for(unsigned half=0;half<2;++half)
        for(unsigned c=0;c<256;++c) words[half][c]=0xffff;
    unsigned bank=0,low=0;
    for(unsigned i=first;i<last;++i) {
        unsigned reg=(unsigned short)(list[i]>>16),data=(unsigned short)list[i];
        if(reg==0x106) { bank=(data>>13)&7;low=(data>>9)&1; }
        else if(reg>=0x180 && reg<=0x1be && !(reg&1)) {
            unsigned colour=bank*32+(reg-0x180)/2;
            if(words[low][colour]!=0xffff || i>=0xffff) return -1;
            words[low][colour]=(unsigned short)i;
        }
    }
    for(unsigned half=0;half<2;++half)
        for(unsigned c=0;c<256;++c) if(words[half][c]==0xffff) return -1;
    return 0;
}
static inline unsigned short slicks_copper_vga_word(const unsigned char *rgb,unsigned low)
{
    unsigned r=(rgb[0]<<2)|(rgb[0]>>4);
    unsigned g=(rgb[1]<<2)|(rgb[1]>>4);
    unsigned b=(rgb[2]<<2)|(rgb[2]>>4);
    if(low) return (unsigned short)(((r&15)<<8)|((g&15)<<4)|(b&15));
    return (unsigned short)(((r&240)<<4)|(g&240)|(b>>4));
}
#endif
