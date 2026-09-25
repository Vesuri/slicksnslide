#ifndef SLICKS_RESULT_WAIT_H
#define SLICKS_RESULT_WAIT_H
struct SlicksResultWaitOps {
    short (*key)(void *);
    unsigned char (*button)(void *);
    void (*delay)(void *,unsigned short);
    void (*clear)(void *);
    void *context;
};
/* Original 2b73b: first release the entry key, then wait for a different
 * make (or a game-port button). A positive limit always delays 100 ms,
 * including on key acceptance, and expires after limit+1 iterations. */
static inline short slicks_result_wait(short limit,unsigned char demo,const struct SlicksResultWaitOps *ops)
{
    if(demo && (limit<=0 || limit>30)) limit=30;
    short previous,key,count=0;
    do { previous=ops->key(ops->context); } while(previous<128);
    do {
        key=ops->key(ops->context);
        if(ops->button(ops->context)) key=57;
        if(key>127) key=previous;
        if(limit>0) {
            ops->delay(ops->context,100);
            count=(short)(unsigned short)(count+1);
            if(count>limit) key=57;
        }
    } while(key==previous);
    ops->clear(ops->context);
    return key;
}
#endif
