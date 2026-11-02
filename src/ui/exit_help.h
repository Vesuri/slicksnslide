#ifndef SLICKS_EXIT_HELP_H
#define SLICKS_EXIT_HELP_H

/* Original exit screen 1977:cd19..cd25: DOS scan codes 21 (Y) and 59 (F1)
 * open Help at topic "reg" (DS:1436). The registered original keeps this. */
static const unsigned char slicks_exit_help_topic[]="reg";
static inline int slicks_exit_help_requested(short scan)
{ return scan==21 || scan==59; }
#endif
