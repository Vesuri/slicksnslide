#ifndef SLICKS_MOVING_PROBE_H
#define SLICKS_MOVING_PROBE_H

struct SlicksMovingProbeOps {
    int (*track)(void *,short,short,signed char,signed char);
    signed char (*car)(void *,short,short,signed char,signed char);
    void (*wall)(void *,short,short,signed char);
    void *context;
};
/* 1cb02..1ccfb. Product truncation BEFORE signed division is intentional.
 * On no hit the caller's subpixel destination is untouched. A zero-length
 * ray probes cars only and also leaves the destination untouched on a hit.
 * The blocked starting pixel is ignored, but still probes cars at the last
 * clear position. Keep this distinct from skipping an entire blocked prefix.
 * -2 is the native unsupported-map boundary, not an original collision. */
static inline int slicks_moving_probe(short x,short y,short nx,short ny,
    short *out_x,short *out_y,signed char scale,signed char exclude,
    signed char sampling,signed char layer,const struct SlicksMovingProbeOps *ops)
{
    short dx=(short)(x-nx),dy=(short)(y-ny);
    short ax=dx<0?(short)-dx:dx,ay=dy<0?(short)-dy:dy;
    signed char sx=dx<0?1:-1,sy=dy<0?1:-1;
    short clear_x=x,clear_y=y;
    if(!ax && !(ay>ax))
        return exclude?ops->car(ops->context,x,y,(signed char)(exclude-1),layer):0;
    short distance=ay>ax?ay:ax;
    for(int step=0;step<=distance;++step) {
        short px,py;
        if(ay>ax) {
            px=(short)(x-(short)(step*dx)/ay); py=(short)(y+sy*step);
        } else {
            px=(short)(x+sx*step); py=(short)(y-(short)(step*dy)/ax);
        }
        int blocked=ops->track(ops->context,px,py,sampling,layer);
        if(blocked<0) return -2;
        if(blocked && step>0) {
            *out_x=(short)(scale*clear_x); *out_y=(short)(scale*clear_y);
            ops->wall(ops->context,clear_x,clear_y,sampling);
            return -1;
        }
        if(!blocked) { clear_x=px; clear_y=py; }
        if(exclude) {
            signed char hit=ops->car(ops->context,clear_x,clear_y,(signed char)(exclude-1),layer);
            if(hit) {
                *out_x=(short)(scale*clear_x); *out_y=(short)(scale*clear_y);
                return hit;
            }
        }
    }
    return 0;
}

/* 1ca5e..1cb01. The special-state test uses the EXCLUDED driver's slot,
 * not the candidate's slot. Exclude=4 is the caller's probe-all sentinel;
 * that original adjacent word must be provided explicitly by the owner. */
static inline signed char slicks_probe_cars(short x,short y,signed char exclude,
    signed char layer,const int car_x[4],const int car_y[4],const signed char roles[4],
    const unsigned char layers[4],short excluded_special)
{
    for(unsigned d=0;d<4;++d) {
        if((int)d==exclude || (int)layer!=(int)layers[d] || excluded_special || !roles[d]) continue;
        short dx=(short)(car_x[d]/100-x),dy=(short)(car_y[d]/100-y);
        if(dx>-5 && dx<5 && dy>-5 && dy<5) return (signed char)(d+1);
    }
    return 0;
}
#endif
