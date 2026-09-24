#include <stdio.h>
#include "../src/platform/amiga/amiga_joystick.h"
int main(void)
{
    unsigned cases=0;
    for(unsigned port=0;port<2;++port) for(unsigned directions=0;directions<16;++directions)
    for(unsigned buttons=0;buttons<4;++buttons) for(unsigned noise=0;noise<2;++noise) {
        unsigned up=directions&1,down=(directions>>1)&1,left=(directions>>2)&1,right=(directions>>3)&1;
        unsigned short joy=(unsigned short)((left<<9)|((up^left)<<8)|(right<<1)|(down^right));
        if(noise) joy|=0xfcfc;
        unsigned char cia=(unsigned char)(255-((buttons&1)<<(6+port)));
        unsigned short pot=(unsigned short)(65535-(((buttons>>1)&1)<<(10+4*port)));
        struct SlicksDeviceSample s=slicks_decode_amiga_joystick(joy,cia,pot,port);
        if(s.x!=(int)right-(int)left || s.y!=(int)down-(int)up || s.buttons!=buttons) return 1;
        ++cases;
    }
    printf("Amiga game ports: %u direction/button/port/noise decode cases pass\n",cases); return 0;
}
