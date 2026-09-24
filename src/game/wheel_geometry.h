#ifndef SLICKS_WHEEL_GEOMETRY_H
#define SLICKS_WHEEL_GEOMETRY_H

/* 1c2e9..1c3fb scans the already-rotated sprite column-first, before driver
 * colour remapping. Missing wheels invalidate X only; their Y stays intact.
 * A third 255 marker would overwrite unrelated DOS state. Reject that
 * unsupported asset rather than silently truncate it or corrupt native RAM. */
static inline int slicks_extract_wheels(unsigned char *pixels,
    unsigned width, unsigned height, signed char x[2], signed char y[2])
{
    unsigned count=0;
    x[0]=x[1]=-1;
    for(unsigned column=0;column<width;++column)
    for(unsigned row=0;row<height;++row) {
        unsigned char *pixel=&pixels[row*width+column];
        if(*pixel==255 || (*pixel==254 && count<2)) {
            if(count==2) return -1;
            x[count]=(signed char)column; y[count]=(signed char)row;
            ++count;
            *pixel=*pixel==255?25:184;
        }
    }
    return 0;
}
#endif
