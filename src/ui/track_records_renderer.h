#ifndef SLICKS_TRACK_RECORDS_RENDERER_H
#define SLICKS_TRACK_RECORDS_RENDERER_H
#include "track_records_draw.h"
#include "chunky_ui.h"
#include "race_hud.h"

struct SlicksRecordsRenderer {
    struct SlicksChunkyUi ui;
    unsigned char *fonts[2],highlight;
    int error;
    int (*text)(void *,struct SlicksChunkyUi *,unsigned char *,const unsigned char *,
        short,short,unsigned char,unsigned char);
    /* Original marker IDs, not an assumed player-menu icon-table offset. */
    int (*icon)(void *,struct SlicksChunkyUi *,short,short,short);
    void *context;
};
static inline void slicks_records_decimal(short value,unsigned char text[7])
{
    unsigned char reverse[5]; unsigned count=0,at=0;
    /* 302b6 negates in a signed 16-bit register before signed division.
     * Preserve its -32768 overflow case rather than widening the negate. */
    short magnitude=value<0?(short)(unsigned short)(-(long)value):value;
    if(value<0) text[at++]='-';
    do { reverse[count++]=(unsigned char)('0'+magnitude%10); magnitude/=10; } while(magnitude);
    while(count) text[at++]=reverse[--count];
    text[at]=0;
}
static inline short slicks_records_render_command(void *context,
    enum SlicksRecordsDrawCommand command,const short *a,const unsigned char *string)
{
    struct SlicksRecordsRenderer *r=context;
    if(r->error) return 0;
    unsigned font=0,flags=0; unsigned char text[9];
    switch(command) {
    case SLICKS_RECORDS_NEAREST: return slicks_ui_nearest(&r->ui,a[0],a[1],a[2]);
    case SLICKS_RECORDS_HIGHLIGHT: r->highlight=(unsigned char)a[0]; return 0;
    case SLICKS_RECORDS_COLOUR: r->fonts[a[0]][6]=(unsigned char)a[1]; return 0;
    case SLICKS_RECORDS_RECTANGLE:
        slicks_ui_rectangle(&r->ui,a[0],a[1],a[2],a[3],(unsigned char)a[4]); return 0;
    case SLICKS_RECORDS_ICON:
        r->error=r->icon(r->context,&r->ui,a[2],a[0],a[1]); return 0;
    case SLICKS_RECORDS_TEXT: font=(unsigned)a[2]; flags=(unsigned)a[3]; break;
    case SLICKS_RECORDS_TIME:
        slicks_hud_time((unsigned short)a[2],(char *)text);
        string=text; font=(unsigned)a[3]; flags=(unsigned)(a[5]<<2); break;
    case SLICKS_RECORDS_NUMBER:
        slicks_records_decimal(a[2],text); string=text; font=(unsigned)a[3]; flags=(unsigned)a[4]; break;
    case SLICKS_RECORDS_CHARACTER:
        text[0]=(unsigned char)a[2]; text[1]=0; string=text; font=(unsigned)a[3]; flags=(unsigned)a[4]; break;
    }
    if(font>=2 || !string) { r->error=-1; return 0; }
    r->error=r->text(r->context,&r->ui,r->fonts[font],string,a[0],a[1],
        (unsigned char)flags,r->highlight);
    return 0;
}
static inline int slicks_records_renderer_draw(struct SlicksRecordsRenderer *r,
    const struct SlicksTrackRecords *records,const signed char ranks[4],short x,short y,
    unsigned char separator,signed char date_order)
{
    if(!r || !records || !ranks || !r->ui.pixels || !r->ui.palette ||
       !r->fonts[0] || !r->fonts[1] || !r->text || !r->icon) return -1;
    r->error=0;
    const struct SlicksRecordsDrawOps ops={slicks_records_render_command,r};
    slicks_draw_track_records(records,ranks,x,y,separator,date_order,&ops);
    return r->error;
}
#endif
