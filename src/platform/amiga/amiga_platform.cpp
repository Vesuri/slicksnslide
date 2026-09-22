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

#define COPPER_LONGS 558
#define BITMAP_BYTES (320UL * 200UL)

static struct SlicksAmigaPlatform *active_platform;
static Bitmap *framework_bitmaps[SLICKS_AMIGA_VIEW_COUNT];
static CopperList *framework_copper[SLICKS_AMIGA_VIEW_COUNT];

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

    for (colour = 0; colour < 256; ++colour) {
        unsigned long r = expand_vga_component(palette[colour * 3]);
        unsigned long g = expand_vga_component(palette[colour * 3 + 1]);
        unsigned long b = expand_vga_component(palette[colour * 3 + 2]);
        colours[colour] = (r << 16) | (g << 8) | b;
    }
    Palette24Bit aga_palette(colours, 256);
    at = list->setPlayfield(1, 320, 200, 8, true, false, false,
                            false, false, true, 0x90);
    list->showBitmap(at, *bitmap);
    at += 16;
    at = list->setPalette24Bit(at, aga_palette, 0, 0, 255, true);
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
    if (active_platform)
        ++active_platform->vblank_count;
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
    next = (unsigned char)((platform->key_head + 1) & 15);
    if (next != platform->key_tail) {
        platform->keys[platform->key_head] = code;
        platform->key_head = next;
    }
    return 0;
}

static int keyboard_begin(struct SlicksAmigaPlatform *platform)
{
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
        unsigned char *data = (unsigned char *)AllocMem(
            BITMAP_BYTES, MEMF_CHIP | MEMF_CLEAR);
        platform->views[view].bitmap = (struct BitMap *)AllocMem(
            sizeof(struct BitMap), MEMF_ANY | MEMF_CLEAR);
        platform->views[view].copper = (unsigned long *)AllocMem(
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
        framework_bitmaps[view] = new Bitmap(data, 320, 200, 8, true);
        framework_copper[view] = new CopperList(
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

int slicks_amiga_platform_set_view(struct SlicksAmigaPlatform *platform,
                                  unsigned short view,
                                  const unsigned char *vga_palette)
{
    if (!platform || view >= SLICKS_AMIGA_VIEW_COUNT || !vga_palette)
        return -1;
    if (build_copper(view, vga_palette) != COPPER_LONGS - 1)
        return -1;
    return validate_framework_view(view);
}

int slicks_amiga_platform_begin(struct SlicksAmigaPlatform *platform,
                               unsigned short view)
{
    struct IntVector *vector;
    if (!platform || platform->active || view >= SLICKS_AMIGA_VIEW_COUNT)
        return -1;
    if (keyboard_begin(platform) != 0)
        return -1;

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
    frame = platform->vblank_count;
    while (platform->vblank_count == frame)
        ;
}

int slicks_amiga_platform_poll_key(struct SlicksAmigaPlatform *platform,
                                  unsigned short *raw)
{
    unsigned char tail;
    if (!platform || !raw || platform->key_tail == platform->key_head)
        return 0;
    tail = platform->key_tail;
    *raw = platform->keys[tail];
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

void slicks_amiga_platform_end(struct SlicksAmigaPlatform *platform)
{
    struct IntVector *vector;
    if (!platform || !platform->active)
        return;

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
