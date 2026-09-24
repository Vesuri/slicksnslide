#ifndef SLICKS_DRIVER_INPUT_H
#define SLICKS_DRIVER_INPUT_H

/* Original keyboard callback 1aa3c..1aabb. Binding groups use DS:4bf2's
 * human-first order, not the same-numbered car. Bit order is the original
 * five DS:5344 bytes: accelerate, brake, left, right, fire. */
static inline void slicks_driver_key(unsigned char controls[4],
    const unsigned char keys[20],const unsigned char order[4],unsigned char scan)
{
    for(unsigned group=0;group<4;++group)
        for(unsigned action=0;action<5;++action)
            if(keys[group*5+action]==(scan&127)) {
                unsigned char mask=(unsigned char)(1U<<action);
                if(scan&128) controls[order[group]]&=(unsigned char)~mask;
                else controls[order[group]]|=mask;
            }
}
#endif
