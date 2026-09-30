#include <exec/execbase.h>
#include <exec/memory.h>
#include <graphics/display.h>
#include <graphics/gfxbase.h>
#include <hardware/cia.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <proto/cia.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <resources/cia.h>

#include "amiga_platform.h"
#include "amiga_key_scan.h"
#include "amiga_joystick.h"
#include "../../ui/key_repeat.h"
#include "amiga_audio.h"
#include "copper_palette.h"
#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"
#include "framework/CopperList.h"
#include "framework/Palette24Bit.h"

#define CUSTOM_WORD(offset) (*(volatile unsigned short *)(0xdff000UL + (offset)))
#define CIAA_PRA (*(volatile unsigned char *)0xbfe001UL)
#define CIAA_SDR (*(volatile unsigned char *)0xbfec01UL)
#define CIAA_CRA (*(volatile unsigned char *)0xbfee01UL)

#define REG_COPJMP1 0x088
#define REG_DMACON  0x096
#define REG_INTENA  0x09a
#define REG_INTREQ  0x09c
#define REG_BPLCON3 0x106
#define REG_VPOSR   0x004
#define REG_VHPOSR  0x006

#define COPPER_LONGS 558
#define BITMAP_BYTES (320UL * 200UL)

static struct SlicksAmigaPlatform *active_platform;
static Bitmap *framework_bitmaps[SLICKS_AMIGA_VIEW_COUNT];
static CopperList *framework_copper[SLICKS_AMIGA_VIEW_COUNT];
static unsigned short palette_words[SLICKS_AMIGA_VIEW_COUNT][2][256];
static unsigned char palette_valid[SLICKS_AMIGA_VIEW_COUNT];
/* Source VGA palettes of the copper lists, for painters that tint the
 * currently displayed view (the original reads its live DAC copy). */
static unsigned char view_palettes[SLICKS_AMIGA_VIEW_COUNT][768];
static unsigned short shown_view;
/* Original DS:174a/174e: shared by every repeating owner. */
static struct SlicksKeyRepeat key_repeat;
extern "C" unsigned char g_slicks_diag_race_load_fault;
static unsigned char create_fault, create_allocation, create_fault_consumed;
static bool fail_create_allocation()
{
    if (!create_fault) return false;
    if (++create_allocation != create_fault) return false;
    create_fault_consumed = 1;
    return true;
}

static unsigned char expand_vga_component(unsigned char value)
{
    return (unsigned char)((value << 2) | (value >> 4));
}

static unsigned long build_copper(unsigned short view_index,
                                  const unsigned char *palette)
{
    unsigned long colours[256];
    unsigned short colour;
    unsigned long at;
    CopperList *list = framework_copper[view_index];
    Bitmap *bitmap = framework_bitmaps[view_index];
    palette_valid[view_index]=0;

    for (at = 0; at < 768; ++at)
        view_palettes[view_index][at] = palette[at];
    for (colour = 0; colour < 256; ++colour) {
        unsigned long r = expand_vga_component(palette[colour * 3]);
        unsigned long g = expand_vga_component(palette[colour * 3 + 1]);
        unsigned long b = expand_vga_component(palette[colour * 3 + 2]);
        colours[colour] = (r << 16) | (g << 8) | b;
    }
    Palette24Bit aga_palette(colours, 256);
    /* DIWHIGH carries the ninth vertical stop bit. Centre the 200-line
     * playfield at $9c so the window is exactly $38..$ff (VSTOP=$100),
     * matching the framework's extended-window setup. */
    at = list->setPlayfield(1, 320, 200, 8, true, false, false,
                            false, false, true, 0x9c);
    list->showBitmap(at, *bitmap);
    at += 16;
    unsigned long palette_at=at;
    at = list->setPalette24Bit(at, aga_palette, 0, 0, 255, true);
    if(slicks_copper_palette_map(list->data(),palette_at,at,palette_words[view_index]))
        return 0;
    palette_valid[view_index]=1;
    return at;
}

static int validate_framework_view(unsigned short view_index)
{
    const Bitmap *bitmap = framework_bitmaps[view_index];
    const unsigned long *list = framework_copper[view_index]->data();
    unsigned long bitplane;
    unsigned short plane;

    if (!bitmap || !list || bitmap->widthInBytes != 40 ||
        bitmap->rowSizeInBytes != 320 || bitmap->bitplanes != 8 ||
        !bitmap->interleaved)
        return -1;
    if (list[11] != copperMove(bpl1mod, 280) ||
        list[12] != copperMove(bpl2mod, 280))
        return -1;
    if (list[6] != copperMove(diwstrt, 0x3881) ||
        list[7] != copperMove(diwstop, 0x00c1) ||
        list[8] != copperMove(diwhigh, 0x2100))
        return -1;
    bitplane = (unsigned long)bitmap->data;
    for (plane = 0; plane < 8; ++plane, bitplane += 40) {
        unsigned short reg = (unsigned short)(bpl1pth + plane * 4);
        if (list[13 + plane * 2] != copperMove(reg, bitplane >> 16) ||
            list[14 + plane * 2] != copperMove(reg + 2, bitplane))
            return -1;
    }
    return 0;
}

static void install_copper(unsigned short view, int immediate)
{
    shown_view = view;
    AmigaHardware::setCopperList(*framework_copper[view], immediate != 0);
}

static void install_raw_copper(unsigned long *data, int immediate)
{
    CopperList list(data);
    AmigaHardware::setCopperList(list, immediate != 0);
}

static unsigned long vertical_blank_handler(void)
{
    CUSTOM_WORD(REG_INTREQ) = INTF_VERTB;
    if (active_platform) {
        struct SlicksAmigaPlatform *p = active_platform;
        ++p->vblank_count;
        /* BIOS tick: PIT input / 65536 (18.2065 Hz), 1/50 s per vblank. */
        p->bios_remainder += 1193182UL;
        if (p->bios_remainder >= 65536UL * 50UL) {
            p->bios_remainder -= 65536UL * 50UL;
            ++p->bios_ticks;
        }
    }
    slicks_amiga_audio_vblank();
    return 0;
}

static unsigned long keyboard_handler(void)
{
    struct SlicksAmigaPlatform *platform = active_platform;
    unsigned char encoded = CIAA_SDR;
    unsigned char code;
    unsigned char next;
    volatile unsigned short delay;

    CIAA_CRA |= CIACRAF_SPMODE;
    for (delay = 0; delay < 200; ++delay)
        ;
    CIAA_CRA &= (unsigned char)~CIACRAF_SPMODE;

    code = (unsigned char)~encoded;
    code = (unsigned char)((code >> 1) | (code << 7));
    if (!platform)
        return 0;
    unsigned short event=slicks_amiga_key_event(code,&platform->keyboard_shifts);
    platform->key_latch = code; /* 36e29: every make and break. */
    next = (unsigned char)((platform->key_head + 1) & 15);
    if (next != platform->key_tail) {
        platform->keys[platform->key_head] = event;
        platform->key_head = next;
    }
    return 0;
}

static int keyboard_begin(struct SlicksAmigaPlatform *platform)
{
    /* Releases while AmigaOS owns the keyboard are not delivered to us. */
    platform->keyboard_shifts=0;
    platform->key_latch=0x80;
    platform->ciaa_base = OpenResource((CONST_STRPTR)CIAANAME);
    if (!platform->ciaa_base)
        return -1;
    platform->keyboard_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    platform->keyboard_interrupt.is_Node.ln_Pri = 0;
    platform->keyboard_interrupt.is_Node.ln_Name = (char *)"Slicks keyboard";
    platform->keyboard_interrupt.is_Data = 0;
    platform->keyboard_interrupt.is_Code = (void (*)())keyboard_handler;
    platform->saved_keyboard_interrupt = AddICRVector(
        platform->ciaa_base, CIAICRB_SP, &platform->keyboard_interrupt);
    if (platform->saved_keyboard_interrupt) {
        RemICRVector(platform->ciaa_base, CIAICRB_SP,
                     platform->saved_keyboard_interrupt);
        AddICRVector(platform->ciaa_base, CIAICRB_SP,
                     &platform->keyboard_interrupt);
    }
    return 0;
}

static void keyboard_end(struct SlicksAmigaPlatform *platform)
{
    if (!platform->ciaa_base)
        return;
    RemICRVector(platform->ciaa_base, CIAICRB_SP,
                 &platform->keyboard_interrupt);
    if (platform->saved_keyboard_interrupt) {
        AddICRVector(platform->ciaa_base, CIAICRB_SP,
                     platform->saved_keyboard_interrupt);
        platform->saved_keyboard_interrupt = 0;
    }
    platform->ciaa_base = 0;
}

int slicks_amiga_platform_create(struct SlicksAmigaPlatform *platform,
                                 struct GfxBase *gfx_base)
{
    unsigned short view;
    unsigned short plane;
    if (!platform || !gfx_base || !(gfx_base->ChipRevBits0 & GFXF_AA_LISA))
        return -1;
    for (view = 0; view < sizeof(*platform); ++view)
        ((unsigned char *)platform)[view] = 0;
    platform->gfx_base = gfx_base;
    AmigaHardware::hasAGAChipSet = true;
    for (view = 0; view < SLICKS_AMIGA_VIEW_COUNT; ++view) {
        unsigned char *data = fail_create_allocation() ? 0 : (unsigned char *)AllocMem(
            BITMAP_BYTES, MEMF_CHIP | MEMF_CLEAR);
        platform->views[view].bitmap = fail_create_allocation() ? 0 : (struct BitMap *)AllocMem(
            sizeof(struct BitMap), MEMF_ANY | MEMF_CLEAR);
        platform->views[view].copper = fail_create_allocation() ? 0 : (unsigned long *)AllocMem(
            COPPER_LONGS * sizeof(unsigned long), MEMF_CHIP | MEMF_CLEAR);
        if (!data || !platform->views[view].bitmap ||
            !platform->views[view].copper) {
            if (data)
                FreeMem(data, BITMAP_BYTES);
            slicks_amiga_platform_destroy(platform);
            return -1;
        }
        platform->views[view].bitmap->BytesPerRow = 320;
        platform->views[view].bitmap->Rows = 200;
        platform->views[view].bitmap->Flags = BMF_INTERLEAVED;
        platform->views[view].bitmap->Depth = 8;
        for (plane = 0; plane < 8; ++plane)
            platform->views[view].bitmap->Planes[plane] = data + plane * 40;
        framework_bitmaps[view] = fail_create_allocation() ? 0 : new Bitmap(data, 320, 200, 8, true);
        framework_copper[view] = fail_create_allocation() ? 0 : new CopperList(
            platform->views[view].copper, COPPER_LONGS);
        if (!framework_bitmaps[view] || !framework_copper[view]) {
            slicks_amiga_platform_destroy(platform);
            return -1;
        }
    }
    return 0;
}

void slicks_amiga_platform_destroy(struct SlicksAmigaPlatform *platform)
{
    unsigned short view;
    if (!platform)
        return;
    if (platform->active)
        slicks_amiga_platform_end(platform);
    for (view = 0; view < SLICKS_AMIGA_VIEW_COUNT; ++view) {
        unsigned char *data = platform->views[view].bitmap
                                  ? (unsigned char *)platform->views[view]
                                        .bitmap->Planes[0]
                                  : 0;
        if (framework_copper[view]) {
            delete framework_copper[view];
            framework_copper[view] = 0;
        }
        if (framework_bitmaps[view]) {
            delete framework_bitmaps[view];
            framework_bitmaps[view] = 0;
        }
        if (platform->views[view].copper) {
            FreeMem(platform->views[view].copper,
                    COPPER_LONGS * sizeof(unsigned long));
            platform->views[view].copper = 0;
        }
        if (platform->views[view].bitmap) {
            FreeMem(platform->views[view].bitmap, sizeof(struct BitMap));
            platform->views[view].bitmap = 0;
        }
        if (data)
            FreeMem(data, BITMAP_BYTES);
    }
}

int slicks_amiga_platform_check_create_failures(struct GfxBase *gfx_base)
{
    /* Explicit diagnostic entry only, before the real display is created.
     * Forbid keeps other tasks from changing AvailMem during each check;
     * interrupts remain enabled and no hardware takeover occurs here. */
    if (active_platform) return -1;
    for (unsigned view=0; view<SLICKS_AMIGA_VIEW_COUNT; ++view)
        if (framework_bitmaps[view] || framework_copper[view]) return -1;
    for (unsigned stage=1; stage<=10; ++stage) {
        struct SlicksAmigaPlatform trial = {};
        Forbid();
        unsigned long before = AvailMem(MEMF_ANY);
        create_fault=stage; create_allocation=0; create_fault_consumed=0;
        int result=slicks_amiga_platform_create(&trial,gfx_base);
        create_fault=0;
        bool clean=result==-1 && create_fault_consumed && !trial.active;
        for (unsigned view=0; view<SLICKS_AMIGA_VIEW_COUNT; ++view)
            if (trial.views[view].bitmap || trial.views[view].copper ||
                framework_bitmaps[view] || framework_copper[view]) clean=false;
        /* Also check the caller's ordinary second cleanup is harmless. */
        slicks_amiga_platform_destroy(&trial);
        if (AvailMem(MEMF_ANY)!=before) clean=false;
        Permit();
        if (!clean) return -(int)stage;
    }
    return 10;
}

int slicks_amiga_platform_set_view(struct SlicksAmigaPlatform *platform,
                                  unsigned short view,
                                  const unsigned char *vga_palette)
{
    if (!platform || view >= SLICKS_AMIGA_VIEW_COUNT || !vga_palette)
        return -1;
    /* Menu-to-menu palette replacement must not rewrite an executing copper
     * list. The display remains owned; there is no LoadView/Permit handoff. */
    if (platform->active)
        slicks_amiga_platform_wait_display_blank(platform);
    if (build_copper(view, vga_palette) != COPPER_LONGS - 1)
        return -1;
    /* Native lifecycle fixture: reject an invalid, not-yet-installed race
     * list through the real validator, then restore it before returning.
     * The loading display may still own view 0 while view 1 is built. */
    if (view == 1 && (!platform->active || shown_view != 1) &&
        g_slicks_diag_race_load_fault == 8) {
        g_slicks_diag_race_load_fault = 0;
        unsigned long *list = framework_copper[view]->data();
        unsigned long saved = list[11];
        list[11] ^= 1;
        int result = validate_framework_view(view);
        list[11] = saved;
        return result;
    }
    return validate_framework_view(view);
}

int slicks_amiga_platform_update_palette(struct SlicksAmigaPlatform *platform,
    unsigned short view,unsigned short first,unsigned short count,const unsigned char *rgb)
{
    if(!platform || view>=SLICKS_AMIGA_VIEW_COUNT || !rgb ||
       first>256 || count>256-first || !framework_copper[view] || !palette_valid[view]) return -1;
    CopperList *list=framework_copper[view];
    for(unsigned c=0;c<3*count;++c) view_palettes[view][3*first+c]=rgb[c];
    for(unsigned c=0;c<count;++c) {
        list->setColor(palette_words[view][0][first+c],slicks_copper_vga_word(rgb+c*3,0),1);
        list->setColor(palette_words[view][1][first+c],slicks_copper_vga_word(rgb+c*3,1),1);
    }
    return 0;
}

int slicks_amiga_platform_begin(struct SlicksAmigaPlatform *platform,
                               unsigned short view)
{
    struct IntVector *vector;
    if (!platform || platform->active || view >= SLICKS_AMIGA_VIEW_COUNT)
        return -1;
    if (keyboard_begin(platform) != 0)
        return -1;
    platform->publish_valid = 0;

    platform->saved_view = platform->gfx_base->ActiView;
    platform->saved_copper = (unsigned long *)platform->gfx_base->copinit;
    platform->saved_dma = CUSTOM_WORD(0x002);
    platform->saved_interrupts = CUSTOM_WORD(0x01c);
    LoadView(0);
    WaitTOF();
    WaitTOF();

    CUSTOM_WORD(REG_INTENA) = 0x7fff;
    CUSTOM_WORD(REG_INTREQ) = 0x7fff;
    CUSTOM_WORD(REG_DMACON) = DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG;

    platform->vertb_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    platform->vertb_interrupt.is_Node.ln_Pri = 127;
    platform->vertb_interrupt.is_Node.ln_Name = (char *)"Slicks VBI";
    platform->vertb_interrupt.is_Data = 0;
    platform->vertb_interrupt.is_Code =
        (void (*)())vertical_blank_handler;
    vector = &SysBase->IntVects[INTB_VERTB];
    Disable();
    platform->saved_vertb = *vector;
    vector->iv_Data = 0;
    vector->iv_Code = (void (*)())vertical_blank_handler;
    vector->iv_Node = &platform->vertb_interrupt.is_Node;
    Enable();
    platform->vertb_taken = 1;
    active_platform = platform;

    install_copper(view, 1);
    CUSTOM_WORD(REG_DMACON) =
        DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER | DMAF_RASTER;
    CUSTOM_WORD(REG_INTENA) =
        INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_PORTS;
    platform->active = 1;
    Forbid();
    return 0;
}

void slicks_amiga_platform_show(struct SlicksAmigaPlatform *platform,
                               unsigned short view)
{
    if (platform && platform->active && view < SLICKS_AMIGA_VIEW_COUNT)
        install_copper(view, 0);
}

void slicks_amiga_platform_wait_vblank(struct SlicksAmigaPlatform *platform)
{
    unsigned long frame;
    if (!platform || !platform->active)
        return;
    if(platform->io_active) { WaitTOF(); return; }
    frame = platform->vblank_count;
    while (platform->vblank_count == frame)
        ;
}

unsigned long slicks_amiga_platform_raster_time(void *context)
{
    const struct SlicksAmigaPlatform *platform =
        (const struct SlicksAmigaPlatform *)context;
    unsigned long frame, again;
    unsigned short high, line;
    do {
        frame = platform->vblank_count;
        high = (unsigned short)(CUSTOM_WORD(REG_VPOSR) & 7);
        line = (unsigned short)((high << 8) | (CUSTOM_WORD(REG_VHPOSR) >> 8));
        again = platform->vblank_count;
    } while (frame != again ||
             high != (unsigned short)(CUSTOM_WORD(REG_VPOSR) & 7));
    return frame * 313UL + line;
}

unsigned short slicks_amiga_platform_shown_view(void) { return shown_view; }
const unsigned char *slicks_amiga_platform_view_palette(unsigned short view)
{
    return view < SLICKS_AMIGA_VIEW_COUNT && palette_valid[view] ?
        view_palettes[view] : 0;
}

int slicks_amiga_platform_wait_publication(struct SlicksAmigaPlatform *platform)
{
    unsigned long at;
    int late = 0;
    if (!platform || !platform->active)
        return 0;
    at = slicks_amiga_platform_raster_time(platform);
    if (platform->publish_valid && at >= platform->publish_deadline)
        late = 1;
    else {
        slicks_amiga_platform_wait_display_end(platform);
        at = slicks_amiga_platform_raster_time(platform);
    }
    /* First edge (line $100 of a 313-line frame) strictly after now. */
    platform->publish_deadline = (at + 57UL) / 313UL * 313UL + 256UL;
    platform->publish_valid = 1;
    return late;
}

void slicks_amiga_platform_wait_display_blank(
    struct SlicksAmigaPlatform *platform)
{
    unsigned short line;
    if (!platform || !platform->active)
        return;
    /* The playfield is exactly hardware lines $38..$ff. Starting visible
     * bitmap writes at line $100 gives them the lower border plus the next
     * frame's upper border, rather than racing the display beam. */
    do {
        line = (unsigned short)(((CUSTOM_WORD(REG_VPOSR) & 7) << 8) |
                                (CUSTOM_WORD(REG_VHPOSR) >> 8));
    } while (line < 0x100);
}

void slicks_amiga_platform_wait_display_end(
    struct SlicksAmigaPlatform *platform)
{
    if (!platform || !platform->active)
        return;
    /* Edge, not level: if preparation finished during the lower border,
     * that publication opportunity has passed. Wait for a fresh $100 edge
     * so audio/C2P get the complete blanking window, once per refresh. */
    unsigned short line;
    do {
        line = (unsigned short)(((CUSTOM_WORD(REG_VPOSR) & 7) << 8) |
                                (CUSTOM_WORD(REG_VHPOSR) >> 8));
    } while (line >= 0x100);
    slicks_amiga_platform_wait_display_blank(platform);
}

int slicks_amiga_platform_repeat_key(struct SlicksAmigaPlatform *platform,
    unsigned char arg,unsigned short *raw)
{
    if (!platform || !raw || !platform->active || platform->io_active)
        return 0;
    unsigned char latch = platform->key_latch;
    if (!slicks_key_repeat_poll(&key_repeat, latch, platform->bios_ticks, arg))
        return 0;
    *raw = latch;
    return 1;
}

void slicks_amiga_platform_clear_latch(struct SlicksAmigaPlatform *platform)
{
    if (!platform) platform = active_platform;
    if (platform) platform->key_latch = 0x80;
}

int slicks_amiga_platform_poll_key(struct SlicksAmigaPlatform *platform,
                                  unsigned short *raw)
{
    unsigned char tail;
    if (!platform || !raw || platform->key_tail == platform->key_head)
        return 0;
    tail = platform->key_tail;
    unsigned short event=platform->keys[tail];
    *raw = event&255;
    /* A dequeued make is 36ce0's immediate return for a fresh hold. */
    if (!(event & 128)) key_repeat.armed = 1;
    platform->key_shifts=(unsigned char)(event>>8);
    platform->key_tail = (unsigned char)((tail + 1) & 15);
    return 1;
}

int slicks_amiga_platform_left_mouse(void)
{
    return !(CIAA_PRA & 0x40);
}

int slicks_amiga_platform_right_mouse(void)
{
    return !(CUSTOM_WORD(0x016) & 0x0400);
}

int slicks_amiga_platform_joystick(unsigned device,struct SlicksDeviceSample *sample)
{
    if(!sample || device<1 || device>2) return -1;
    unsigned port=device==1?1:0;
    *sample=slicks_decode_amiga_joystick(CUSTOM_WORD(0x00a+2*port),CIAA_PRA,
        CUSTOM_WORD(0x016),port);
    return 0;
}

int slicks_amiga_platform_begin_io(struct SlicksAmigaPlatform *platform)
{
    if(!platform || !platform->active || platform->io_active) return -1;
    slicks_amiga_platform_wait_display_blank(platform);
    keyboard_end(platform);
    Disable();
    CUSTOM_WORD(REG_INTENA)=0x7fff;
    SysBase->IntVects[INTB_VERTB]=platform->saved_vertb;
    platform->vertb_taken=0;active_platform=0;
    /* Restore device service, but retain our raster/copper and no OS sprites.
     * LoadView(NULL) remains in force; do not expose the saved OS view. */
    CUSTOM_WORD(REG_DMACON)=DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG;
    CUSTOM_WORD(REG_DMACON)=DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER | DMAF_RASTER |
        (platform->saved_dma & (DMAF_DISK | DMAF_BLITTER));
    CUSTOM_WORD(REG_INTREQ)=0x7fff;
    CUSTOM_WORD(REG_INTENA)=INTF_SETCLR | (platform->saved_interrupts & 0x7fff);
    platform->io_active=1;
    Enable();Permit();
    return 0;
}

int slicks_amiga_platform_end_io(struct SlicksAmigaPlatform *platform)
{
    if(!platform || !platform->active || !platform->io_active) return -1;
    Forbid();
    slicks_amiga_platform_wait_display_blank(platform);
    Disable();
    CUSTOM_WORD(REG_INTENA)=0x7fff;
    struct IntVector *vector=&SysBase->IntVects[INTB_VERTB];
    vector->iv_Data=0;vector->iv_Code=(void (*)())vertical_blank_handler;
    vector->iv_Node=&platform->vertb_interrupt.is_Node;
    platform->vertb_taken=1;active_platform=platform;
    Enable();
    int result=keyboard_begin(platform);
    CUSTOM_WORD(REG_DMACON)=DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG;
    CUSTOM_WORD(REG_DMACON)=DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER | DMAF_RASTER;
    CUSTOM_WORD(REG_INTREQ)=0x7fff;
    CUSTOM_WORD(REG_INTENA)=INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_PORTS;
    platform->io_active=0;
    return result;
}

void slicks_amiga_platform_end(struct SlicksAmigaPlatform *platform)
{
    struct IntVector *vector;
    if (!platform || !platform->active)
        return;
    if(platform->io_active) (void)slicks_amiga_platform_end_io(platform);

    keyboard_end(platform);
    CUSTOM_WORD(REG_INTENA) = 0x7fff;
    CUSTOM_WORD(REG_INTREQ) = 0x7fff;
    CUSTOM_WORD(REG_DMACON) = DMAF_COPPER | DMAF_RASTER | DMAF_SPRITE;
    install_raw_copper(platform->saved_copper, 1);
    CUSTOM_WORD(REG_BPLCON3) = 0x0c00;
    CUSTOM_WORD(REG_DMACON) = DMAF_SETCLR | DMAF_MASTER | DMAF_COPPER;

    if (platform->vertb_taken) {
        vector = &SysBase->IntVects[INTB_VERTB];
        Disable();
        *vector = platform->saved_vertb;
        Enable();
        platform->vertb_taken = 0;
    }
    active_platform = 0;

    CUSTOM_WORD(REG_DMACON) = DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG;
    CUSTOM_WORD(REG_DMACON) = (unsigned short)(
        DMAF_SETCLR |
        (platform->saved_dma & (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)));
    CUSTOM_WORD(REG_INTREQ) = 0x7fff;
    CUSTOM_WORD(REG_INTENA) = 0x7fff;
    CUSTOM_WORD(REG_INTENA) = (unsigned short)(
        INTF_SETCLR | (platform->saved_interrupts & 0x7fff));
    platform->active = 0;
    Permit();
    LoadView(platform->saved_view);
    WaitTOF();
    WaitTOF();
}

unsigned short slicks_amiga_platform_restore_status(
    const struct SlicksAmigaPlatform *platform)
{
    const struct IntVector *vector;
    unsigned short status = 0;
    if (!platform)
        return 0;
    vector = &SysBase->IntVects[INTB_VERTB];
    if (!platform->active)
        status |= 1;
    if (platform->gfx_base->ActiView == platform->saved_view)
        status |= 2;
    if (vector->iv_Data == platform->saved_vertb.iv_Data &&
        vector->iv_Code == platform->saved_vertb.iv_Code &&
        vector->iv_Node == platform->saved_vertb.iv_Node)
        status |= 4;
    if ((CUSTOM_WORD(0x002) &
         (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)) ==
        (platform->saved_dma &
         (DMAF_ALL | DMAF_MASTER | DMAF_BLITHOG)))
        status |= 8;
    if ((CUSTOM_WORD(0x01c) & 0x7fff) ==
        (platform->saved_interrupts & 0x7fff))
        status |= 16;
    /* COP1LC is write-only. Reaching this point proves that write and the
     * complete restore sequence ran; only readable OS state is compared. */
    return status;
}
