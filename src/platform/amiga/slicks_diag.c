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
volatile unsigned long g_slicks_diag_display_checksum;

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
extern unsigned short slicks_dispatch_title_key(unsigned short scan_code);
extern int slicks_setup_basic_mode(unsigned char *logical,
                                   unsigned short *mode_state);
extern void slicks_convert_to_amiga(const unsigned char *logical,
                                    unsigned char *chunky,
                                    struct BitMap *bitmap);
extern const unsigned char slicks_basic_palette[];

static unsigned short amiga_raw_to_dos_scan(const unsigned short raw)
{
    switch (raw & 0x7fu) {
    case 0x45: return 0x01; /* Escape */
    case 0x44: return 0x1c; /* Return */
    case 0x40: return 0x39; /* Space */
    case 0x50: return 0x3b; /* F1 */
    case 0x58: return 0x43; /* F9 */
    case 0x59: return 0x44; /* F10 */
    default: return 0;
    }
}

static void make_title_surface(unsigned char *planes)
{
    slicks_draw_title_pages(planes);
}

static unsigned long checksum_planes(const unsigned char *planes)
{
    unsigned long checksum = 0x534c4943UL;
    unsigned long i;
    for (i = 0; i < 0x40000UL; ++i)
        checksum = (checksum << 5) ^ (checksum >> 27) ^ planes[i];
    return checksum;
}

static unsigned long checksum_bitmap(const struct BitMap *bitmap)
{
    unsigned long checksum = 0x43325038UL;
    unsigned short plane;
    unsigned short y;
    unsigned short byte_x;

    for (plane = 0; plane < 8; ++plane) {
        const unsigned char *source = bitmap->Planes[plane];
        for (y = 0; y < 200; ++y) {
            for (byte_x = 0; byte_x < 40; ++byte_x) {
                checksum = (checksum << 5) ^ (checksum >> 27) ^
                           source[(unsigned long)y * bitmap->BytesPerRow +
                                  byte_x];
            }
        }
    }
    return checksum;
}

int main(void)
{
    static unsigned long palette[770];
    static unsigned short mode_state[11];
    unsigned char *logical = 0;
    unsigned char *chunky = 0;
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

    logical = (unsigned char *)AllocMem(0x40000UL, MEMF_ANY);
    if (!logical)
        goto cleanup;
    chunky = (unsigned char *)AllocMem(320UL * 200UL, MEMF_ANY);
    if (!chunky)
        goto cleanup;
    if (slicks_setup_basic_mode(logical, mode_state) != 0)
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

    slicks_convert_to_amiga(logical, chunky, screen->RastPort.BitMap);
    g_slicks_diag_display_checksum =
        checksum_bitmap(screen->RastPort.BitMap);
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
            unsigned short scan =
                message_class == IDCMP_MOUSEBUTTONS
                    ? 0x1c
                    : amiga_raw_to_dos_scan(code);
            unsigned short action = slicks_dispatch_title_key(scan);
            if (action == 1 || action == 2) {
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
    if (chunky)
        FreeMem(chunky, 320UL * 200UL);
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
