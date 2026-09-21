#include <exec/execbase.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

struct ExecBase *SysBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

volatile unsigned short g_slicks_diag_ready;
volatile unsigned long g_slicks_diag_checksum;

__attribute__((noinline)) void slicks_diag_frame_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((constructor)) static void initialize_sysbase(void)
{
    struct ExecBase *base;
    __asm volatile("move.l 4.w,%0" : "=a"(base));
    SysBase = base;
}

extern void slicks_draw_title_pages(unsigned char *planes);
extern const unsigned char slicks_basic_palette[];

static unsigned char logical_pixel(const unsigned char *planes,
                                   unsigned short x, unsigned short y)
{
    unsigned long address = ((unsigned long)(x & 3u) << 16) +
                            (unsigned long)y * 100u + (x >> 2);
    return planes[address];
}

static void make_title_surface(unsigned char *planes)
{
    slicks_draw_title_pages(planes);
}

static void convert_to_amiga(const unsigned char *logical,
                             struct BitMap *bitmap)
{
    unsigned short plane;
    unsigned short y;
    unsigned short byte_x;
    for (plane = 0; plane < 8; ++plane) {
        unsigned char *destination = bitmap->Planes[plane];
        for (y = 0; y < 200; ++y) {
            for (byte_x = 0; byte_x < 40; ++byte_x) {
                unsigned char packed = 0;
                unsigned short bit;
                for (bit = 0; bit < 8; ++bit) {
                    unsigned short x = (unsigned short)(byte_x * 8u + bit);
                    unsigned char color = logical_pixel(logical, x, y);
                    packed |= (unsigned char)(((color >> plane) & 1u)
                                              << (7u - bit));
                }
                destination[(unsigned long)y * bitmap->BytesPerRow + byte_x] =
                    packed;
            }
        }
    }
}

static unsigned long checksum_planes(const unsigned char *planes)
{
    unsigned long checksum = 0x534c4943UL;
    unsigned long i;
    for (i = 0; i < 0x40000UL; ++i)
        checksum = (checksum << 5) ^ (checksum >> 27) ^ planes[i];
    return checksum;
}

int main(void)
{
    static unsigned long palette[770];
    unsigned char *logical = 0;
    struct Screen *screen = 0;
    struct Window *window = 0;
    int result = 20;

    GfxBase = (struct GfxBase *)OpenLibrary(
        (CONST_STRPTR)"graphics.library", 39);
    IntuitionBase =
        (struct IntuitionBase *)OpenLibrary(
            (CONST_STRPTR)"intuition.library", 39);
    if (!GfxBase || !IntuitionBase)
        goto cleanup;

    logical = (unsigned char *)AllocMem(0x40000UL, MEMF_ANY | MEMF_CLEAR);
    if (!logical)
        goto cleanup;
    make_title_surface(logical);
    g_slicks_diag_checksum = checksum_planes(logical);

    palette[0] = 256UL << 16;
    for (unsigned short index = 0; index < 768; ++index) {
        unsigned long value = slicks_basic_palette[index];
        unsigned long expanded = (value << 2) | (value >> 4);
        palette[index + 1] = expanded * 0x01010101UL;
    }
    palette[769] = 0;

    screen = OpenScreenTags(
        0, SA_DisplayID, LORES_KEY, SA_Width, 320, SA_Height, 200, SA_Depth, 8,
        SA_Type, CUSTOMSCREEN | SCREENQUIET, SA_ShowTitle, FALSE, SA_Quiet,
        TRUE, TAG_DONE);
    if (!screen)
        goto cleanup;
    LoadRGB32(&screen->ViewPort, palette);

    window = OpenWindowTags(
        0, WA_CustomScreen, (ULONG)screen, WA_Left, 0, WA_Top, 0, WA_Width, 320,
        WA_Height, 200, WA_Backdrop, TRUE, WA_Borderless, TRUE, WA_Activate,
        TRUE, WA_RMBTrap, TRUE, WA_NoCareRefresh, TRUE, WA_IDCMP,
        IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY, TAG_DONE);
    if (!window)
        goto cleanup;

    convert_to_amiga(logical, screen->RastPort.BitMap);
    MakeScreen(screen);
    RethinkDisplay();
    ScreenToFront(screen);
    g_slicks_diag_ready = 1;
    slicks_diag_frame_ready();

    for (;;) {
        struct IntuiMessage *message;
        WaitPort(window->UserPort);
        while ((message =
                    (struct IntuiMessage *)GetMsg(window->UserPort)) != 0) {
            unsigned long message_class = message->Class;
            unsigned short code = message->Code;
            ReplyMsg((struct Message *)message);
            if (message_class == IDCMP_MOUSEBUTTONS ||
                (message_class == IDCMP_RAWKEY && code == 0x45)) {
                result = 0;
                goto cleanup;
            }
        }
    }

cleanup:
    g_slicks_diag_ready = 0;
    if (window)
        CloseWindow(window);
    if (screen)
        CloseScreen(screen);
    if (logical)
        FreeMem(logical, 0x40000UL);
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    IntuitionBase = 0;
    GfxBase = 0;
    return result;
}
