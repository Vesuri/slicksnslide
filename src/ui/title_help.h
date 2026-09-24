#ifndef SLICKS_TITLE_HELP_H
#define SLICKS_TITLE_HELP_H
/* The title presents Load at 4, Read This at 5 and Quit at 6. Original
 * Read This calls 2a096 with DS:1436 ("reg"); F1 at 2a3cc passes DS:11d0
 * (empty topic). Resolve even the empty topic through the original index;
 * it can name an anchor and is not equivalent to the viewer's F1 body. */
static inline const unsigned char *slicks_title_help_topic(unsigned action,unsigned selection)
{
    if(action==3) return (const unsigned char *)"";
    if(action==2 && selection==5) return (const unsigned char *)"reg";
    return 0;
}
#endif
