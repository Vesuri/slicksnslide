#include <stdio.h>
#include <string.h>
#include "../src/platform/amiga/copper_palette.h"
int main(void)
{
    unsigned long list[600],original[600];unsigned short words[2][256];
    unsigned at=29;
    memset(list,0,sizeof list);
    /* Full palette structure emitted by CopperList::setPalette24Bit. */
    for(unsigned half=0;half<2;++half)for(unsigned c=0;c<256;++c) {
        if(!(c&31)) list[at++]=0x01060000UL|((c&224)<<8)|(half<<9);
        list[at++]=((unsigned long)(0x180+2*(c&31))<<16)|0x456;
    }
    if(at!=557 || slicks_copper_palette_map(list,29,at,words))return 1;
    memcpy(original,list,sizeof list);
    for(unsigned first=0;first<256;++first) {
        memcpy(list,original,sizeof list);
        unsigned n=256-first<5?256-first:5;
        for(unsigned c=first;c<first+n;++c)for(unsigned half=0;half<2;++half) {
            unsigned i=words[half][c];
            if((list[i]>>16)!=0x180+2*(c&31))return 1;
            list[i]=(list[i]&0xffff0000UL)|0xabc;
        }
        unsigned bank=0,half=0;
        for(unsigned i=29;i<at;++i) {
            unsigned reg=list[i]>>16;
            if(reg==0x106) { bank=((unsigned)list[i]>>13)&7;half=((unsigned)list[i]>>9)&1; }
            else {
                unsigned c=bank*32+(reg-0x180)/2;
                unsigned value=c>=first && c<first+n?0xabc:0x456;
                if((unsigned short)list[i]!=value || words[half][c]!=i)return 1;
            }
        }
    }
    memcpy(list,original,sizeof list);list[words[0][199]]=0xfffffffe;
    if(!slicks_copper_palette_map(list,29,at,words))return 1;
    for(unsigned r=0;r<64;++r)for(unsigned g=0;g<64;++g)for(unsigned b=0;b<64;++b) {
        unsigned char rgb[3]={(unsigned char)r,(unsigned char)g,(unsigned char)b};
        unsigned long colour=(((r<<2)|(r>>4))<<16)|(((g<<2)|(g>>4))<<8)|((b<<2)|(b>>4));
        /* Framework's full-palette high/low nibble expressions. */
        unsigned high=((colour&0xf00000)>>12)|((colour&0xf000)>>8)|((colour&0xf0)>>4);
        unsigned low=((colour&0x0f0000)>>8)|((colour&0x0f00)>>4)|(colour&15);
        if(slicks_copper_vga_word(rgb,0)!=high || slicks_copper_vga_word(rgb,1)!=low)return 1;
    }
    puts("Copper palette: all banks/range boundaries, missing-entry rejection and 262144 VGA colours match framework encoding");return 0;
}
