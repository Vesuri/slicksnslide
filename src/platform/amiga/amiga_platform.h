#ifndef SLICKS_AMIGA_PLATFORM_H
#define SLICKS_AMIGA_PLATFORM_H

#include <exec/interrupts.h>
#include <graphics/gfx.h>
#include <graphics/view.h>

#define SLICKS_AMIGA_VIEW_COUNT 2

struct GfxBase;

struct SlicksAmigaView {
    struct BitMap *bitmap;
    unsigned long *copper;
};

struct SlicksAmigaPlatform {
    struct GfxBase *gfx_base;
    struct View *saved_view;
    unsigned long *saved_copper;
    struct IntVector saved_vertb;
    struct Interrupt vertb_interrupt;
    struct SlicksAmigaView views[SLICKS_AMIGA_VIEW_COUNT];
    struct Library *ciaa_base;
    struct Interrupt keyboard_interrupt;
    struct Interrupt *saved_keyboard_interrupt;
    volatile unsigned long vblank_count;
    volatile unsigned char key_head;
    volatile unsigned char key_tail;
    volatile unsigned short keys[16]; /* Raw byte plus event-time Shift bits. */
    unsigned char keyboard_shifts; /* Interrupt producer state. */
    unsigned char key_shifts; /* Modifiers of the last polled event. */
    unsigned short saved_dma;
    unsigned short saved_interrupts;
    unsigned char active;
    unsigned char vertb_taken;
    unsigned char io_active;
};

#ifdef __cplusplus
extern "C" {
#endif

int slicks_amiga_platform_create(struct SlicksAmigaPlatform *platform,
                                 struct GfxBase *gfx_base);
void slicks_amiga_platform_destroy(struct SlicksAmigaPlatform *platform);
int slicks_amiga_platform_set_view(struct SlicksAmigaPlatform *platform,
                                  unsigned short view,
                                  const unsigned char *vga_palette);
int slicks_amiga_platform_update_palette(struct SlicksAmigaPlatform *platform,
    unsigned short view,unsigned short first,unsigned short count,const unsigned char *rgb);
int slicks_amiga_platform_begin(struct SlicksAmigaPlatform *platform,
                               unsigned short view);
void slicks_amiga_platform_show(struct SlicksAmigaPlatform *platform,
                               unsigned short view);
void slicks_amiga_platform_wait_vblank(struct SlicksAmigaPlatform *platform);
void slicks_amiga_platform_wait_display_blank(
    struct SlicksAmigaPlatform *platform);
void slicks_amiga_platform_wait_display_end(
    struct SlicksAmigaPlatform *platform);
int slicks_amiga_platform_poll_key(struct SlicksAmigaPlatform *platform,
                                  unsigned short *raw);
int slicks_amiga_platform_left_mouse(void);
int slicks_amiga_platform_right_mouse(void);
struct SlicksDeviceSample;
int slicks_amiga_platform_joystick(unsigned device,struct SlicksDeviceSample *sample);
void slicks_amiga_platform_end(struct SlicksAmigaPlatform *platform);
/* Temporary OS file-I/O window; keep the custom copper/bitmap installed.
 * Caller must stop Paula playback and finish blitter work before entry.
 * No menu/input or bitmap/copper mutation until end_io returns. */
int slicks_amiga_platform_begin_io(struct SlicksAmigaPlatform *platform);
int slicks_amiga_platform_end_io(struct SlicksAmigaPlatform *platform);
unsigned short slicks_amiga_platform_restore_status(
    const struct SlicksAmigaPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif
