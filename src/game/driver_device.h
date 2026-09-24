#ifndef SLICKS_DRIVER_DEVICE_H
#define SLICKS_DRIVER_DEVICE_H

struct SlicksDriverDeviceState {
    short countdown;
    signed char previous_axis;
    unsigned char previous,rising;
};
struct SlicksDeviceSample { signed char x,y; unsigned char buttons; };

/* Original 199cf..19d03, after the platform's device sampling boundary.
 * Device is indexed by driver, unlike the keyboard binding group's order.
 * PC analogue devices 1/2 are rate-limited; digital/LPT devices 3..6 are
 * polled every update. The fifth keyboard action is never overwritten here.
 * Return 1 when sampling was due, 0 otherwise, -1 for invalid device data. */
static inline int slicks_driver_device(struct SlicksDriverDeviceState *s,
    unsigned char *controls,unsigned char device,signed char participation,
    signed char elapsed,signed char interval,unsigned char axis_throttle,
    short weapons,const struct SlicksDeviceSample *sample)
{
    signed char index=(signed char)(unsigned char)(device-1);
    int poll=participation<0 && index>=0;
    if(poll && index>=6) return -1;
    if(poll && index<2) {
        s->countdown=(short)(unsigned short)((unsigned short)s->countdown-elapsed);
        poll=s->countdown<=0;
        if(poll) s->countdown=interval;
    }
    if(poll) {
        if(!sample) return -1;
        unsigned char value=*controls&16;
        if(sample->x<0) value|=4;
        else if(sample->x>0) value|=8;
        if(index>=2 || axis_throttle) {
            if(sample->y<0) value|=1;
            else if(sample->y>0) value|=2;
            if(index>=2) {
                if(sample->buttons&1) value|=12;
            } else {
                if(weapons && (sample->buttons&1)) value|=2;
                if(weapons && (sample->buttons&2) && !s->previous_axis) value|=12;
                s->previous_axis=(signed char)(sample->buttons&2);
            }
        } else {
            if(sample->buttons&1) value|=1;
            if(sample->buttons&2) value|=2;
            if(sample->y<0 && weapons && s->previous_axis>=0) value|=12;
            s->previous_axis=sample->y;
        }
        *controls=value;
    }
    s->rising=(unsigned char)(*controls&~s->previous&15);
    s->previous=(unsigned char)(*controls&15);
    return poll;
}
#endif
