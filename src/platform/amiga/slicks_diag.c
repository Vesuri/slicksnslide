#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "../../game/race_runtime.h"
#include "../../game/track_scene.h"
#include "resource_archive.h"

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

volatile unsigned short g_slicks_diag_ready;
volatile unsigned short g_slicks_diag_ingame;
volatile unsigned short g_slicks_diag_race_error;
volatile unsigned short g_slicks_diag_race_stage;
volatile unsigned long g_slicks_diag_checksum;
volatile unsigned long g_slicks_diag_display_checksum;
volatile unsigned long g_slicks_diag_race_frame;
volatile unsigned long g_slicks_diag_skidmarks;
volatile unsigned long g_slicks_diag_collisions;
volatile unsigned char g_slicks_diag_countdown_stage;
volatile long g_slicks_diag_car_x[SLICKS_RACE_CAR_COUNT];
volatile long g_slicks_diag_car_y[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_timer[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_lap[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_lap_timer[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_waypoint[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_acceleration[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_steering[SLICKS_RACE_CAR_COUNT];
volatile unsigned char *g_slicks_diag_logical;

__attribute__((noinline)) void slicks_diag_frame_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_gameplay_ready(void)
{
    __asm volatile("" ::: "memory");
}

static void race_checkpoint(unsigned short stage)
{
    g_slicks_diag_race_stage = stage;
    slicks_diag_frame_ready();
}

__attribute__((constructor)) static void initialize_sysbase(void)
{
    struct ExecBase *base;
    __asm volatile("move.l 4.w,%0" : "=a"(base));
    SysBase = base;
}

extern void slicks_draw_title_pages(unsigned char *planes,
                                    const unsigned char *frame,
                                    const unsigned char *palette);
extern int slicks_prepare_title_frame(const unsigned char *asset,
                                      unsigned char *frame);
extern unsigned short slicks_dispatch_title_key(unsigned short scan_code);
extern int slicks_setup_basic_mode(unsigned char *logical,
                                   unsigned short *mode_state);
extern void slicks_convert_to_amiga(const unsigned char *logical,
                                    unsigned char *chunky,
                                    struct BitMap *bitmap);
extern void slicks_chunky_to_amiga(const unsigned char *chunky,
                                   struct BitMap *bitmap);

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

static void make_title_surface(unsigned char *planes,
                               const unsigned char *frame,
                               const unsigned char *palette)
{
    slicks_draw_title_pages(planes, frame, palette);
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

static void load_palette(struct Screen *screen, unsigned long *table,
                         const unsigned char *source)
{
    unsigned short index;
    table[0] = 256UL << 16;
    for (index = 0; index < 768; ++index) {
        unsigned long value = source[index];
        unsigned long expanded = (value << 2) | (value >> 4);
        table[index + 1] = expanded * 0x01010101UL;
    }
    table[769] = 0;
    LoadRGB32(&screen->ViewPort, table);
}

static long load_plain_file(const char *path, void *destination,
                            unsigned long capacity)
{
    BPTR file = Open((CONST_STRPTR)path, MODE_OLDFILE);
    LONG size;
    if (!file)
        return -1;
    size = Read(file, destination, (LONG)capacity);
    Close(file);
    return size;
}

static void update_race_diagnostics(const struct SlicksRaceRuntime *race);

static int enter_basic_race(struct Screen *screen, unsigned long *palette,
                            unsigned char *logical, unsigned char *chunky,
                            unsigned short *mode_state,
                            struct SlicksRaceRuntime *race)
{
    struct SlicksResourceArchive archive = {0, 0};
    struct SlicksTrackNavigation *navigation = 0;
    unsigned char *dat = 0;
    unsigned char *track = 0;
    unsigned char *arena = 0;
    unsigned char *car_resource = 0;
    unsigned char *font_resource = 0;
    unsigned char race_palette[768];
    long dat_size;
    long track_size;
    unsigned short car;
    unsigned short direction;
    int result = -1;

    dat = (unsigned char *)AllocMem(65536UL, MEMF_ANY);
    track = (unsigned char *)AllocMem(8192UL, MEMF_ANY);
    arena = (unsigned char *)AllocMem(65536UL, MEMF_ANY);
    navigation = (struct SlicksTrackNavigation *)
        AllocMem(sizeof(*navigation), MEMF_ANY);
    car_resource = (unsigned char *)AllocMem(128UL, MEMF_ANY);
    font_resource = (unsigned char *)AllocMem(2048UL, MEMF_ANY);
    if (!dat || !track || !arena || !navigation || !car_resource ||
        !font_resource) {
        g_slicks_diag_race_error = 1;
        goto cleanup;
    }
    race_checkpoint(1);
    dat_size = load_plain_file("SLICKS.DAT", dat, 65536UL);
    track_size = load_plain_file("TRACKS/BASIC.SS", track, 8192UL);
    if (dat_size <= 0 || track_size <= 0) {
        g_slicks_diag_race_error = 2;
        goto cleanup;
    }
    race_checkpoint(2);
    if (slicks_resource_archive_open(&archive, "SLICKS.000") != 0 ||
        slicks_resource_archive_load(&archive, "peli.@p", race_palette,
                                     sizeof(race_palette)) != 768L) {
        g_slicks_diag_race_error = 3;
        goto cleanup;
    }
    race_checkpoint(3);
    if (slicks_setup_basic_mode(logical, mode_state) != 0) {
        g_slicks_diag_race_error = 4;
        goto cleanup;
    }
    race_checkpoint(4);
    if (slicks_build_track_scene(logical, dat, (unsigned long)dat_size, track,
                                 (unsigned long)track_size, arena, 65536UL,
                                 navigation) != 233) {
        g_slicks_diag_race_error = 5;
        goto cleanup;
    }
    race_checkpoint(5);

    slicks_race_initialize(race, navigation);
    {
        long font_size = slicks_resource_archive_load(
            &archive, "pieni.@f", font_resource, 2048UL);
        if (font_size <= 0 ||
            slicks_race_add_font(race, font_resource,
                                 (unsigned long)font_size) != 0) {
            g_slicks_diag_race_error = 6;
            goto cleanup;
        }
    }
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        char property_name[11] = "auto00.omi";
        long property_size;
        property_name[5] = (char)('0' + car);
        property_size = slicks_resource_archive_load(
            &archive, property_name, car_resource, 128UL);
        if (property_size != SLICKS_CAR_PROPERTY_SIZE ||
            slicks_race_add_car_properties(
                race, car, car_resource, (unsigned long)property_size) != 0) {
            g_slicks_diag_race_error = 6;
            goto cleanup;
        }
        for (direction = 0; direction < SLICKS_CAR_BASE_DIRECTIONS;
             ++direction) {
            char name[11] = "auto00.000";
            long car_size;
            name[5] = (char)('0' + car);
            name[9] = (char)('0' + direction);
            car_size = slicks_resource_archive_load(
                &archive, name, car_resource, 128UL);
            if (car_size <= 0 ||
                slicks_race_add_car_sprite(race, car, direction,
                                           car_resource,
                                           (unsigned long)car_size) != 0) {
                g_slicks_diag_race_error = 6;
                goto cleanup;
            }
        }
    }
    race_checkpoint(6);
    if (slicks_race_start(race, logical, chunky) != 0) {
        g_slicks_diag_race_error = 7;
        goto cleanup;
    }
    race_checkpoint(7);

    load_palette(screen, palette, race_palette);
    race_checkpoint(8);
    slicks_convert_to_amiga(logical, chunky, screen->RastPort.BitMap);
    race_checkpoint(9);
    g_slicks_diag_checksum = checksum_planes(logical);
    g_slicks_diag_display_checksum =
        checksum_bitmap(screen->RastPort.BitMap);
    MakeScreen(screen);
    RethinkDisplay();
    ScreenToFront(screen);
    g_slicks_diag_ingame = 1;
    update_race_diagnostics(race);
    slicks_diag_frame_ready();
    result = 0;

cleanup:
    slicks_resource_archive_close(&archive);
    if (font_resource)
        FreeMem(font_resource, 2048UL);
    if (car_resource)
        FreeMem(car_resource, 128UL);
    if (navigation)
        FreeMem(navigation, sizeof(*navigation));
    if (arena)
        FreeMem(arena, 65536UL);
    if (track)
        FreeMem(track, 8192UL);
    if (dat)
        FreeMem(dat, 65536UL);
    if (result != 0)
        slicks_diag_frame_ready();
    return result;
}

static void update_race_diagnostics(const struct SlicksRaceRuntime *race)
{
    unsigned short car;
    g_slicks_diag_race_frame = race->frame_count;
    g_slicks_diag_skidmarks = race->skidmark_count;
    g_slicks_diag_collisions = race->collision_count;
    g_slicks_diag_countdown_stage = race->countdown_stage;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        g_slicks_diag_car_x[car] = race->cars[car].x;
        g_slicks_diag_car_y[car] = race->cars[car].y;
        g_slicks_diag_timer[car] = race->cars[car].elapsed_centiseconds;
        g_slicks_diag_lap[car] = race->cars[car].lap;
        g_slicks_diag_lap_timer[car] =
            race->cars[car].current_lap_centiseconds;
        g_slicks_diag_waypoint[car] = race->cars[car].waypoint;
        g_slicks_diag_acceleration[car] =
            race->properties[car].acceleration;
        g_slicks_diag_steering[car] = race->properties[car].steering;
    }
}

static int update_race_key(struct SlicksRaceRuntime *race,
                           unsigned short raw)
{
    unsigned char control;
    unsigned char controls = race->controls;
    unsigned char pressed = !(raw & 0x80);
    switch (raw & 0x7f) {
    case 0x4c: control = SLICKS_CONTROL_ACCELERATE; break;
    case 0x4d: control = SLICKS_CONTROL_BRAKE; break;
    case 0x4e: control = SLICKS_CONTROL_RIGHT; break;
    case 0x4f: control = SLICKS_CONTROL_LEFT; break;
    default: return 0;
    }
    if (pressed)
        controls |= control;
    else
        controls &= (unsigned char)~control;
    slicks_race_set_controls(race, controls, 1);
    return 1;
}

int main(int argc, char **argv)
{
    static unsigned long palette[770];
    static unsigned char source_palette[768];
    static unsigned short mode_state[11];
    struct SlicksResourceArchive archive = {0, 0};
    unsigned char *logical = 0;
    unsigned char *chunky = 0;
    unsigned char *title_asset = 0;
    unsigned char *title_frame = 0;
    struct SlicksRaceRuntime *race = 0;
    struct Screen *screen = 0;
    struct Window *window = 0;
    int result = 20;

    DOSBase = (struct DosLibrary *)OpenLibrary(
        (CONST_STRPTR)"dos.library", 37);
    GfxBase = (struct GfxBase *)OpenLibrary(
        (CONST_STRPTR)"graphics.library", 39);
    IntuitionBase =
        (struct IntuitionBase *)OpenLibrary(
            (CONST_STRPTR)"intuition.library", 39);
    if (!DOSBase || !GfxBase || !IntuitionBase)
        goto cleanup;

    if (slicks_resource_archive_open(&archive, "SLICKS.000") != 0)
        goto cleanup;
    title_asset = (unsigned char *)AllocMem(64003UL, MEMF_ANY);
    title_frame = (unsigned char *)AllocMem(64002UL, MEMF_ANY);
    if (!title_asset || !title_frame)
        goto cleanup;
    if (slicks_resource_archive_load(&archive, "mainmenu.@I", title_asset,
                                     64003UL) != 64003L ||
        slicks_resource_archive_load(&archive, "partII", source_palette,
                                     sizeof(source_palette)) != 768L ||
        slicks_prepare_title_frame(title_asset, title_frame) != 0)
        goto cleanup;

    logical = (unsigned char *)AllocMem(0x40000UL, MEMF_ANY);
    if (!logical)
        goto cleanup;
    g_slicks_diag_logical = logical;
    chunky = (unsigned char *)AllocMem(320UL * 200UL, MEMF_ANY);
    if (!chunky)
        goto cleanup;
    race = (struct SlicksRaceRuntime *)AllocMem(sizeof(*race), MEMF_ANY);
    if (!race)
        goto cleanup;
    if (slicks_setup_basic_mode(logical, mode_state) != 0)
        goto cleanup;
    make_title_surface(logical, title_frame, source_palette);
    g_slicks_diag_checksum = checksum_planes(logical);
    FreeMem(title_frame, 64002UL);
    title_frame = 0;
    FreeMem(title_asset, 64003UL);
    title_asset = 0;
    slicks_resource_archive_close(&archive);

    screen = OpenScreenTags(
        0, SA_DisplayID, LORES_KEY, SA_Width, 320, SA_Height, 200, SA_Depth, 8,
        SA_Type, CUSTOMSCREEN | SCREENQUIET, SA_ShowTitle, FALSE, SA_Quiet,
        TRUE, TAG_DONE);
    if (!screen)
        goto cleanup;
    load_palette(screen, palette, source_palette);

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

    (void)argv;
    if (argc > 1 &&
        enter_basic_race(screen, palette, logical, chunky, mode_state,
                         race) != 0)
        goto cleanup;

    for (;;) {
        struct IntuiMessage *message;
        if (g_slicks_diag_ingame)
            WaitTOF();
        else
            WaitPort(window->UserPort);
        while ((message =
                    (struct IntuiMessage *)GetMsg(window->UserPort)) != 0) {
            unsigned long message_class = message->Class;
            unsigned short code = message->Code;
            ReplyMsg((struct Message *)message);
            if (g_slicks_diag_ingame) {
                if (message_class == IDCMP_RAWKEY &&
                    (code & 0x7f) == 0x45) {
                    result = 0;
                    goto cleanup;
                }
                if (message_class == IDCMP_RAWKEY)
                    (void)update_race_key(race, code);
                continue;
            }
            unsigned short scan =
                message_class == IDCMP_MOUSEBUTTONS
                    ? 0x1c
                    : amiga_raw_to_dos_scan(code);
            unsigned short action = slicks_dispatch_title_key(scan);
            if (action == 1) {
                result = 0;
                goto cleanup;
            }
            if (action == 2 && !g_slicks_diag_ingame &&
                enter_basic_race(screen, palette, logical, chunky,
                                 mode_state, race) != 0)
                goto cleanup;
        }
        if (g_slicks_diag_ingame) {
            slicks_race_step(race, logical);
            slicks_chunky_to_amiga(chunky, screen->RastPort.BitMap);
            update_race_diagnostics(race);
            if (race->frame_count == 200) {
                g_slicks_diag_checksum = checksum_planes(logical);
                g_slicks_diag_display_checksum =
                    checksum_bitmap(screen->RastPort.BitMap);
                slicks_diag_gameplay_ready();
            }
            slicks_diag_frame_ready();
        }
    }

cleanup:
    g_slicks_diag_ready = 0;
    g_slicks_diag_ingame = 0;
    slicks_resource_archive_close(&archive);
    if (window)
        CloseWindow(window);
    if (screen)
        CloseScreen(screen);
    if (chunky)
        FreeMem(chunky, 320UL * 200UL);
    if (race)
        FreeMem(race, sizeof(*race));
    if (logical)
        FreeMem(logical, 0x40000UL);
    g_slicks_diag_logical = 0;
    if (title_frame)
        FreeMem(title_frame, 64002UL);
    if (title_asset)
        FreeMem(title_asset, 64003UL);
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    IntuitionBase = 0;
    GfxBase = 0;
    DOSBase = 0;
    return result;
}
