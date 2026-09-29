#ifndef SLICKS_CAR_DISPLAY_H
#define SLICKS_CAR_DISPLAY_H

struct SlicksCarDisplay {
    signed char direction; /* Original DS:3068. */
    unsigned char ticks;   /* Original DS:3069, tested as signed after ADD. */
    short frame;           /* Frame submitted before the state update. */
};

/* Original 23d97..23e7a: humans use the current direction immediately;
 * computers retain it until the wrapping signed-byte counter exceeds five.
 * The actor receives the OLD frame on the update that crosses the threshold.
 * Matching directions do not clear a sub-threshold counter. This is display
 * state only: simulation heading and wheel-effect geometry are unchanged. */
static inline void slicks_car_display_step(struct SlicksCarDisplay *state,
    short heading,signed char role,unsigned short ticks)
{
    short direction=(short)(heading/1200);
    if(state->direction<0 || role<0) state->direction=(signed char)direction;
    state->frame=direction;
    if(state->direction!=direction) {
        state->frame=state->direction;
        state->ticks=(unsigned char)(state->ticks+(unsigned char)ticks);
    }
    if((signed char)state->ticks>5) {
        state->direction=(signed char)direction;
        state->ticks=0;
    }
}
#endif
