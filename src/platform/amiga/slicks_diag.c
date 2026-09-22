#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "../../game/race_runtime.h"
#include "../../game/track_scene.h"
#include "amiga_platform.h"
#include "amiga_audio.h"
#include "resource_archive.h"

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

volatile unsigned short g_slicks_diag_ready;
volatile unsigned short g_slicks_diag_ingame;
volatile unsigned short g_slicks_diag_race_error;
volatile unsigned short g_slicks_diag_race_stage;
volatile unsigned long g_slicks_diag_checksum;
volatile unsigned long g_slicks_diag_display_checksum;
volatile unsigned long g_slicks_diag_race_frame;
volatile unsigned long g_slicks_diag_skidmarks;
volatile unsigned long g_slicks_diag_collisions;
volatile unsigned long g_slicks_diag_track_collisions;
volatile unsigned char g_slicks_diag_countdown_stage;
volatile unsigned char g_slicks_diag_start_light_visible;
volatile unsigned char g_slicks_diag_start_light_stage_mask;
volatile unsigned short g_slicks_diag_dirty_ranges;
volatile unsigned short g_slicks_diag_dirty_rows;
volatile unsigned long g_slicks_diag_dirty_c2p_calls;
volatile unsigned long g_slicks_diag_dirty_c2p_rows;
volatile unsigned short g_slicks_diag_restore_status;
volatile unsigned short g_slicks_diag_force_exit;
volatile unsigned short g_slicks_diag_track_zones;
volatile unsigned long g_slicks_diag_material_checksum;
volatile unsigned long g_slicks_diag_surface_checksum;
volatile long g_slicks_diag_car_x[SLICKS_RACE_CAR_COUNT];
volatile long g_slicks_diag_car_y[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_timer[SLICKS_RACE_CAR_COUNT];
volatile short g_slicks_diag_speed[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_material[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_lap[SLICKS_RACE_CAR_COUNT];
volatile unsigned short g_slicks_diag_lap_timer[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_waypoint[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_finished[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_finish_position[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_race_complete;
volatile unsigned char g_slicks_diag_results_drawn;
volatile unsigned char g_slicks_diag_audio_ready;
volatile unsigned char g_slicks_diag_engine_started;
volatile unsigned char g_slicks_diag_music_started;
volatile unsigned char g_slicks_diag_acceleration[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_steering[SLICKS_RACE_CAR_COUNT];
volatile unsigned char *g_slicks_diag_logical;
volatile unsigned long g_slicks_diag_target_frame = 200;

__attribute__((noinline)) void slicks_diag_frame_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_gameplay_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_system_restored(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_results_ready(void)
{
    __asm volatile("" ::: "memory");
}

static void race_checkpoint(unsigned short stage)
{
    g_slicks_diag_race_stage = stage;
    if (g_slicks_diag_ready)
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
extern void slicks_draw_title_menu_selection(unsigned char *planes,
                                             const unsigned char *palette,
                                             unsigned short selection);
extern void slicks_draw_title_text(unsigned char *planes, const char *text,
                                  unsigned short x, unsigned short y,
                                  unsigned short colour);
extern int slicks_prepare_title_frame(const unsigned char *asset,
                                      unsigned char *frame);
extern unsigned short slicks_dispatch_title_key(unsigned short scan_code);
extern int slicks_setup_basic_mode(unsigned char *logical,
                                   unsigned short *mode_state);
extern void slicks_convert_to_amiga(const unsigned char *logical,
                                    unsigned char *chunky,
                                    struct BitMap *bitmap);
extern void slicks_chunky_rows_to_amiga(const unsigned char *chunky,
                                        struct BitMap *bitmap,
                                        unsigned long top,
                                        unsigned long bottom);

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

static void clear_title_rectangle(unsigned char *logical,
                                  unsigned short left, unsigned short top,
                                  unsigned short right, unsigned short bottom)
{
    unsigned short x;
    unsigned short y;
    for (y = top; y < bottom; ++y)
        for (x = left; x < right; ++x)
            logical[(unsigned long)y * 100UL + (x >> 2) +
                    (unsigned long)(x & 3) * 65536UL] = 0;
}

static void redraw_title_configuration(
    struct SlicksAmigaPlatform *platform, unsigned char *logical,
    unsigned char *chunky, const unsigned char *palette,
    unsigned short selection, unsigned short vehicle,
    unsigned short track, unsigned short laps)
{
    char vehicle_text[] = "CAR AUTO 01";
    char laps_text[] = "LAPS 4";
    const char *track_text = track ? "TRACK BASICTRK" : "TRACK BASIC";
    vehicle_text[9] = (char)('0' + (vehicle + 1) / 10);
    vehicle_text[10] = (char)('0' + (vehicle + 1) % 10);
    laps_text[5] = (char)('0' + laps);
    slicks_draw_title_menu_selection(logical, palette, selection);
    clear_title_rectangle(logical, 105, 164, 235, 191);
    slicks_draw_title_text(logical, vehicle_text, 160, 166, 15);
    slicks_draw_title_text(logical, track_text, 160, 174, 15);
    slicks_draw_title_text(logical, laps_text, 160, 182, 15);
    slicks_convert_to_amiga(logical, chunky, platform->views[0].bitmap);
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

static unsigned long checksum_material_map(const unsigned char *material_map)
{
    unsigned long checksum = 0x4d41544cUL;
    unsigned long at;
    for (at = 0; at < SLICKS_TRACK_MATERIAL_SIZE; ++at)
        checksum = (checksum << 5) ^ (checksum >> 27) ^ material_map[at];
    return checksum;
}

static unsigned long checksum_surface_map(const unsigned char *surface_map)
{
    unsigned long checksum = 0x53555246UL;
    unsigned long at;
    for (at = 0; at < SLICKS_TRACK_MATERIAL_SIZE; ++at)
        checksum = (checksum << 5) ^ (checksum >> 27) ^ surface_map[at];
    return checksum;
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

static void prepare_race_palette(unsigned char *palette)
{
    static const unsigned char car_ramps[4][2][3] = {
        {{32, 0, 0}, {63, 45, 0}},
        {{20, 8, 45}, {40, 48, 60}},
        {{20, 8, 45}, {40, 48, 60}},
        {{20, 8, 45}, {40, 48, 60}}
    };
    static const unsigned char yellow_ramp[5][3] = {
        {63, 61, 1}, {63, 59, 11}, {63, 57, 21},
        {63, 55, 31}, {63, 53, 41}
    };
    unsigned short car;
    unsigned short shade;

    /* The DOS race setup rewrites four five-shade car slots after loading
     * peli.@p.  Recreate the interpolation rather than displaying the raw
     * resource palette, whose first twenty entries are placeholders. */
    for (car = 0; car < 4; ++car) {
        for (shade = 0; shade < 5; ++shade) {
            unsigned short colour;
            unsigned short index = 1 + car * 5 + shade;
            for (colour = 0; colour < 3; ++colour) {
                unsigned short first = car_ramps[car][0][colour];
                unsigned short last = car_ramps[car][1][colour];
                palette[index * 3 + colour] =
                    (unsigned char)(first +
                        ((long)(last - first) * shade) / 4L);
            }
        }
    }
    palette[183 * 3] = 39;
    palette[183 * 3 + 1] = 43;
    palette[183 * 3 + 2] = 10;
    for (shade = 0; shade < 5; ++shade) {
        palette[(199 + shade) * 3] = yellow_ramp[shade][0];
        palette[(199 + shade) * 3 + 1] = yellow_ramp[shade][1];
        palette[(199 + shade) * 3 + 2] = yellow_ramp[shade][2];
    }
}

static int prepare_race(struct SlicksAmigaPlatform *platform,
                      unsigned char *logical, unsigned char *chunky,
                      unsigned short *mode_state,
                      struct SlicksRaceRuntime *race,
                      const char *track_path,
                      unsigned char *race_palette)
{
    struct SlicksResourceArchive archive = {0, 0, 0};
    struct SlicksTrackNavigation *navigation = 0;
    unsigned char *dat = 0;
    unsigned char *track = 0;
    unsigned char *arena = 0;
    unsigned char *car_resource = 0;
    unsigned char *font_resource = 0;
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
    {
        unsigned long at;
        for (at = 0; at < sizeof(*navigation); ++at)
            ((unsigned char *)navigation)[at] = 0;
    }
    slicks_race_initialize(race, navigation);
    race_checkpoint(1);
    dat_size = load_plain_file("SLICKS.DAT", dat, 65536UL);
    track_size = load_plain_file(track_path, track, 8192UL);
    if (dat_size <= 0 || track_size <= 0) {
        g_slicks_diag_race_error = 2;
        goto cleanup;
    }
    race_checkpoint(2);
    if (slicks_resource_archive_open(&archive, "SLICKS.000") != 0 ||
        slicks_resource_archive_load(&archive, "peli.@p", race_palette,
                                     768UL) != 768L) {
        g_slicks_diag_race_error = 3;
        goto cleanup;
    }
    prepare_race_palette(race_palette);
    race_checkpoint(3);
    if (slicks_setup_basic_mode(logical, mode_state) != 0) {
        g_slicks_diag_race_error = 4;
        goto cleanup;
    }
    race_checkpoint(4);
    if (slicks_build_track_scene(logical, race->material_map,
                                 race->surface_map, dat,
                                 (unsigned long)dat_size, track,
                                 (unsigned long)track_size, arena, 65536UL,
                                 navigation) <= 0) {
        g_slicks_diag_race_error = 5;
        goto cleanup;
    }
    race_checkpoint(5);

    race->navigation = *navigation;
    g_slicks_diag_track_zones = navigation->zone_count;
    g_slicks_diag_material_checksum =
        checksum_material_map(race->material_map);
    g_slicks_diag_surface_checksum =
        checksum_surface_map(race->surface_map);
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
    for (car = 0; car < SLICKS_START_LIGHT_COUNT; ++car) {
        char name[10] = "lahto1.@I";
        long light_size;
        name[5] = (char)('1' + car);
        light_size = slicks_resource_archive_load(
            &archive, name, font_resource, 2048UL);
        if (light_size <= 0 ||
            slicks_race_add_start_light(race, car, font_resource,
                                        (unsigned long)light_size) != 0) {
            g_slicks_diag_race_error = 6;
            goto cleanup;
        }
    }
    for (car = 0; car < SLICKS_TRAIL_SPRITE_COUNT; ++car) {
        char name[7] = "savu.1";
        long trail_size;
        name[5] = (char)('1' + car);
        trail_size = slicks_resource_archive_load(
            &archive, name, car_resource, 128UL);
        if (trail_size <= 0 ||
            slicks_race_add_trail_sprite(race, car, car_resource,
                                         (unsigned long)trail_size) != 0) {
            g_slicks_diag_race_error = 6;
            goto cleanup;
        }
    }
    for (car = 0; car < SLICKS_VEHICLE_COUNT; ++car) {
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
            if (car == 9 && direction == 0) {
                name[0] = 'c'; name[1] = 'a'; name[2] = 'r'; name[3] = '9';
                name[4] = 0;
            }
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

    if (slicks_amiga_platform_set_view(platform, 1, race_palette) != 0) {
        g_slicks_diag_race_error = 8;
        goto cleanup;
    }
    race_checkpoint(8);
    slicks_convert_to_amiga(logical, chunky, platform->views[1].bitmap);
    slicks_race_clear_dirty_rows(race);
    race_checkpoint(9);
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

static void enter_prepared_race(struct SlicksAmigaPlatform *platform,
                                unsigned char *logical,
                                struct SlicksRaceRuntime *race)
{
    g_slicks_diag_logical = logical;
    g_slicks_diag_checksum = checksum_planes(logical);
    g_slicks_diag_display_checksum =
        checksum_bitmap(platform->views[1].bitmap);
    slicks_amiga_platform_show(platform, 1);
    g_slicks_diag_ingame = 1;
    update_race_diagnostics(race);
    slicks_diag_frame_ready();
}

static void update_race_diagnostics(const struct SlicksRaceRuntime *race)
{
    unsigned short car;
    g_slicks_diag_race_frame = race->frame_count;
    g_slicks_diag_skidmarks = race->skidmark_count;
    g_slicks_diag_collisions = race->collision_count;
    g_slicks_diag_track_collisions = race->track_collision_count;
    g_slicks_diag_countdown_stage = race->countdown_stage;
    g_slicks_diag_start_light_visible = race->start_light_visible;
    g_slicks_diag_start_light_stage_mask = race->start_light_stage_mask;
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        g_slicks_diag_car_x[car] = race->cars[car].x;
        g_slicks_diag_car_y[car] = race->cars[car].y;
        g_slicks_diag_timer[car] = race->cars[car].elapsed_centiseconds;
        g_slicks_diag_speed[car] = race->cars[car].speed;
        {
            long x = race->cars[car].x / 100;
            long y = race->cars[car].y / 100;
            g_slicks_diag_material[car] =
                x >= 0 && x < 320 && y >= 0 && y < 190
                    ? race->material_map[(unsigned long)y * 320UL +
                                         (unsigned long)x]
                    : 31;
        }
        g_slicks_diag_lap[car] = race->cars[car].lap;
        g_slicks_diag_lap_timer[car] =
            race->cars[car].current_lap_centiseconds;
        g_slicks_diag_waypoint[car] = race->cars[car].waypoint;
        g_slicks_diag_finished[car] = race->cars[car].finished;
        g_slicks_diag_finish_position[car] =
            race->cars[car].finish_position;
        g_slicks_diag_acceleration[car] =
            race->properties[race->cars[car].vehicle].property_4;
        g_slicks_diag_steering[car] =
            race->properties[race->cars[car].vehicle].property_6;
    }
    g_slicks_diag_race_complete = race->race_complete;
    g_slicks_diag_results_drawn = race->results_drawn;
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
    static unsigned char source_palette[768];
    static unsigned char race_palette[768];
    static unsigned short mode_state[11];
    struct SlicksResourceArchive archive = {0, 0, 0};
    struct SlicksAmigaPlatform platform = {0};
    struct SlicksAmigaAudio audio = {0};
    unsigned char *logical = 0;
    unsigned char *chunky = 0;
    unsigned char *title_asset = 0;
    unsigned char *title_frame = 0;
    struct SlicksRaceRuntime *race = 0;
    unsigned char *sample_resource = 0;
    unsigned long title_checksum = 0;
    unsigned long title_display_checksum = 0;
    unsigned char left_was_down = 0;
    unsigned char auto_race;
    unsigned char restore_test;
    unsigned char race_prepared = 0;
    unsigned short menu_selection = 0;
    unsigned short selected_vehicle = 5;
    unsigned short selected_track = 0;
    unsigned short selected_laps = 4;
    const char *track_path;
    int result = 20;

    DOSBase = (struct DosLibrary *)OpenLibrary(
        (CONST_STRPTR)"dos.library", 37);
    GfxBase = (struct GfxBase *)OpenLibrary(
        (CONST_STRPTR)"graphics.library", 39);
    if (!DOSBase || !GfxBase)
        goto cleanup;
    if (slicks_amiga_platform_create(&platform, GfxBase) != 0)
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
    redraw_title_configuration(&platform, logical, chunky, source_palette,
                               menu_selection, selected_vehicle,
                               selected_track, selected_laps);
    title_checksum = checksum_planes(logical);
    slicks_convert_to_amiga(logical, chunky, platform.views[0].bitmap);
    if (slicks_amiga_platform_set_view(&platform, 0, source_palette) != 0)
        goto cleanup;
    title_display_checksum = checksum_bitmap(platform.views[0].bitmap);
    sample_resource = (unsigned char *)AllocMem(131691UL, MEMF_ANY);
    if (!sample_resource ||
        slicks_resource_archive_load(&archive, "samples.dat", sample_resource,
                                     131691UL) != 131691L ||
        slicks_amiga_audio_create(&audio, sample_resource, 131691UL) != 0 ||
        slicks_resource_archive_load(&archive, "intermed.wav", sample_resource,
                                     131691UL) != 18852L ||
        slicks_amiga_audio_add_music(&audio, sample_resource, 18852UL) != 0)
        goto cleanup;
    g_slicks_diag_audio_ready = audio.ready;
    FreeMem(sample_resource, 131691UL);
    sample_resource = 0;
    slicks_resource_archive_close(&archive);

    /* The no-stdlib Amiga entry passes the CLI byte count in d0 and its raw
     * argument string in a0, rather than constructing a Unix argv array. */
    restore_test = (unsigned char)(
        argc > 0 && ((const char *)argv)[0] == 'E');
    auto_race = (unsigned char)(argc > 1 && !restore_test);
    if (argc > 0 && ((const char *)argv)[0] == 'L')
        g_slicks_diag_target_frame = 700;
    if (argc > 0 && ((const char *)argv)[0] == 'R')
        g_slicks_diag_target_frame = 1800;
    track_path = argc > 0 && ((const char *)argv)[0] == 'T'
                     ? "TRACKS/BASICTRK.SS"
                     : "TRACKS/BASIC.SS";

    /* Automated gates prepare before takeover. Interactive play deliberately
     * waits until GO so the native menu can choose the track, car and laps;
     * its loader temporarily runs with AmigaOS restored. */
    if (auto_race) {
        if (prepare_race(&platform, logical, chunky, mode_state, race,
                         track_path, race_palette) != 0)
            goto cleanup;
        race_prepared = 1;
        if (argc > 0 && ((const char *)argv)[0] == 'R')
            slicks_race_set_laps(race, 1);
    }

    g_slicks_diag_checksum = title_checksum;
    g_slicks_diag_display_checksum = title_display_checksum;
    if (slicks_amiga_platform_begin(&platform, 0) != 0)
        goto cleanup;
    g_slicks_diag_ready = 1;
    slicks_diag_frame_ready();
    if (restore_test || g_slicks_diag_force_exit) {
        result = 0;
        goto cleanup;
    }

    if (auto_race) {
        enter_prepared_race(&platform, logical, race);
        slicks_amiga_audio_start_engine(
            &audio, race->cars[0].vehicle,
            race->properties[race->cars[0].vehicle].engine_volume);
    }

    for (;;) {
        unsigned short code;
        unsigned char left_down;
        slicks_amiga_platform_wait_vblank(&platform);
        if (g_slicks_diag_force_exit) {
            result = 0;
            goto cleanup;
        }
        if (slicks_amiga_platform_right_mouse()) {
            result = 0;
            goto cleanup;
        }
        while (slicks_amiga_platform_poll_key(&platform, &code)) {
            if (g_slicks_diag_ingame) {
                if (!(code & 0x80) &&
                    ((code & 0x7f) == 0x45 ||
                     (race->race_complete && (code & 0x7f) == 0x44))) {
                    slicks_amiga_audio_stop(&audio);
                    if (slicks_setup_basic_mode(logical, mode_state) != 0)
                        goto cleanup;
                    make_title_surface(logical, title_frame, source_palette);
                    redraw_title_configuration(
                        &platform, logical, chunky, source_palette,
                        menu_selection, selected_vehicle, selected_track,
                        selected_laps);
                    slicks_amiga_platform_show(&platform, 0);
                    g_slicks_diag_ingame = 0;
                    race_prepared = 0;
                    continue;
                }
                (void)update_race_key(race, code);
                continue;
            }
            {
                unsigned char pressed = (unsigned char)!(code & 0x80);
                unsigned short raw = code & 0x7f;
                unsigned char redraw = 0;
                if (!pressed)
                    continue;
                if (raw == 0x4c) {
                    menu_selection = (unsigned short)(
                        menu_selection ? menu_selection - 1 : 5);
                    redraw = 1;
                } else if (raw == 0x4d) {
                    menu_selection = (unsigned short)(
                        menu_selection < 5 ? menu_selection + 1 : 0);
                    redraw = 1;
                } else if (raw == 0x4f || raw == 0x4e) {
                    int delta = raw == 0x4e ? 1 : -1;
                    if (menu_selection == 1) {
                        selected_vehicle = (unsigned short)(
                            (selected_vehicle + SLICKS_VEHICLE_COUNT + delta) %
                            SLICKS_VEHICLE_COUNT);
                        redraw = 1;
                    } else if (menu_selection == 2) {
                        selected_track ^= 1;
                        redraw = 1;
                    } else if (menu_selection == 3) {
                        selected_laps = (unsigned short)(
                            selected_laps + delta);
                        if (selected_laps < 1)
                            selected_laps = 9;
                        if (selected_laps > 9)
                            selected_laps = 1;
                        redraw = 1;
                    }
                }
                if (redraw) {
                    redraw_title_configuration(
                        &platform, logical, chunky, source_palette,
                        menu_selection, selected_vehicle, selected_track,
                        selected_laps);
                    continue;
                }
                unsigned short action = slicks_dispatch_title_key(
                    amiga_raw_to_dos_scan(code));
                if (action == 1) {
                    result = 0;
                    goto cleanup;
                }
                if (action == 2 && menu_selection == 5) {
                    result = 0;
                    goto cleanup;
                }
                if (action == 2 && menu_selection == 0) {
                    g_slicks_diag_ready = 0;
                    slicks_amiga_platform_end(&platform);
                    track_path = selected_track ? "TRACKS/BASICTRK.SS" :
                                                  "TRACKS/BASIC.SS";
                    if (prepare_race(&platform, logical, chunky, mode_state,
                                     race, track_path, race_palette) != 0)
                        goto cleanup;
                    race_prepared = 1;
                    slicks_race_set_vehicle(race, 0, selected_vehicle);
                    slicks_race_set_laps(race, selected_laps);
                    if (slicks_amiga_platform_begin(&platform, 1) != 0)
                        goto cleanup;
                    g_slicks_diag_ready = 1;
                    enter_prepared_race(&platform, logical, race);
                    slicks_amiga_audio_start_engine(
                        &audio, race->cars[0].vehicle,
                        race->properties[race->cars[0].vehicle].engine_volume);
                }
            }
        }
        left_down = (unsigned char)slicks_amiga_platform_left_mouse();
        if (!g_slicks_diag_ingame && left_down && !left_was_down) {
            unsigned short action = slicks_dispatch_title_key(0x1c);
            if (action == 1) {
                result = 0;
                goto cleanup;
            }
            if (action == 2 && menu_selection == 0) {
                if (!race_prepared) {
                    g_slicks_diag_ready = 0;
                    slicks_amiga_platform_end(&platform);
                    track_path = selected_track ? "TRACKS/BASICTRK.SS" :
                                                  "TRACKS/BASIC.SS";
                    if (prepare_race(&platform, logical, chunky, mode_state,
                                     race, track_path, race_palette) != 0)
                        goto cleanup;
                    race_prepared = 1;
                    slicks_race_set_vehicle(race, 0, selected_vehicle);
                    slicks_race_set_laps(race, selected_laps);
                    if (slicks_amiga_platform_begin(&platform, 1) != 0)
                        goto cleanup;
                    g_slicks_diag_ready = 1;
                }
                enter_prepared_race(&platform, logical, race);
                slicks_amiga_audio_start_engine(
                    &audio, race->cars[0].vehicle,
                    race->properties[race->cars[0].vehicle].engine_volume);
            }
        }
        left_was_down = left_down;
        if (g_slicks_diag_ingame) {
            unsigned short dirty;
            unsigned char completed_now = 0;
            slicks_race_step(race, logical);
            if (race->race_complete && audio.engine_started) {
                slicks_amiga_audio_stop(&audio);
                slicks_amiga_audio_start_music(&audio);
                completed_now = 1;
            }
            slicks_amiga_audio_update(
                &audio, race->cars[0].speed, race->collision_count,
                race->skidmark_count);
            g_slicks_diag_engine_started = audio.engine_started;
            g_slicks_diag_music_started = audio.music_started;
            g_slicks_diag_dirty_ranges = race->dirty_row_count;
            g_slicks_diag_dirty_rows = 0;
            for (dirty = 0; dirty < race->dirty_row_count; ++dirty) {
                const struct SlicksDirtyRows *rows = &race->dirty_rows[dirty];
                slicks_chunky_rows_to_amiga(
                    chunky, platform.views[1].bitmap,
                    rows->top, rows->bottom);
                g_slicks_diag_dirty_rows += rows->bottom - rows->top;
                g_slicks_diag_dirty_c2p_rows += rows->bottom - rows->top;
                ++g_slicks_diag_dirty_c2p_calls;
            }
            slicks_race_clear_dirty_rows(race);
            update_race_diagnostics(race);
            if (completed_now) {
                g_slicks_diag_checksum = checksum_planes(logical);
                g_slicks_diag_display_checksum =
                    checksum_bitmap(platform.views[1].bitmap);
                slicks_diag_results_ready();
            }
            if (race->frame_count == g_slicks_diag_target_frame) {
                g_slicks_diag_checksum = checksum_planes(logical);
                g_slicks_diag_display_checksum =
                    checksum_bitmap(platform.views[1].bitmap);
                slicks_diag_gameplay_ready();
            }
            slicks_diag_frame_ready();
        }
    }

cleanup:
    g_slicks_diag_ready = 0;
    g_slicks_diag_ingame = 0;
    slicks_resource_archive_close(&archive);
    slicks_amiga_audio_stop(&audio);
    slicks_amiga_platform_end(&platform);
    if (platform.gfx_base) {
        g_slicks_diag_restore_status =
            slicks_amiga_platform_restore_status(&platform);
        slicks_diag_system_restored();
    }
    slicks_amiga_platform_destroy(&platform);
    slicks_amiga_audio_destroy(&audio);
    if (sample_resource)
        FreeMem(sample_resource, 131691UL);
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
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    GfxBase = 0;
    DOSBase = 0;
    return result;
}
