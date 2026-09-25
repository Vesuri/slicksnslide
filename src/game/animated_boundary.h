#ifndef SLICKS_ANIMATED_BOUNDARY_H
#define SLICKS_ANIMATED_BOUNDARY_H
/* Original 1fec8..1ff97. Boundary collision classes and palette animation
 * share one level; the endpoint pause consumes exactly one RNG draw. */
static inline int slicks_advance_boundary(short *level,short *timer,short *direction,
    unsigned short ticks,unsigned long *random_state,unsigned char colours[15])
{
    *timer=(short)((unsigned short)*timer-ticks);
    if(*timer>=0) return 0;
    *level=(short)((unsigned short)*level+(unsigned short)*direction);
    if(*level<0 || *level>5) {
        *random_state=(*random_state*0x015a4e35UL+1UL)&0xffffffffUL;
        *timer=(short)((((*random_state>>16)&32767UL)*150UL)/32768UL+100);
        *level=*level<0?0:5;
        *direction=(short)(0U-(unsigned short)*direction);
        return 0;
    }
    *timer=10;
    for(unsigned i=0;i<5;++i) {
        int distance=*level-(int)i;
        colours[i*3]=distance>0?63:35;
        colours[i*3+1]=distance>0?(unsigned char)(51+distance*2):35;
        colours[i*3+2]=distance>0?(unsigned char)(51-distance*10):35;
    }
    return 1;
}
#endif
