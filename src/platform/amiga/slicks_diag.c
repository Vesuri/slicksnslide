#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <utility/date.h>
#include <hardware/cia.h>
#include <hardware/intbits.h>
#include <proto/cia.h>
#include <resources/cia.h>
#include <devices/input.h>
#include <devices/inputevent.h>

#include "../../game/race_runtime.h"
#include "../../game/driver_input.h"
#include "../../game/driver_device.h"
#include "../../game/track_scene.h"
#include "../../graphics/row_offsets.h"
#include "../../game/race_options.h"
#include "../../game/player_profiles.h"
#include "../../game/profile_palette.h"
#include "../../game/setup_session.h"
#include "../../game/championship.h"
#include "../../game/post_race_records.h"
#include "../../ui/palette_fade.h"
#include "../../ui/result_wait.h"
#include "../../ui/title_demo.h"
#include "../../ui/title_background.h"
#include "../../ui/track_data_view.h"
#include "../../game/race_return.h"
#include "../../ui/saved_file_dialog.h"
#include "amiga_saved_files.h"
#include "../../game/arcade_setup.h"
#include "../../game/race_timing.h"
#include "../../gen/setup_defaults.h"
#include "../../ui/service_options.h"
#include "amiga_platform.h"
#include "amiga_audio.h"
#include "amiga_player_menu.h"
#include "amiga_shop.h"
#include "amiga_setup_storage.h"
#include "../../ui/player_menu.h"
#include "../../ui/profile_actions.h"
#include "../../ui/title_help.h"
#include "../../ui/title_navigation.h"
#include "../../ui/title_start.h"
#include "../../ui/arcade_title_painter.h"
#include "../../ui/language_table.h"
#include "resource_archive.h"
#include "menu_resources.h"
#include "../../ui/font_resource.h"
#include "../../game/registration.h"
#include "../../ui/registration_ui.h"
#include "../../ui/help_text_dirty.h"
#include "../../ui/menu_bitmap.h"
extern void slicks_draw_title_registration(unsigned char *,const unsigned char *);
extern void slicks_tick_title_registration(unsigned char *,const unsigned char *,const unsigned char *);
extern void slicks_tick_title_colours(unsigned char *,const unsigned char *);
extern void slicks_draw_title_background(unsigned char *,const unsigned char *,const unsigned char *);
extern void slicks_records_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);

static struct SlicksRegistration registration;
static struct SlicksResourceCache *menu_cache;
static struct SlicksAmigaTrackListCache track_list_cache;
static struct SlicksAmigaSavedFilesCache saved_files_cache;
volatile unsigned long g_slicks_menu_cache_bytes;
volatile short g_slicks_registration_status;
__attribute__((noinline)) void slicks_diag_registration_loaded(void) { __asm__ volatile("" ::: "memory"); }
/* OS-owned startup only. Never serialize registration in CFG/PLR/SSS files. */
static int load_registration(void)
{
    unsigned char bytes[64]; unsigned long count=0; int failed=0;
    BPTR file=Open((CONST_STRPTR)"SLICKS.REK",MODE_OLDFILE);
    if(!file) {
        if(IoErr()!=ERROR_OBJECT_NOT_FOUND) return -1;
        return slicks_registration_decode(&registration,0,0);
    }
    while(count<sizeof bytes) {
        LONG n=Read(file,bytes+count,(LONG)(sizeof bytes-count));
        if(n<0) { failed=1; break; }
        if(!n) break;
        count+=(unsigned long)n;
    }
    if(!Close(file)) failed=1;
    return failed?-1:slicks_registration_decode(&registration,bytes,count);
}

unsigned char *slicks_title_font;
unsigned char *slicks_title_small_font;
static unsigned char *title_arcade_font;
static unsigned char title_language[2048];
static unsigned title_language_used;
extern const unsigned char *slicks_title_labels[7];
static char menu_language_name[10]="lang1.txt";
static unsigned char language_choice_test;
volatile unsigned char g_slicks_language_console_modes,g_slicks_language_console_bytes;

/* Emulator fixture only: deliver real Amiga input events to console.device,
 * not bytes directly to the chooser or writes from the debugger. */
static int language_console_test_key(unsigned step)
{
    static const unsigned char keys[]={0x4c,0x4d,0x4d,0x4c,0x45};
    if(step>=sizeof keys) return -1;
    struct MsgPort *port=CreateMsgPort();
    if(!port) return -1;
    struct IOStdReq *request=(struct IOStdReq *)CreateIORequest(port,sizeof(*request));
    int result=-1;
    if(request) {
        if(!OpenDevice((CONST_STRPTR)"input.device",0,(struct IORequest *)request,0)) {
            struct InputEvent events[2]={0};
            events[0].ie_NextEvent=&events[1];
            events[0].ie_Class=events[1].ie_Class=IECLASS_RAWKEY;
            events[0].ie_Code=keys[step]; events[1].ie_Code=keys[step]|0x80;
            request->io_Command=IND_WRITEEVENT; request->io_Data=events;
            request->io_Length=sizeof events;
            result=DoIO((struct IORequest *)request)?-1:0;
            CloseDevice((struct IORequest *)request);
        }
        DeleteIORequest((struct IORequest *)request);
    }
    DeleteMsgPort(port); return result;
}

/* Original startup's text-console chooser. This runs before hardware
 * takeover; the game menus themselves never need console/disk transitions. */
#include "amiga_language_chooser.h"
static unsigned char title_arcade_refresh=2;
static struct SlicksTitleDemo title_demo;
static unsigned char demo_render_only;
static unsigned char demo_lifecycle_test,demo_test_stage,demo_test_round;
static unsigned char display_allocation_test;
int g_slicks_display_allocation_checks;
volatile unsigned char g_slicks_demo_test_error,g_slicks_demo_test_views;
volatile unsigned char g_slicks_demo_idle_entries;
volatile unsigned char g_slicks_demo_menu_waits;
volatile unsigned long g_slicks_loading_io_bytes,g_slicks_loading_io_hash;
volatile unsigned char g_slicks_loading_io_checks;
volatile unsigned char g_slicks_demo_natural_returns;
const struct SlicksConfiguration *g_slicks_demo_expected_configuration;
static const short *demo_expected_playlist;
static unsigned short demo_expected_playlist_count;
volatile unsigned char g_slicks_demo_saved_roundtrip;
volatile unsigned long g_slicks_demo_return_frames[2],g_slicks_demo_return_clocks[2],g_slicks_demo_return_deadlines[2];
static unsigned long demo_idle_input_at;
volatile unsigned long g_slicks_demo_idle_wait_frames;
__attribute__((noinline)) void slicks_diag_demo_test_done(void) { __asm__ volatile("" ::: "memory"); }
extern unsigned char slicks_title_counter;
extern unsigned short slicks_title_third_color;
extern void slicks_title_font_text(unsigned char *,const unsigned char *,const unsigned char *,short,short,unsigned short,unsigned short);
#include "../../ui/title_status.h"
#include "../../ui/menu_icon.h"
static const struct SlicksConfiguration *title_configuration;
static unsigned char title_icons[5][64];
static unsigned short title_icon_width[5],title_icon_height[5];
extern void slicks_draw_title_status_text(unsigned char *,const char *,short,short,unsigned short);
#define TITLE_FONT_CAPACITY 8192UL
volatile unsigned long g_slicks_load_ticks[14];
static unsigned char shop_end_game;
static unsigned char shop_test;
static unsigned char mode_transition_test;
volatile unsigned short g_slicks_diag_mode_case;
static unsigned char shop_transition_test,shop_transition_phase;
static unsigned char shop_resume_test;
/* NATURALW-only parameter: 1..8 exercise each weapon; 9 buys two and cycles.
 * Cases 7/8 test the registered branch, never normal setup. */
volatile unsigned short g_slicks_diag_weapon_case;
volatile unsigned short g_slicks_diag_weapon_hud_checks,g_slicks_diag_weapon_hud_failures;
static short weapon_hud_last_count=-1;
static signed char weapon_hud_last_selection=-2;
__attribute__((noinline)) void slicks_diag_weapon_hud_checked(void) { __asm__ volatile("" ::: "memory"); }
static short shop_track_position;
/* A staged championship is not published to the live playlist until its
 * race assets load successfully. Its shop must still show the staged total. */
static short shop_track_total;
volatile unsigned short g_slicks_shop_test_phase;
volatile unsigned short g_slicks_shop_help_phase;
struct SlicksShopMenu *g_slicks_shop_menu;
void __attribute__((noinline)) slicks_diag_shop_ready(void) { __asm__ volatile("" ::: "memory"); }

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;
extern unsigned char *slicks_title_background;
/* Setup-owned state survives races; it is not an emulated DOS data segment. */
struct SlicksPlayerProfiles g_slicks_profiles;
struct SlicksSetupSession g_slicks_setup_session;
unsigned char g_slicks_diag_race_load_fault;
struct SlicksAmigaPlayerMenu *g_slicks_diag_pause_menu;
volatile unsigned short g_slicks_diag_pause_phase;
struct SlicksAmigaPlayerMenu *g_slicks_diag_intermission_menu;
volatile unsigned short g_slicks_diag_intermission_phase;
void __attribute__((noinline)) slicks_diag_intermission_checkpoint(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_intermission_input(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_intermission_retry(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_checkpoint(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_child(void) { __asm__ volatile("" ::: "memory"); }
struct SlicksRaceRuntime *g_slicks_diag_paused_race;
volatile unsigned char g_slicks_diag_pause_unavailable;
void __attribute__((noinline)) slicks_diag_pause_live_enter(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_live_recovered(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_warning_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_live_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_pause_live_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_race_load_failed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_race_load_dismissed(void) { __asm__ volatile("" ::: "memory"); }
volatile unsigned char g_slicks_diag_unique_stage;
void __attribute__((noinline)) slicks_diag_unique_selection(void) { __asm__ volatile("" ::: "memory"); }
struct SlicksAmigaPlayerMenu *g_slicks_player_menu;
struct SlicksAmigaPlayerMenu *g_slicks_options_menu;
struct SlicksAmigaPlayerMenu *g_slicks_track_menu;
struct SlicksTrackRenderer g_slicks_track_renderer;
struct SlicksTrackMenu g_slicks_track_state;
static short track_selection[256];
struct SlicksTrackPlaylist g_slicks_track_playlist={track_selection,1,256};
volatile unsigned short g_slicks_track_action;
void __attribute__((noinline)) slicks_diag_tracks_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_tracks_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_track_info_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_track_info_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_track_info_warning_closed(void) { __asm__ volatile("" ::: "memory"); }
struct SlicksAmigaPlayerMenu *g_slicks_title_help;
struct SlicksOptionsRenderer g_slicks_options_renderer;
struct SlicksOptionsMenu g_slicks_options_state;
volatile unsigned short g_slicks_options_action;
const struct SlicksConfiguration *g_slicks_options_configuration;
void __attribute__((noinline)) slicks_diag_options_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_options_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_controllers_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_controllers_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_help_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_help_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_help_failed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_help_warning_closed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_help_test_ready(void) { __asm__ volatile("" ::: "memory"); }
static unsigned char help_fail_archive;
static unsigned char help_fail_surface;
unsigned char g_slicks_title_help_warning;
static unsigned char title_help_saved[280*22];
struct SlicksSetupLoadReport g_slicks_setup_load_report;
struct SlicksSetupStorageReport g_slicks_setup_save_report;
struct SlicksSetupStorageReport g_slicks_track_clear_report;
unsigned short g_slicks_track_clear_changed;
unsigned char g_slicks_track_clear_phase;
void __attribute__((noinline)) slicks_diag_track_clear_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_track_clear_cancelled(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_setup_saved(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_setup_save_failed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_setup_load_failed(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_setup_save_cancelled(void) { __asm__ volatile("" ::: "memory"); }
volatile unsigned short g_slicks_diag_player_menu_action;
volatile unsigned short g_slicks_diag_player_menu_row;
static struct SlicksSetupResources setup_resources;
static unsigned char setup_vehicle_requests[4];
static void request_setup_vehicle(void *context,unsigned driver,signed char vehicle)
{
    /* Asset preparation is deferred until GO; all vehicle resources are
     * loaded there before any selected car is started or drawn. */
    ((unsigned char *)context)[driver]=(unsigned char)vehicle;
}

#define CUSTOM_WORD(offset) \
    (*(volatile unsigned short *)(0xdff000UL + (offset)))
#define REG_VPOSR 0x004
#define REG_VHPOSR 0x006
#define PAL_RASTER_LINES 312UL
/* Kalms' modulo path reads one final 32-pixel block ahead.  Its output is
 * discarded, but the read must still stay within an allocated buffer. */
#define C2P_LOOKAHEAD_BYTES 32UL
#define CHUNKY_ALLOCATION_BYTES (320UL * 200UL + C2P_LOOKAHEAD_BYTES)
#define TITLE_FRAME_ALLOCATION_BYTES (64002UL + C2P_LOOKAHEAD_BYTES)

volatile unsigned short g_slicks_diag_ready;
volatile unsigned short g_slicks_diag_ingame;
volatile unsigned short g_slicks_diag_race_error;
volatile unsigned short g_slicks_diag_race_stage;
volatile unsigned short g_slicks_diag_shadow_check;
volatile unsigned long g_slicks_diag_jump_takeoffs;
volatile unsigned long g_slicks_diag_jump_landings;
volatile unsigned long g_slicks_diag_jump_shadow_frames;
volatile unsigned short g_slicks_diag_jump_peak;
volatile unsigned long g_slicks_diag_checksum;
volatile unsigned long g_slicks_diag_display_checksum;
volatile unsigned long g_slicks_diag_race_frame;
volatile unsigned long g_slicks_diag_skidmarks;
volatile unsigned short g_slicks_diag_particles;
volatile unsigned long g_slicks_diag_collisions;
volatile unsigned long g_slicks_diag_track_collisions;
volatile unsigned char g_slicks_diag_countdown_stage;
volatile unsigned char g_slicks_diag_start_light_visible;
volatile unsigned char g_slicks_diag_start_light_stage_mask;
volatile unsigned short g_slicks_diag_dirty_ranges;
volatile unsigned short g_slicks_diag_dirty_rows;
volatile unsigned short g_slicks_diag_dirty_pixels;
volatile unsigned short g_slicks_diag_sparse_converted;
volatile unsigned long g_slicks_diag_dirty_c2p_calls;
volatile unsigned long g_slicks_diag_dirty_c2p_rows;
volatile unsigned short g_slicks_diag_restore_status;
volatile unsigned short g_slicks_diag_force_exit;
volatile unsigned short g_slicks_diag_track_zones;
volatile unsigned short g_slicks_diag_track_files;
volatile char g_slicks_diag_first_track[12];
volatile char g_slicks_diag_second_track[12];
volatile unsigned long g_slicks_diag_material_checksum;
volatile unsigned long g_slicks_diag_surface_checksum;
volatile unsigned long g_slicks_diag_material_count[32];
volatile long g_slicks_diag_car_x[SLICKS_RACE_CAR_COUNT];
volatile long g_slicks_diag_car_y[SLICKS_RACE_CAR_COUNT];
volatile long g_slicks_diag_car_vx[SLICKS_RACE_CAR_COUNT];
volatile long g_slicks_diag_car_vy[SLICKS_RACE_CAR_COUNT];
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
volatile unsigned char g_slicks_diag_engine_sample_block;
volatile unsigned short g_slicks_diag_engine_frequency;
volatile unsigned short g_slicks_diag_engine_period;
volatile unsigned short g_slicks_diag_pitch_bank_mask;
volatile unsigned char g_slicks_diag_effect_sample_block;
volatile unsigned char g_slicks_diag_effect_priority;
volatile unsigned long
    g_slicks_diag_sound_event_totals[SLICKS_SOUND_SAMPLE_COUNT];
volatile unsigned char g_slicks_diag_music_started;
volatile unsigned char g_slicks_diag_acceleration[SLICKS_RACE_CAR_COUNT];
volatile unsigned char g_slicks_diag_steering[SLICKS_RACE_CAR_COUNT];
volatile unsigned char *g_slicks_diag_logical;
volatile unsigned long g_slicks_diag_target_frame = 200;
static unsigned char fuel_race_test;
static unsigned char service_menu_test;
static unsigned char completion_watch;
volatile unsigned short g_slicks_diag_damage_peak[4];
volatile unsigned short g_slicks_diag_repair_frames[4];
volatile unsigned short g_slicks_diag_damage_status_checks;
volatile unsigned long g_slicks_diag_damage_peak_impact;
static short previous_damage[4];
static struct SlicksStatusClock status_clock;
static unsigned long status_clock_vblank;
volatile unsigned short g_slicks_diag_status_checks;
volatile unsigned short g_slicks_diag_status_failures;
static unsigned char status_checked_phases;
volatile unsigned int g_slicks_diag_fuel[4];
volatile unsigned int g_slicks_diag_fuel_capacity[4];
volatile short g_slicks_diag_service[4];
volatile unsigned short g_slicks_diag_service_requests[4];
volatile unsigned short g_slicks_diag_refuel_frames[4];
volatile unsigned short g_slicks_diag_service_departures[4];
volatile unsigned char g_slicks_diag_fuel_surface[4];
volatile unsigned char g_slicks_diag_fuel_layer[4];
volatile long g_slicks_diag_fuel_speed[4];
volatile short g_slicks_diag_fuel_target_x[4], g_slicks_diag_fuel_target_y[4];
volatile unsigned short g_slicks_diag_pit_frames[4];
/* Opt-in through the debugger; never part of performance measurements.
 * Reuses the no-longer-needed decoded title source allocation for a snapshot. */
volatile unsigned char g_slicks_diag_audit_bitmap;
static unsigned char weapon_hud_fixture;

/* Explicit renderer diagnostic only; never seed ordinary gameplay inventory. */
static void set_weapon_hud_fixture(struct SlicksRaceRuntime *race)
{
    unsigned phase=race->frame_count<350?0:race->frame_count<500?1:race->frame_count<550?2:3;
    race->weapons_enabled=phase!=2;
    for(unsigned car=0;car<4;++car) {
        race->selected_weapon[car]=(signed char)(car+(phase==1?4:0));
        for(unsigned slot=5;slot<13;++slot) {
            race->weapon_capacity[slot]=20;
            race->weapon_inventory[car][slot]=(short)(5+car*5);
        }
    }
}
volatile unsigned char g_slicks_diag_scanout_only;
volatile unsigned char g_slicks_diag_audio_in_blank = 1;
volatile unsigned long g_slicks_diag_audio_blank_spills;
volatile unsigned short g_slicks_diag_audio_hold_frames;
volatile unsigned long g_slicks_diag_audio_blank_max_lines;
volatile unsigned long g_slicks_diag_scanout_frames;
volatile unsigned long g_slicks_diag_audit_frame;
volatile unsigned long g_slicks_diag_audit_offset;
volatile unsigned short g_slicks_diag_audit_x;
volatile unsigned short g_slicks_diag_audit_y;
volatile unsigned char g_slicks_diag_audit_plane;
volatile unsigned char g_slicks_diag_audit_before;
volatile unsigned char g_slicks_diag_audit_after;
volatile unsigned long g_slicks_diag_profile_step_vblanks;
volatile unsigned long g_slicks_diag_profile_audio_vblanks;
volatile unsigned long g_slicks_diag_profile_c2p_vblanks;
volatile unsigned long g_slicks_diag_profile_diag_vblanks;
volatile unsigned long g_slicks_diag_profile_total_vblanks;
volatile unsigned long g_slicks_diag_profile_step_lines;
volatile unsigned long g_slicks_diag_profile_audio_lines;
volatile unsigned long g_slicks_diag_profile_c2p_lines;
volatile unsigned long g_slicks_diag_profile_diag_lines;
volatile unsigned long g_slicks_diag_profile_total_lines;
volatile unsigned char g_slicks_diag_profile_all;
volatile unsigned char g_slicks_diag_live_stats;
volatile unsigned long g_slicks_diag_bench_frames;
volatile unsigned long g_slicks_diag_bench_work_max;
volatile unsigned long g_slicks_diag_bench_work_sum;
/* Bounded benchmark evidence, collected after the work timer stops. */
volatile unsigned long g_slicks_diag_bench_work_samples[704];
volatile unsigned short g_slicks_diag_bench_particle_samples[704];
volatile unsigned long g_slicks_diag_bench_stage_sum[8];
volatile unsigned long g_slicks_diag_bench_tail_sum[4];
volatile unsigned long g_slicks_diag_bench_simulation_sum[3];
volatile unsigned long g_slicks_diag_bench_actor_sum[8];
volatile unsigned long g_slicks_diag_bench_car_sum[2];
volatile unsigned long g_slicks_diag_bench_motion_sum[3];
static unsigned long g_slicks_diag_profile_motion[3],g_slicks_diag_profile_motion_at;
volatile unsigned long g_slicks_diag_bench_work_max_frame;
volatile unsigned long g_slicks_diag_bench_max_stages[8];
volatile unsigned short g_slicks_diag_bench_max_particles;
volatile unsigned long g_slicks_diag_bench_max_simulation[3];
volatile unsigned long g_slicks_diag_bench_max_tail[4];
volatile unsigned long g_slicks_diag_bench_max_actors[8];
volatile unsigned long g_slicks_diag_bench_max_car_draw[2];
volatile unsigned long g_slicks_diag_bench_max_rect_pixels;
volatile unsigned long g_slicks_diag_initial_cache_control;
static unsigned long g_slicks_diag_profile_rect_pixels;
volatile unsigned short g_slicks_diag_bench_max_sparse;
static unsigned long g_slicks_diag_profile_car_draw[2],g_slicks_diag_profile_car_at;
static unsigned long g_slicks_diag_profile_sprite[3],g_slicks_diag_profile_sprite_at;
volatile unsigned long g_slicks_diag_bench_max_sprite[3];
static unsigned long g_slicks_diag_profile_tail[4], g_slicks_diag_profile_tail_at;
volatile unsigned long g_slicks_diag_bench_work_over;
volatile unsigned long g_slicks_diag_bench_wall_max;
volatile unsigned long g_slicks_diag_bench_cadence_sum;
volatile unsigned long g_slicks_diag_bench_cadence_count;
static unsigned long g_slicks_diag_bench_previous;
volatile unsigned long g_slicks_diag_profile_restore_lines;
volatile unsigned long g_slicks_diag_profile_advance_lines;
volatile unsigned long g_slicks_diag_profile_update_lines;
volatile unsigned long g_slicks_diag_profile_hud_lines;
volatile unsigned long g_slicks_diag_profile_draw_lines;
volatile unsigned long g_slicks_diag_profile_actor_lines[24];
static unsigned long g_slicks_diag_profile_actor_at;
static const struct SlicksAmigaPlatform *g_slicks_diag_profile_platform;
static unsigned long g_slicks_diag_profile_race_at;

/* NATURALS<track>: statistical PC sampling of measured race updates.
 * GDB stops are only serviced at vsync, so debugger interrupts phase-lock
 * to the beam; this target-side CIA-B timer samples wall time instead.
 * Diagnostic only: normal play and the other benchmark modes never start it. */
extern void slicks_pc_sampler_handler(void);
volatile unsigned char *g_slicks_pc_samples;
volatile unsigned long g_slicks_pc_sample_count;
volatile unsigned long g_slicks_pc_sample_capacity;
volatile unsigned long g_slicks_pc_sample_missed;
volatile unsigned long g_slicks_pc_sample_period = 0x2545f491UL;
volatile unsigned short g_slicks_pc_sample_frame = 0xffff;
volatile unsigned long g_slicks_pc_sample_first_frame;
volatile unsigned char *g_slicks_pc_sample_timer;
volatile unsigned char g_slicks_pc_sampling, g_slicks_pc_sampler_bit = 0xff;
static struct Library *g_slicks_pc_ciab;
static struct Interrupt g_slicks_pc_interrupt;

static void pc_sampler_start(void)
{
    static const unsigned long capacities[] = {16384, 8192, 4096};
    volatile unsigned char *control;
    unsigned bit;
    if (g_slicks_pc_sampler_bit != 0xff) return;
    for (unsigned i = 0; i < 3 && !g_slicks_pc_samples; ++i) {
        g_slicks_pc_samples = AllocMem(capacities[i] * 8, MEMF_ANY);
        if (g_slicks_pc_samples) g_slicks_pc_sample_capacity = capacities[i];
    }
    g_slicks_pc_ciab = OpenResource((CONST_STRPTR)CIABNAME);
    if (!g_slicks_pc_samples || !g_slicks_pc_ciab) return;
    g_slicks_pc_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    g_slicks_pc_interrupt.is_Node.ln_Name = (char *)"Slicks PC sampler";
    g_slicks_pc_interrupt.is_Code = slicks_pc_sampler_handler;
    for (bit = CIAICRB_TA; bit <= CIAICRB_TB; ++bit) {
        /* Touch only a timer whose interrupt vector was actually free. */
        if (AddICRVector(g_slicks_pc_ciab, (WORD)bit, &g_slicks_pc_interrupt))
            continue;
        control = (volatile unsigned char *)(bit == CIAICRB_TA ? 0xbfde00UL : 0xbfdf00UL);
        g_slicks_pc_sample_timer =
            (volatile unsigned char *)(bit == CIAICRB_TA ? 0xbfd400UL : 0xbfd600UL);
        *control = 0;                     /* stopped, continuous, E clock */
        g_slicks_pc_sample_timer[0] = 0xe8;
        g_slicks_pc_sample_timer[0x100] = 0x03;
        *control = CIACRAF_LOAD | CIACRAF_START;
        g_slicks_pc_sampler_bit = (unsigned char)bit;
        *(volatile unsigned short *)0xdff09aUL = INTF_SETCLR | INTF_EXTER;
        return;
    }
}

static void pc_sampler_stop(void)
{
    if (g_slicks_pc_sampler_bit >= 0xfe) return;
    *(volatile unsigned char *)(g_slicks_pc_sampler_bit == CIAICRB_TA ?
        0xbfde00UL : 0xbfdf00UL) = 0;
    RemICRVector(g_slicks_pc_ciab, (WORD)g_slicks_pc_sampler_bit, &g_slicks_pc_interrupt);
    g_slicks_pc_sampler_bit = 0xfe;       /* stopped; samples stay for GDB */
}

static void pc_sampler_release(void)
{
    pc_sampler_stop();
    if (g_slicks_pc_samples)
        FreeMem((APTR)g_slicks_pc_samples, g_slicks_pc_sample_capacity * 8);
    g_slicks_pc_samples = 0;
#ifdef SLICKS_SHADOW_CHECK
    {
        extern unsigned char *slicks_shadow_state,*slicks_shadow_chunky;
        if (slicks_shadow_state) FreeMem(slicks_shadow_state, 57344);
        if (slicks_shadow_chunky) FreeMem(slicks_shadow_chunky, 64000);
        slicks_shadow_state = 0;
        slicks_shadow_chunky = 0;
    }
#endif
}

#ifdef SLICKS_RETENTION_CHECK
#include "retention_snapshot.h"
volatile unsigned long g_slicks_retention_immutable_mismatches;
volatile unsigned long g_slicks_retention_checks, g_slicks_retention_mismatches,
    g_slicks_retention_first_mismatch, g_slicks_retention_particle_mismatches;
volatile unsigned long g_slicks_status_cache_checks,g_slicks_status_cache_mismatches,
    g_slicks_status_cache_first_mismatch;
static unsigned long retention_check_hash(const unsigned char *chunky)
{
    const unsigned long *p = (const unsigned long *)chunky;
    unsigned long h = 0x811c9dc5UL;
    for (unsigned i = 0; i < 16000; ++i) h = ((h << 5) | (h >> 27)) ^ p[i];
    return h;
}
#endif

__attribute__((noinline)) void slicks_diag_frame_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_player_menu_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_player_menu_closed(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_profile_picker_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_profile_editor_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_name_dialog_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_colour_dialog_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_gameplay_ready(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_collision_failed(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_race_progress(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_system_restored(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_bitmap_audit_failed(void)
{
    __asm volatile("" ::: "memory");
}

__attribute__((noinline)) void slicks_diag_results_ready(void)
{
    __asm volatile("" ::: "memory");
}

static void race_checkpoint(unsigned short stage)
{
    struct DateStamp now;
    DateStamp(&now);
    if(stage<14) g_slicks_load_ticks[stage]=(unsigned long)now.ds_Minute*3000UL+now.ds_Tick;
    g_slicks_diag_race_stage = stage;
    if (g_slicks_diag_ready)
        slicks_diag_frame_ready();
}

unsigned long slicks_diag_profile_raster_time(void)
{
    const struct SlicksAmigaPlatform *platform =
        g_slicks_diag_profile_platform;
    unsigned long frame_before;
    unsigned long frame_after;
    unsigned short line;
    unsigned short high,high_after;
    do {
        frame_before = platform->vblank_count;
        high=CUSTOM_WORD(REG_VPOSR)&7;
        line=(unsigned short)((high<<8)|(CUSTOM_WORD(REG_VHPOSR)>>8));
        high_after=CUSTOM_WORD(REG_VPOSR)&7;
        frame_after = platform->vblank_count;
    } while (frame_before != frame_after || high!=high_after);
    return frame_before * PAL_RASTER_LINES + line;
}

static void slicks_diag_profile_race(unsigned char phase)
{
    unsigned long now = slicks_diag_profile_raster_time();
    if(phase>=70 && phase<=74) {
        if(phase==71)g_slicks_diag_profile_motion[0]=now-g_slicks_diag_profile_motion_at;
        else if(phase>=73)g_slicks_diag_profile_motion[phase-72]=now-g_slicks_diag_profile_motion_at;
        g_slicks_diag_profile_motion_at=now;
        return;
    }
    if(phase>=59 && phase<=63) {
        if(phase==59) {
            for(unsigned i=0;i<3;++i)g_slicks_diag_profile_sprite[i]=0;
        } else if(phase>=61) {
            g_slicks_diag_profile_sprite[phase-61]+=now-g_slicks_diag_profile_sprite_at;
        }
        g_slicks_diag_profile_sprite_at=now;
        return;
    }
    if(phase>=49 && phase<=52) {
        if(phase==49) {
            g_slicks_diag_profile_car_draw[0]=g_slicks_diag_profile_car_draw[1]=0;
        } else if(phase>50) {
            g_slicks_diag_profile_car_draw[phase-51]+=now-g_slicks_diag_profile_car_at;
        }
        g_slicks_diag_profile_car_at=now;
        return;
    }
    if(phase>=39 && phase<=44) {
        if(phase==39) {
            for(unsigned i=0;i<4;++i) g_slicks_diag_profile_tail[i]=0;
        } else if(phase>40) {
            g_slicks_diag_profile_tail[phase-41]+=now-g_slicks_diag_profile_tail_at;
        }
        g_slicks_diag_profile_tail_at=now;
        return;
    }
    if(phase>=10) {
        if(phase!=10 && phase!=20 && phase!=30)
            g_slicks_diag_profile_actor_lines[phase-10]=now-g_slicks_diag_profile_actor_at;
        g_slicks_diag_profile_actor_at=now;
        return;
    }
    if (!phase) {
        g_slicks_diag_profile_race_at = now;
        return;
    }
    if (phase == 1)
        g_slicks_diag_profile_restore_lines =
            now - g_slicks_diag_profile_race_at;
    else if (phase == 2)
        g_slicks_diag_profile_advance_lines =
            now - g_slicks_diag_profile_race_at;
    else if (phase == 3)
        g_slicks_diag_profile_update_lines =
            now - g_slicks_diag_profile_race_at;
    else if (phase == 4)
        g_slicks_diag_profile_hud_lines =
            now - g_slicks_diag_profile_race_at;
    else if (phase == 5)
        g_slicks_diag_profile_draw_lines =
            now - g_slicks_diag_profile_race_at;
    g_slicks_diag_profile_race_at = now;
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
extern void slicks_chunky_rect_to_amiga(const unsigned char *chunky,
                                        struct BitMap *bitmap,
                                        unsigned long left,
                                        unsigned long top,
                                        unsigned long right,
                                        unsigned long bottom,
                                        unsigned char *packed_scratch);
extern void slicks_chunky_pixels_to_amiga(
    const unsigned char *chunky, struct BitMap *bitmap,
    const struct SlicksDirtyPixel *pixels, unsigned long count);

#define SLICKS_TRACK_FILE_MAX 256
#define SLICKS_TRACK_NAME_SIZE 12

static unsigned short discover_tracks(
    char names[SLICKS_TRACK_FILE_MAX][SLICKS_TRACK_NAME_SIZE],unsigned char legacy_basic_first)
{
    struct FileInfoBlock *info =
        (struct FileInfoBlock *)AllocDosObject(DOS_FIB, 0);
    BPTR lock = Lock((CONST_STRPTR)"TRACKS", ACCESS_READ);
    unsigned short count = 0;
    unsigned short at;
    if (!info || !lock)
        goto cleanup;
    if (!Examine(lock, info))
        goto cleanup;
    while (count < SLICKS_TRACK_FILE_MAX && ExNext(lock, info)) {
        const char *source = (const char *)info->fib_FileName;
        unsigned short length = 0;
        if (info->fib_DirEntryType >= 0)
            continue;
        while (source[length] && length < SLICKS_TRACK_NAME_SIZE)
            ++length;
        if (length < 4 || length >= SLICKS_TRACK_NAME_SIZE ||
            source[length - 3] != '.' ||
            (source[length - 2] != 'S' && source[length - 2] != 's') ||
            (source[length - 1] != 'S' && source[length - 1] != 's'))
            continue;
        for (at = 0; at <= length; ++at)
            names[count][at] = source[at];
        ++count;
    }
cleanup:
    if (lock)
        UnLock(lock);
    if (info)
        FreeDosObject(DOS_FIB, info);
    if (!count) {
        static const char fallback[] = "BASIC.SS";
        for (at = 0; at < sizeof(fallback); ++at)
            names[0][at] = fallback[at];
        count = 1;
    }
    /* AmigaDOS directory order is filesystem-dependent. Native setup uses
     * alphabetical names; only legacy race diagnostics promote BASIC. */
    {
        unsigned short left;
        for (left = 0; left + 1 < count; ++left) {
            unsigned short right;
            for (right = left + 1; right < count; ++right) {
                unsigned short character = 0;
                while (names[left][character] == names[right][character] &&
                       names[left][character])
                    ++character;
                if ((unsigned char)names[right][character] <
                    (unsigned char)names[left][character]) {
                    char temporary[SLICKS_TRACK_NAME_SIZE];
                    for (at = 0; at < SLICKS_TRACK_NAME_SIZE; ++at) {
                        temporary[at] = names[left][at];
                        names[left][at] = names[right][at];
                        names[right][at] = temporary[at];
                    }
                }
            }
        }
    }
    for (at = 0; legacy_basic_first && at < count; ++at) {
        static const char basic[] = "BASIC.SS";
        unsigned short character = 0;
        while (basic[character] == names[at][character] && basic[character])
            ++character;
        if (!basic[character] && !names[at][character]) {
            char temporary[SLICKS_TRACK_NAME_SIZE];
            unsigned short byte;
            unsigned short position;
            for (byte = 0; byte < SLICKS_TRACK_NAME_SIZE; ++byte)
                temporary[byte] = names[at][byte];
            for (position = at; position > 0; --position)
                for (byte = 0; byte < SLICKS_TRACK_NAME_SIZE; ++byte)
                    names[position][byte] = names[position - 1][byte];
            for (byte = 0; byte < SLICKS_TRACK_NAME_SIZE; ++byte)
                names[0][byte] = temporary[byte];
            break;
        }
    }
    return count;
}

static void make_track_path(char *path, const char *name)
{
    static const char prefix[] = "TRACKS/";
    unsigned short at;
    for (at = 0; at < sizeof(prefix) - 1; ++at)
        path[at] = prefix[at];
    while (*name)
        path[at++] = *name++;
    path[at] = 0;
}

#include "amiga_key_scan.h"
#include "../../ui/title_dirty.h"
static struct SlicksTitleDirty title_dirty;
volatile unsigned long g_slicks_title_full_publications,g_slicks_title_partial_publications;
volatile unsigned long g_slicks_title_last_pixels;
static unsigned char title_dirty_test;
volatile unsigned long g_slicks_title_dirty_checks,g_slicks_title_dirty_errors;
volatile unsigned short g_slicks_title_seen_modes,g_slicks_title_seen_roles,g_slicks_title_seen_counts;
volatile unsigned short g_slicks_title_arcade_counts,g_slicks_title_arcade_draws;
static void publish_title_dirty(struct SlicksAmigaPlatform *p,
    const unsigned char *logical,unsigned char *chunky)
{
    if(!title_dirty.count) return;
    slicks_title_dirty_unpack(&title_dirty,logical,chunky);
    /* Prepare in ordinary memory first; start publication at a fresh display
     * end, not partway through the lower border. All eight planes per block. */
    slicks_amiga_platform_wait_display_end(p);
    unsigned long pixels=0;
    for(unsigned i=0;i<title_dirty.count;++i) {
        const struct SlicksTitleRect *r=&title_dirty.rects[i];
        slicks_chunky_rect_to_amiga(chunky,p->views[0].bitmap,
            r->left,r->top,r->right,r->bottom,0);
        pixels+=(unsigned long)(r->right-r->left)*(r->bottom-r->top);
    }
    g_slicks_title_last_pixels=pixels;
    if(pixels==64000) ++g_slicks_title_full_publications;
    else ++g_slicks_title_partial_publications;
    title_dirty.count=0;
    if(title_dirty_test) {
        const struct BitMap *bitmap=p->views[0].bitmap;
        for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
            unsigned colour=0;
            for(unsigned plane=0;plane<8;++plane)
                if(bitmap->Planes[plane][y*320+(x>>3)]&(128U>>(x&7))) colour|=1U<<plane;
            unsigned expected=logical[(x&3)*65536UL+y*100+(x>>2)];
            if(colour!=expected || chunky[mult320[y]+x]!=expected) ++g_slicks_title_dirty_errors;
        }
        ++g_slicks_title_dirty_checks;
    }
}

static void make_title_surface(unsigned char *planes,
                               const unsigned char *frame,
                               const unsigned char *palette)
{
    if(title_configuration && title_configuration->options[0]==5)
        slicks_draw_title_background(planes,frame,palette);
    else slicks_draw_title_pages(planes, frame, palette);
    title_arcade_refresh=2;
    slicks_title_dirty_add(&title_dirty,0,0,320,200);
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

static int show_race_load_error(struct SlicksAmigaPlatform *platform,
    unsigned char *logical,unsigned char *chunky,unsigned short *mode_state,
    const unsigned char *palette,unsigned char retry)
{
    if(slicks_setup_basic_mode(logical,mode_state)) return -1;
    clear_title_rectangle(logical,0,0,320,200);
    slicks_draw_title_text(logical,"RACE SETUP FAILED",160,70,15);
    slicks_draw_title_text(logical,g_slicks_diag_race_error==1?"NOT ENOUGH MEMORY":
        g_slicks_diag_race_error==7?"CHECK PLAYERS AND CONTROLLERS":"CHECK GAME AND TRACK FILES",160,90,15);
    slicks_draw_title_text(logical,retry?"ENTER RETRIES - ESC RETURNS TO MENU":
        "ENTER OR ESC RETURNS TO MENU",160,110,15);
    slicks_convert_to_amiga(logical,chunky,platform->views[0].bitmap);
    if(slicks_amiga_platform_set_view(platform,0,palette) ||
       slicks_amiga_platform_begin(platform,0)) return -1;
    g_slicks_diag_ingame=0; g_slicks_diag_ready=1;
    slicks_diag_race_load_failed(); return 0;
}

static void arcade_dirty(void *p,short l,short t,short r,short b)
{(void)p;slicks_title_dirty_add(&title_dirty,l,t,r,b);}
extern short slicks_menu_measure(const unsigned char *,const unsigned char *);
static void arcade_text(void *p,unsigned char *logical,const unsigned char *font,
    const unsigned char *text,short x,short y,unsigned short flags,unsigned short shadow)
{(void)p;slicks_title_font_text(logical,font,text,x,y,flags,shadow);__asm volatile("" ::: "memory");}
static void redraw_title_configuration(
    struct SlicksAmigaPlatform *platform, unsigned char *logical,
    unsigned char *chunky, const unsigned char *palette,
    unsigned short selection,
    unsigned short vehicle,
    const char *track_name, unsigned short laps)
{
    /* These legacy diagnostic arguments never belong on the original title.
     * Its status is alongside PLAYERS/TRACKS/OPTIONS, not a black footer. */
    (void)vehicle; (void)track_name; (void)laps;
    if(title_configuration && title_configuration->options[0]==5) {
        if(title_dirty_test) {
            ++g_slicks_title_arcade_draws;
            if(setup_resources.override_count>=1 && setup_resources.override_count<=4)
                g_slicks_title_arcade_counts|=1U<<(setup_resources.override_count-1);
        }
        struct SlicksArcadeTitlePainter painter={.logical=logical,
            .fonts={slicks_title_small_font,title_arcade_font,slicks_title_font,slicks_title_font},
            .background=slicks_title_background,.palette=palette,
            .players=slicks_language_lookup(title_language,title_language_used,(const unsigned char *)"players",(const unsigned char *)"PLAYERS"),
            .settings=slicks_language_lookup(title_language,title_language_used,(const unsigned char *)"settings",(const unsigned char *)"SETTINGS"),
            .summary=slicks_language_lookup(title_language,title_language_used,(const unsigned char *)"arcade.settingstext",(const unsigned char *)"%d SECS\n%d TRACKS"),
            .seconds=title_configuration->options[13],.tracks=title_configuration->options[14],
            .text=arcade_text,.dirty=arcade_dirty};
        if(slicks_arcade_title_paint(&painter,&slicks_title_counter,&title_arcade_refresh,(unsigned char)selection,
            setup_resources.override_count,(const signed char (*)[6])slicks_original_fallback_colours)) g_slicks_diag_force_exit=1;
        slicks_title_third_color=painter.shadow;
        goto owner;
    }
    slicks_title_dirty_add(&title_dirty,200,96,244,140);
    /* Restore the status background as well as the original label crop:
     * shrinking counts, inactive drivers and disabled badges must erase. */
    for(unsigned y=96;y<140;++y) for(unsigned x=200;x<244;++x)
        logical[(x&3)*65536UL+y*100+(x>>2)]=
            slicks_title_background[2+(x&3)*16000UL+y*80+(x>>2)];
    slicks_draw_title_menu_selection(logical, palette, selection);
    /* Original crop is byte-aligned: x=108..207, y=77..173. All menu
     * labels and the old/new selection bevel lie inside these bounds. */
    slicks_title_dirty_add(&title_dirty,108,77,208,174);
    if(title_configuration) {
        struct SlicksRaceOptions options;
        short mode=title_configuration->options[0];
        if(title_dirty_test) {
            if(mode>=0 && mode<6) g_slicks_title_seen_modes|=1U<<mode;
            for(unsigned i=0;i<4;++i) {
                signed char role=g_slicks_setup_session.players.participation[i];
                g_slicks_title_seen_roles|=role<0?1:role>0?2:4;
            }
            if(g_slicks_track_playlist.count==195) g_slicks_title_seen_counts|=1;
            if(g_slicks_track_playlist.count==194) g_slicks_title_seen_counts|=2;
        }
        slicks_resolve_race_options(&options,title_configuration,
            slicks_original_mode_flags[mode>=0 && mode<6?mode:0]);
        struct SlicksTitleStatusCommand commands[9];
        unsigned count=slicks_title_status_commands(commands,
            g_slicks_setup_session.players.participation,g_slicks_track_playlist.count,
            (short)g_slicks_diag_track_files,options.inventory_mode,options.weapons_enabled,mode);
        struct SlicksChunkyUi ui={.palette=palette};
        slicks_title_small_font[6]=slicks_ui_nearest(&ui,70,70,15);
        for(unsigned i=0;i<count;++i) {
            const struct SlicksTitleStatusCommand *c=&commands[i];
            if(c->kind) {
                char number[8]; unsigned value=c->value<0?-(int)c->value:c->value;
                unsigned at=sizeof number;number[--at]=0;
                do {number[--at]=(char)('0'+value%10);value/=10;} while(value);
                if(c->value<0) number[--at]='-';
                slicks_draw_title_status_text(logical,number+at,c->x,c->y,c->flags);
            } else {
                unsigned icon=(unsigned)c->value,w=title_icon_width[icon],h=title_icon_height[icon];
                for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) {
                    unsigned char pixel=title_icons[icon][y*w+x];
                    unsigned dx=(unsigned)c->x+x,dy=(unsigned)c->y+y;
                    if(pixel) logical[(dx&3)*65536UL+dy*100+(dx>>2)]=pixel;
                }
            }
        }
    }
owner:
    if(registration.name[0]) {
        slicks_draw_title_registration(logical,registration.name);
        struct SlicksChunkyUi bounds={0,palette,arcade_dirty,0};
        slicks_font_text_dirty(&bounds,slicks_title_small_font,registration.name,
            310,190,1,2,slicks_menu_measure(slicks_title_small_font,registration.name),0);
    }
    publish_title_dirty(platform,logical,chunky);
    /* Keep GCC from emitting a cross-section PC32 sibling jump, which the
     * HUNK converter cannot relocate. No extra hardware or rendering work. */
    __asm volatile("" ::: "memory");
}

static void present_menu_surface(struct SlicksAmigaPlatform *platform,struct SlicksAmigaPlayerMenu *menu)
{
    if(!menu->dirty_count) return;
    if(platform->active) slicks_amiga_platform_wait_display_blank(platform);
    for(unsigned i=0;i<menu->dirty_count;++i)
        slicks_chunky_rect_to_amiga(menu->renderer.ui.pixels,platform->views[0].bitmap,
            menu->dirty[i].left,menu->dirty[i].top,
            menu->dirty[i].right,menu->dirty[i].bottom,0);
    slicks_amiga_player_menu_clear_dirty(menu);
}
static void present_player_menu(struct SlicksAmigaPlatform *platform)
{ present_menu_surface(platform,g_slicks_player_menu); }

struct SlicksAmigaPlayerMenu *g_slicks_diag_saved_menu;
volatile unsigned short g_slicks_diag_saved_phase;
static unsigned char championship_test,championship_test_stage,championship_picker_count;
static unsigned char championship_dialog_step;
static unsigned char championship_delete_test;
static unsigned char championship_scan_test;
static unsigned char championship_cleanup_test;
extern unsigned char g_slicks_diag_backup_protect;
extern unsigned char g_slicks_diag_saved_lock_failure;
extern unsigned char g_slicks_diag_saved_next_failure;
void __attribute__((noinline)) slicks_diag_saved_ready(void) { __asm__ volatile("" ::: "memory"); }
void __attribute__((noinline)) slicks_diag_saved_closed(void) { __asm__ volatile("" ::: "memory"); }
static void championship_test_keys(struct SlicksAmigaPlatform *p,const unsigned char *keys,unsigned count)
{
    p->key_tail=0;
    for(unsigned i=0;i<count;++i) p->keys[i]=keys[i];
    p->key_head=(unsigned char)count;
}
static void championship_dialog_checkpoint(struct SlicksAmigaPlatform *p)
{
    slicks_diag_saved_ready();
    if(!championship_test) return;
    if(championship_cleanup_test) {
        static const unsigned char phases[]={1,3,3};
        static const unsigned char keys[][3]={{0x42,0x42,0x44},{0x15,0,0},{0x44,0x59,0x45}};
        static const unsigned char counts[]={3,1,3};
        unsigned step=championship_dialog_step++;
        if(step>=3 || g_slicks_diag_saved_phase!=phases[step]) {
            g_slicks_diag_force_exit=1; return;
        }
        championship_test_keys(p,keys[step],counts[step]); return;
    }
    if(championship_scan_test) {
        static const unsigned char leave[]={0x44,0x59,0x45};
        if(g_slicks_diag_saved_phase!=3 || championship_dialog_step++) {
            g_slicks_diag_force_exit=1;return;
        }
        championship_test_keys(p,leave,sizeof leave);return;
    }
    if(championship_delete_test) {
        static const unsigned char phases[]={1,3,3,1};
        static const unsigned char keys[][3]={{0x42,0x44,0},{0x15,0,0},
            {0x44,0,0},{0x45,0x59,0x45}};
        static const unsigned char counts[]={2,1,1,3};
        unsigned step=championship_dialog_step++;
        if(step>=sizeof phases || phases[step]!=g_slicks_diag_saved_phase) {
            g_slicks_diag_force_exit=1; return;
        }
        championship_test_keys(p,keys[step],counts[step]); return;
    }
    if(championship_test==3) {
        /* Real first intermission: cancel a name, create TEMP,
         * cancel/accept overwrite of E2E, cancel/accept deletion of TEMP.
         * Only ordinary keys; catalogue inspection chooses a visible row. */
        static const unsigned char phases[]={1,1,2,1,2,3,1,3,1,3,3,1,3,1,3,1};
        unsigned step=championship_dialog_step++;
        if(step>=sizeof phases || phases[step]!=g_slicks_diag_saved_phase) { g_slicks_diag_force_exit=1; return; }
        unsigned char keys[16]; unsigned count=0;
        if(step==2) keys[count++]=0x45;
        else if(step==4) { keys[count++]=0x14; keys[count++]=0x12; keys[count++]=0x37; keys[count++]=0x19; keys[count++]=0x44; }
        else if(step==5 || step==10) { keys[count++]=0x44; keys[count++]=0x44; }
        else if(step==6 || step==8 || step==11 || step==13) {
            struct SlicksListRenderer *r=&g_slicks_diag_saved_menu->picker->renderer;
            unsigned target=0; unsigned char initial=step<11?'E':'T';
            while(target<(unsigned)r->state.count && r->names[target*r->stride]!=initial) ++target;
            if(target>=12 || target==(unsigned)r->state.count) { g_slicks_diag_force_exit=1; return; }
            keys[count++]=0x42; /* Save As -> Delete */
            if(step<11) keys[count++]=0x42; /* Delete -> Save */
            while(target--) keys[count++]=0x4d;
            keys[count++]=0x44;
        } else if(step==7 || step==12) keys[count++]=0x36; /* N */
        else if(step==9 || step==14) keys[count++]=0x15; /* Y */
        else if(step==15) { keys[count++]=0x45; keys[count++]=0x59; keys[count++]=0x45; }
        else keys[count++]=0x44;
        championship_test_keys(p,keys,count); return;
    }
    if(championship_test==4) {
        unsigned char keys[2]={0x44,0x45};
        if(g_slicks_diag_saved_phase==3) ++championship_dialog_step;
        if(g_slicks_diag_saved_phase==1 && championship_dialog_step) keys[0]=0x45;
        championship_test_keys(p,keys,championship_dialog_step?2:1); return;
    }
    if(g_slicks_diag_saved_phase==1) {
        static const unsigned char cancel[]={0x45,0x44},accept[]={0x44};
        if(championship_test==6 && championship_dialog_step) {
            static const unsigned char leave[]={0x45,0x59,0x45};
            championship_test_keys(p,leave,sizeof leave);
        }
        else if((championship_test==1 || championship_test==6) && !championship_picker_count++) championship_test_keys(p,cancel,2);
        else championship_test_keys(p,accept,1);
    } else if(g_slicks_diag_saved_phase==2) {
        static const unsigned char name[]={0x12,0x02,0x12,0x44};
        championship_test_keys(p,name,4);
    } else if(g_slicks_diag_saved_phase==3) {
        static const unsigned char exit[]={0x44,0x59,0x45};
        if(championship_test==6) ++championship_dialog_step;
        championship_test_keys(p,exit,3);
    }
}
static int championship_notice(struct SlicksAmigaPlatform *p,struct SlicksAmigaPlayerMenu *m,
    const unsigned char *text)
{
    if(slicks_amiga_message_open(m,text,slicks_original_players_footer_percent)) return -1;
    present_menu_surface(p,m);
    if(!p->active && slicks_amiga_platform_begin(p,0)) return -1;
    g_slicks_diag_saved_phase=3; championship_dialog_checkpoint(p);
    int scan=0;
    while(!scan && !g_slicks_diag_force_exit) {
        unsigned short raw; slicks_amiga_platform_wait_vblank(p);
        while(slicks_amiga_platform_poll_key(p,&raw))
            if(!(raw&128) && (scan=amiga_raw_to_dos_scan(raw))) break;
    }
    if(slicks_amiga_message_close(m)) return -1;
    return g_slicks_diag_force_exit?-1:scan;
}

/* The original .SSS list/name widgets, with native transactional disk I/O.
 * RAM-only widget transitions retain takeover. Filesystem calls explicitly
 * release it. Closing RAM-only widgets preserves the current ownership;
 * callers reacquire only if a preceding file operation released it.
 * Returns 1 accepted, 0 cancelled, -1 unrecoverable display/allocation error. */
static int run_saved_game_dialog(struct SlicksAmigaPlatform *p,struct SlicksAmigaPlayerMenu *m,
    struct SlicksSavedGame *game,unsigned char tracks[][8],unsigned char saving)
{
    unsigned char (*names)[9]=saved_files_cache.names,name[9]={0}; char path[13];
    int result=-1;
    g_slicks_diag_saved_menu=m;
again:
    int count=saved_files_cache.count;
    if(count<0 || (!count && !saving)) {
        const unsigned char *message=count==-2?(const unsigned char *)"MORE THAN 40 SAVES - MANAGE FILES FIRST":
            count<0?(const unsigned char *)"CANNOT READ SAVED GAMES":slicks_original_saved_empty;
        if(championship_notice(p,m,message)<0) goto done;
        result=0; goto done;
    }
    if(slicks_amiga_saved_files_picker(m,names,count,saving,
        saving?slicks_original_saved_save:slicks_original_saved_load,slicks_original_players_footer_percent)) goto done;
    g_slicks_diag_saved_phase=1;
    for(;;) {
        if(slicks_amiga_profile_picker_draw(m,p->vblank_count)) goto done;
        present_menu_surface(p,m);
        if(!p->active && slicks_amiga_platform_begin(p,0)) goto done;
        championship_dialog_checkpoint(p);
        slicks_amiga_platform_wait_vblank(p);
        if(g_slicks_diag_force_exit) goto done;
        unsigned short raw;
        while(slicks_amiga_platform_poll_key(p,&raw)) {
            (void)slicks_amiga_menu_character(m,(unsigned char)raw);
            if(!(raw&128)) slicks_list_dialog_key(&m->picker->renderer.state,
                (unsigned char)amiga_raw_to_menu_scan(raw));
            if(m->picker->renderer.state.done) break;
        }
        if(m->picker->renderer.state.done) break;
    }
    struct SlicksSavedFileChoice choice=slicks_saved_file_choice(slicks_amiga_profile_picker_close(m),saving);
    if(choice.action==SLICKS_SAVED_FILE_CANCEL) { result=0; goto done; }
    if(choice.action==SLICKS_SAVED_FILE_NAME) {
        name[0]=0;
        if(slicks_amiga_saved_filename_open(m,name,slicks_original_saved_name,slicks_original_players_footer_percent)) goto done;
        int accepted=0;
        g_slicks_diag_saved_phase=2;
        while(!accepted) {
            if(slicks_amiga_name_dialog_tick(m,p->vblank_count)) goto done;
            present_menu_surface(p,m);
            if(!p->active && slicks_amiga_platform_begin(p,0)) goto done;
            championship_dialog_checkpoint(p); slicks_amiga_platform_wait_vblank(p);
            if(g_slicks_diag_force_exit) goto done;
            unsigned short raw;
            while(slicks_amiga_platform_poll_key(p,&raw)) {
                unsigned char character=slicks_amiga_menu_character(m,(unsigned char)raw);
                if(raw&128) continue;
                accepted=slicks_name_dialog_key(&m->name_dialog->renderer,character);
                m->name_dialog->hidden=0;
                if(accepted) break;
            }
        }
        if(slicks_amiga_name_dialog_close(m) || accepted<0) goto done;
        if(accepted!=1 || !name[0]) goto again;
    } else {
        if(choice.index<0 || choice.index>=count) goto again;
        for(unsigned i=0;i<9;++i) name[i]=names[choice.index][i];
    }
    if(slicks_saved_file_path(path,name)) {
        if(championship_notice(p,m,(const unsigned char *)"USE 1-8 LETTERS, DIGITS, - OR _")<0) goto done;
        goto again;
    }
    if(choice.action==SLICKS_SAVED_FILE_DELETE) {
        int key=championship_notice(p,m,slicks_original_saved_delete);
        if(key<0) goto done;
        if(slicks_saved_file_delete_accepted((unsigned char)key)) {
            slicks_amiga_platform_end(p);
            int failed=slicks_amiga_saved_file_delete(path);
            slicks_amiga_saved_files_refresh(&saved_files_cache);
            if(failed &&
               championship_notice(p,m,(const unsigned char *)"DELETE FAILED - CHECK NEW/BAK FILES")<0) goto done;
        }
        goto again;
    }
    const char *error=0;
    if(saving) {
        slicks_amiga_platform_end(p);
        int exists=slicks_amiga_saved_file_exists(path);
        if(exists<0) error="CANNOT ACCESS SAVE FILE";
        else if(exists) {
            int key=championship_notice(p,m,(const unsigned char *)"OVERWRITE THIS SAVED GAME? Y/N");
            if(key<0) goto done;
            if(key!=0x15) goto again;
        }
        if(!error) {
            slicks_amiga_platform_end(p);
            struct SlicksSetupStorageReport report=slicks_amiga_store_saved_game(path,game);
            slicks_amiga_saved_files_refresh(&saved_files_cache);
            if(report.result==SLICKS_SETUP_SAVED || report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING) {
                if(championship_notice(p,m,(const unsigned char *)(report.result==SLICKS_SETUP_SAVED?
                    "GAME SAVED":"GAME SAVED - BACKUP REMAINS"))<0) goto done;
                result=1; goto done;
            }
            error=report.result==SLICKS_SETUP_RECOVERY_REQUIRED?
                "SAVE RECOVERY REQUIRED - KEEP NEW/BAK":"SAVE FAILED - RETRY OR ESC";
        }
    } else {
        slicks_amiga_platform_end(p);
        struct SlicksSetupLoadReport report=slicks_amiga_load_saved_game(path,game,tracks,256);
        slicks_amiga_saved_files_refresh(&saved_files_cache);
        if(report.result==SLICKS_SETUP_LOADED) { result=1; goto done; }
        error=report.result==SLICKS_SETUP_LOAD_RECOVERY?"KEEP SAVE NEW/BAK FILES - RECOVERY REQUIRED":
            report.result==SLICKS_SETUP_LOAD_INVALID?"INVALID SAVED GAME":"LOAD FAILED - RETRY OR ESC";
    }
    if(championship_notice(p,m,(const unsigned char *)error)<0) goto done;
    goto again;
done:
    if(m->picker) (void)slicks_amiga_profile_picker_close(m);
    if(m->name_dialog) (void)slicks_amiga_name_dialog_close(m);
    if(m->message) (void)slicks_amiga_message_close(m);
    present_menu_surface(p,m);
    slicks_diag_saved_closed();
    g_slicks_diag_saved_menu=0; g_slicks_diag_saved_phase=0;
    return result;
}

void __attribute__((noinline)) slicks_diag_profile_dialog_failed(void) { __asm__ volatile("" ::: "memory"); }
static int profile_dialog_warning(struct SlicksAmigaPlatform *platform,unsigned char diagnostic)
{
    struct SlicksAmigaPlayerMenu *m=g_slicks_player_menu;
    m->editor_pending=0;
    if(slicks_amiga_warning_open(m,(const unsigned char *)"DIALOG UNAVAILABLE - PRESS A KEY")) return -1;
    present_player_menu(platform);
    if(!platform->active && slicks_amiga_platform_begin(platform,0)) return -1;
    slicks_diag_profile_dialog_failed();
    if(diagnostic) {
        /* Dismiss and retry through normal input, preserving queued typing. */
        unsigned short keys[15]; unsigned count=0;
        while(platform->key_tail!=platform->key_head) {
            if(count==13) return -1;
            keys[count++]=platform->keys[platform->key_tail];
            platform->key_tail=(unsigned char)((platform->key_tail+1)&15);
        }
        platform->key_tail=0; platform->keys[0]=0x44; platform->keys[1]=0x44;
        for(unsigned i=0;i<count;++i) platform->keys[i+2]=keys[i];
        platform->key_head=(unsigned char)(count+2);
    }
    return 0;
}

static int pause_child_present(struct SlicksAmigaPlatform *platform,
    struct SlicksAmigaPlayerMenu *m,unsigned phase)
{
    present_menu_surface(platform,m);
    if(!platform->active && slicks_amiga_platform_begin(platform,0)) return -1;
    g_slicks_diag_pause_phase=(unsigned short)phase; slicks_diag_pause_child(); return 0;
}
static int test_pause_children(struct SlicksAmigaPlatform *platform,
    struct SlicksAmigaPlayerMenu *m,struct SlicksResourceArchive *archive)
{
    /* Private diagnostic configuration: never save or alter the live setup. */
    struct SlicksConfiguration config=slicks_original_configuration;
    const struct SlicksControllersLabels labels={slicks_original_controller_key_names,
        slicks_original_controller_scans,slicks_original_controller_defaults,slicks_original_controller_exit};
    int result=-1; unsigned char *saved=AllocMem(64000,MEMF_ANY);
    if(!saved) return -1;
    for(unsigned row=1;row<=3;++row) {
        while(m->race_menu->state.row!=row) {
            slicks_race_menu_key(&m->race_menu->state,m->race_menu->state.row<row?0x50:0x48);
            if(slicks_amiga_race_menu_draw(m)) goto done;
        }
        if(slicks_amiga_race_menu_draw(m)) goto done;
        unsigned char colours[2]={m->fonts[0][6],m->fonts[1][6]};
        for(unsigned long i=0;i<64000;++i) saved[i]=m->renderer.ui.pixels[i];
        enum SlicksRaceMenuAction action=slicks_race_menu_key(&m->race_menu->state,0x1c);
        if(row==1) {
            if(action!=SLICKS_RACE_MENU_HELP || slicks_amiga_help_open(m,archive,(const unsigned char *)"")) goto done;
            if(pause_child_present(platform,m,40)) goto done;
            if(slicks_help_viewer_key(m->help,27,1) || !m->help->navigation.done) goto done;
            slicks_amiga_platform_end(platform);
            if(slicks_amiga_help_close(m) || pause_child_present(platform,m,41)) goto done;
        } else if(row==2) {
            if(action!=SLICKS_RACE_MENU_CONTROLLERS || slicks_amiga_controllers_open_at(m,archive,45,65) ||
               slicks_amiga_controllers_draw(m,&config,&labels) || pause_child_present(platform,m,42)) goto done;
            unsigned char previous=config.player_input[0];
            if(slicks_controllers_key(&m->controllers_dialog->state,&config,slicks_original_configuration.keys,0x1c) ||
               config.player_input[0]!=(previous+1)%3 || slicks_amiga_controllers_draw(m,&config,&labels) ||
               pause_child_present(platform,m,43)) goto done;
            if(slicks_controllers_key(&m->controllers_dialog->state,&config,slicks_original_configuration.keys,1) ||
               !m->controllers_dialog->state.done) goto done;
            slicks_amiga_platform_end(platform);
            if(slicks_amiga_controllers_close(m) || pause_child_present(platform,m,44)) goto done;
        } else {
            unsigned char changed=0; unsigned short timer=0; config.field_05de=100;
            if(action!=SLICKS_RACE_MENU_SPEED || slicks_amiga_race_speed_open(m,&config,&changed) || !changed ||
               pause_child_present(platform,m,45)) goto done;
            if(slicks_amiga_race_speed_key(m,&config,0x4d) || slicks_amiga_race_speed_key(m,&config,0x48) ||
               config.field_05de!=106 || pause_child_present(platform,m,46)) goto done;
            if(slicks_amiga_race_speed_key(m,&config,1) || !m->race_menu->speed.done) goto done;
            slicks_amiga_platform_end(platform);
            if(slicks_amiga_race_speed_close(m,&config,&timer) || timer!=530 || config.field_05de!=106 ||
               pause_child_present(platform,m,47)) goto done;
        }
        if(m->fonts[0][6]!=colours[0] || m->fonts[1][6]!=colours[1]) goto done;
        for(unsigned long i=0;i<64000;++i) if(saved[i]!=m->renderer.ui.pixels[i]) goto done;
        slicks_amiga_platform_end(platform);
    }
    result=0;
done:
    slicks_amiga_platform_end(platform);
    FreeMem(saved,64000); return result;
}
/* Component diagnostic, not the interactive race input loop. */
static int test_pause_surface(struct SlicksAmigaPlatform *platform,
    unsigned char *chunky,const unsigned char *palette)
{
    struct SlicksResourceArchive archive={0}; int result=-1;
    struct SlicksAmigaPlayerMenu *m=0;
    if(slicks_resource_archive_open(&archive,"SLICKS.000")) return -1;
    m=slicks_amiga_race_surface_create(&archive,chunky,palette);
    if(!m) goto done;
    g_slicks_diag_pause_menu=m;
    for(unsigned long i=0;i<64000;++i) m->saved[i]=chunky[i];
    unsigned char old_colour=m->fonts[0][6];
    for(unsigned fault=1;fault<=3;++fault) {
        g_slicks_diag_pause_fault=(unsigned char)fault;
        if(!slicks_amiga_race_menu_open(m,&archive,menu_language_name,slicks_original_race_menu_keys,0,50) ||
           m->race_menu || g_slicks_diag_pause_fault || m->fonts[0][6]!=old_colour) goto done;
        for(unsigned long i=0;i<64000;++i) if(chunky[i]!=m->saved[i]) goto done;
        g_slicks_diag_pause_phase=(unsigned short)fault; slicks_diag_pause_checkpoint();
    }
    for(unsigned repeat=0;repeat<2;++repeat) {
        if(slicks_amiga_race_menu_open(m,&archive,menu_language_name,slicks_original_race_menu_keys,
            (unsigned char)(repeat?5:0),50)) goto done;
        if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
        slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
        slicks_amiga_player_menu_clear_dirty(m);
        if(slicks_amiga_platform_begin(platform,0)) goto done;
        for(unsigned step=0;step<7;++step) {
            if(step) slicks_race_menu_key(&m->race_menu->state,(unsigned char)(repeat?0x48:0x50));
            if(slicks_amiga_race_menu_draw(m)) goto done;
            present_menu_surface(platform,m);
            g_slicks_diag_pause_phase=(unsigned short)(10+repeat*10+step);
            slicks_diag_pause_checkpoint();
        }
        slicks_amiga_platform_end(platform);
        if(test_pause_children(platform,m,&archive)) goto done;
        if(slicks_amiga_race_menu_close(m) || m->fonts[0][6]!=old_colour) goto done;
        for(unsigned long i=0;i<64000;++i) if(chunky[i]!=m->saved[i]) goto done;
        present_menu_surface(platform,m);
        if(slicks_amiga_platform_begin(platform,0)) goto done;
        g_slicks_diag_pause_phase=(unsigned short)(17+repeat*10); slicks_diag_pause_checkpoint();
        slicks_amiga_platform_end(platform);
    }
    result=0;
done:
    slicks_amiga_platform_end(platform);
    slicks_amiga_player_menu_destroy(m); g_slicks_diag_pause_menu=0;
    slicks_resource_archive_close(&archive);
    return result;
}

/* RAM-only menu transitions retain takeover. Real disk boundaries still call
 * platform_end explicitly and arrive here inactive. Publish at display blank. */
static int show_view(struct SlicksAmigaPlatform *platform,unsigned short view)
{
    if(!platform->active) return slicks_amiga_platform_begin(platform,view);
    slicks_amiga_platform_wait_display_blank(platform);
    slicks_amiga_platform_show(platform,view);
    return 0;
}
static int show_menu(struct SlicksAmigaPlatform *platform)
{
    return show_view(platform,0);
}

static int open_help(struct SlicksAmigaPlatform *platform,struct SlicksAmigaPlayerMenu *menu,const unsigned char *topic)
{
    struct SlicksResourceArchive archive={0};
    int result=slicks_resource_archive_cached(&archive,help_fail_archive?0:menu_cache);
    help_fail_archive=0;
    if(!result) result=slicks_amiga_help_open(menu,&archive,topic);
    slicks_resource_archive_close(&archive);
    if(result && slicks_amiga_help_warning_open(menu)) return -1;
    present_menu_surface(platform,menu);
    if(show_menu(platform)) return -1;
    if(result) slicks_diag_help_failed(); else slicks_diag_help_ready();
    return 0;
}

static int open_title_help(struct SlicksAmigaPlatform *platform,unsigned char *logical,unsigned char *chunky,
    const unsigned char *palette,const unsigned char *topic)
{
    struct SlicksResourceArchive archive={0};
    int result=slicks_resource_archive_cached(&archive,help_fail_archive?0:menu_cache);
    help_fail_archive=0;
    if(!result && !help_fail_surface)
        g_slicks_title_help=slicks_amiga_help_surface_create(&archive,chunky,palette);
    help_fail_surface=0;
    result=g_slicks_title_help?slicks_amiga_title_help_open(g_slicks_title_help,&archive,topic):-1;
    slicks_resource_archive_close(&archive);
    if(result) {
        slicks_amiga_player_menu_destroy(g_slicks_title_help); g_slicks_title_help=0;
        /* The title font and logical surface are already resident. This
         * recovery needs neither a Help surface nor any new allocation. */
        unsigned at=0;
        for(unsigned y=90;y<112;++y) for(unsigned x=20;x<300;++x)
            title_help_saved[at++]=logical[y*100UL+(x>>2)+((x&3)<<16)];
        clear_title_rectangle(logical,20,90,300,112);
        slicks_draw_title_text(logical,"HELP UNAVAILABLE - PRESS A KEY",160,96,15);
        slicks_title_dirty_add(&title_dirty,20,90,300,112);
        publish_title_dirty(platform,logical,chunky);
        g_slicks_title_help_warning=1;
        if(show_menu(platform)) return -1;
        slicks_diag_help_failed(); return 0;
    }
    present_menu_surface(platform,g_slicks_title_help);
    if(show_menu(platform)) return -1;
    slicks_diag_help_ready(); return 0;
}

static int present_track_clear_message(struct SlicksAmigaPlatform *platform,const unsigned char *message)
{
    if(slicks_amiga_message_open(g_slicks_options_menu,message,slicks_original_players_footer_percent)) return -1;
    present_menu_surface(platform,g_slicks_options_menu);
    if(slicks_amiga_platform_begin(platform,0)) return -1;
    slicks_diag_track_clear_ready(); return 0;
}

static const unsigned char *native_track_name(void *context,unsigned index)
{
    /* Original 35d28 with strip-extension=1: eight-byte DOS base name.
     * Keep the full filename in the IO catalogue, not in the displayed list. */
    const char (*names)[SLICKS_TRACK_NAME_SIZE]=context;
    static unsigned char display[9]; unsigned i=0;
    while(i<8 && names[index][i] && names[index][i]!='.') {
        display[i]=(unsigned char)names[index][i]; ++i;
    }
    display[i]=0; return display;
}
static int draw_track_menu(struct SlicksAmigaPlatform *platform,short total)
{
    const struct SlicksTrackMenuLabels labels={
        {slicks_original_track_actions[0],slicks_original_track_actions[1],slicks_original_track_actions[2],
         slicks_original_track_actions[3],slicks_original_track_actions[4],slicks_original_track_actions[5]},
        slicks_original_track_random_on,slicks_original_track_random_off,slicks_original_track_separator};
    g_slicks_track_menu->error=0;
    if(slicks_track_renderer_draw(&g_slicks_track_renderer,&g_slicks_track_state,total,
        &g_slicks_track_playlist,&labels) || g_slicks_track_menu->error) return -1;
    present_menu_surface(platform,g_slicks_track_menu);
    return 0;
}
static int open_track_menu(struct SlicksAmigaPlatform *platform,unsigned char *chunky,
    char names[][SLICKS_TRACK_NAME_SIZE],short total,short random_count)
{
    /* This constructor only loads trckmenu/fonts from the memory provider;
     * names and saved track lists are startup catalogues. RECORDS still
     * loads the explicitly selected track through an OS boundary. */
    struct SlicksResourceArchive archive={0};
    if(slicks_resource_archive_cached(&archive,menu_cache)) return -1;
    g_slicks_track_menu=slicks_amiga_track_menu_create(&archive,chunky,
        slicks_language_lookup(title_language,title_language_used,slicks_original_track_title,slicks_original_track_title),
        slicks_original_track_footer,total,slicks_original_players_footer_percent,
        &g_slicks_track_renderer,native_track_name,names);
    slicks_resource_archive_close(&archive);
    if(!g_slicks_track_menu) return -1;
    g_slicks_track_state.column=0; g_slicks_track_state.previous=-1;
    g_slicks_track_state.done=0; g_slicks_track_state.random_count=random_count; g_slicks_track_action=0;
    if(draw_track_menu(platform,total) || slicks_amiga_platform_set_view(platform,0,g_slicks_track_menu->palette) ||
       show_menu(platform)) return -1;
    slicks_diag_tracks_ready(); return 0;
}
struct SlicksSetupLoadReport g_slicks_track_lists_load;
struct SlicksSetupStorageReport g_slicks_track_lists_save;
__attribute__((noinline)) void slicks_diag_track_lists_ready(void) { __asm__ volatile("" ::: "memory"); }
__attribute__((noinline)) void slicks_diag_track_lists_closed(void) { __asm__ volatile("" ::: "memory"); }
/* Modal transitions are RAM-only; commits explicitly release hardware. */
static int track_lists_finish(struct SlicksAmigaPlatform *platform,short total,const char *error)
{
    slicks_amiga_track_lists_close(g_slicks_track_menu);
    g_slicks_track_state.previous=-1;
    if(draw_track_menu(platform,total)) return -1;
    if(error && slicks_amiga_message_open(g_slicks_track_menu,(const unsigned char *)error,
        slicks_original_players_footer_percent)) return -1;
    slicks_diag_track_lists_closed(); return 0;
}
static int track_lists_commit(struct SlicksAmigaPlatform *platform,short total,void *names,int remove)
{
    struct SlicksAmigaTrackLists *lists=g_slicks_track_menu->track_lists;
    /* Picker/name/confirmation work is RAM-only. Release hardware only
     * when committing the catalogue transaction to disk. */
    slicks_amiga_platform_end(platform);
    g_slicks_track_lists_save=slicks_amiga_store_track_lists(&lists->catalogue,remove,
        remove<0?lists->name:0,&g_slicks_track_playlist,total,native_track_name,names);
    const char *error=0;
    switch(g_slicks_track_lists_save.result) {
    case SLICKS_SETUP_SAVED: break;
    case SLICKS_SETUP_SAVED_CLEANUP_PENDING: error="SAVED - BACKUP REMAINS"; break;
    case SLICKS_SETUP_RECOVERY_REQUIRED: error="KEEP SLICKS.TRK NEW/BAK FILES"; break;
    default: error="SLICKS.TRK SAVE FAILED"; break;
    }
    int result=track_lists_finish(platform,total,error);
    /* Close every borrowed view before publishing a refreshed catalogue.
     * This remains part of the explicit save/delete disk boundary. */
    slicks_amiga_track_list_cache_refresh(&track_list_cache);
    return result;
}
static int track_lists_key(struct SlicksAmigaPlatform *platform,short total,void *names,
    unsigned char character,unsigned char scan,unsigned long tick)
{
    struct SlicksAmigaPlayerMenu *m=g_slicks_track_menu;
    struct SlicksAmigaTrackLists *lists=m->track_lists;
    if(m->name_dialog) {
        int done=slicks_name_dialog_key(&m->name_dialog->renderer,character);
        if(done<0 || m->error) return -1;
        m->name_dialog->hidden=0;
        if(!done) return 0;
        if(slicks_amiga_name_dialog_close(m)) return -1;
        return done==1?track_lists_commit(platform,total,names,-1):track_lists_finish(platform,total,0);
    }
    if(m->message) {
        if(!scan) return 0;
        if(slicks_amiga_message_close(m)) return -1;
        return slicks_track_list_delete_accepted(scan)?
            track_lists_commit(platform,total,names,lists->pending_delete):track_lists_finish(platform,total,0);
    }
    if(!m->picker) return -1;
    slicks_list_dialog_key(&m->picker->renderer.state,scan);
    if(!m->picker->renderer.state.done) return slicks_amiga_profile_picker_draw(m,tick);
    struct SlicksTrackListChoice choice=slicks_amiga_track_lists_choice(m,g_slicks_track_playlist.count);
    switch(choice.action) {
    case SLICKS_TRACK_LIST_LOAD:
        if(slicks_track_lists_select(&lists->catalogue,choice.index,&g_slicks_track_playlist,
            total,native_track_name,names)) return track_lists_finish(platform,total,"TRACK LIST CANNOT BE LOADED");
        return track_lists_finish(platform,total,0);
    case SLICKS_TRACK_LIST_NAME:
        lists->name[0]=0;
        return slicks_amiga_name_dialog_open_at(m,lists->name,slicks_original_track_list_name,
            185,40,slicks_original_players_footer_percent);
    case SLICKS_TRACK_LIST_CONFIRM_DELETE:
        lists->pending_delete=choice.index;
        return slicks_amiga_message_open_font(m,slicks_original_track_list_delete_question,
            slicks_original_players_footer_percent,2);
    default: return track_lists_finish(platform,total,0);
    }
}
static int draw_options_menu(struct SlicksAmigaPlatform *platform,const struct SlicksConfiguration *configuration)
{
    const struct SlicksOptionsLabels labels={slicks_original_option_labels,slicks_original_option_suffixes,
        slicks_original_mode_labels,slicks_original_options_boolean[0],slicks_original_options_boolean[1]};
    g_slicks_options_menu->error=0;
    if(slicks_options_renderer_draw(&g_slicks_options_renderer,&g_slicks_options_state,configuration,
        slicks_original_option_specs,&labels) || g_slicks_options_menu->error) return -1;
    present_menu_surface(platform,g_slicks_options_menu);
    return 0;
}
static int open_options_menu(struct SlicksAmigaPlatform *platform,unsigned char *chunky,
    const unsigned char *palette,const struct SlicksConfiguration *configuration)
{
    struct SlicksResourceArchive archive={0};
    if(slicks_resource_archive_cached(&archive,menu_cache)) return -1;
    g_slicks_options_menu=slicks_amiga_options_menu_create(&archive,chunky,palette,
        slicks_language_lookup(title_language,title_language_used,slicks_original_options_title,slicks_original_options_title),
        &g_slicks_options_renderer);
    slicks_resource_archive_close(&archive);
    if(!g_slicks_options_menu) return -1;
    g_slicks_options_state=(struct SlicksOptionsMenu){0,0,0,-1}; g_slicks_options_action=0;
    g_slicks_options_configuration=configuration;
    if(draw_options_menu(platform,configuration) || show_menu(platform)) return -1;
    slicks_diag_options_ready(); return 0;
}

static int draw_controllers_dialog(struct SlicksAmigaPlatform *platform,
    const struct SlicksConfiguration *configuration)
{
    const struct SlicksControllersLabels labels={slicks_original_controller_key_names,
        slicks_original_controller_scans,slicks_original_controller_defaults,slicks_original_controller_exit};
    if(slicks_amiga_controllers_draw(g_slicks_options_menu,configuration,&labels)) return -1;
    present_menu_surface(platform,g_slicks_options_menu);
    slicks_diag_controllers_ready(); return 0;
}
static int open_controllers_dialog(struct SlicksAmigaPlatform *platform,
    const struct SlicksConfiguration *configuration)
{
    struct SlicksResourceArchive archive={0};
    if(slicks_resource_archive_cached(&archive,menu_cache)) return -1;
    int result=slicks_amiga_controllers_open(g_slicks_options_menu,&archive);
    slicks_resource_archive_close(&archive);
    if(result || draw_controllers_dialog(platform,configuration) ||
       show_menu(platform)) return -1;
    g_slicks_options_action=SLICKS_OPTIONS_NONE;
    return 0;
}

static int open_player_menu(struct SlicksAmigaPlatform *platform,unsigned char *chunky,
    struct SlicksConfiguration *configuration,struct SlicksPlayerMenu *state)
{
    struct SlicksResourceArchive archive={0};
    const struct SlicksPlayerMenuLabels labels={slicks_original_players_random,
        slicks_original_players_random_each,{slicks_original_players_add,
        slicks_original_players_edit,slicks_original_players_delete,slicks_original_players_exit}};
    if(slicks_resource_archive_cached(&archive,menu_cache)) return -1;
    g_slicks_player_menu=slicks_amiga_player_menu_create(&archive,chunky,
        slicks_language_lookup(title_language,title_language_used,slicks_original_players_title,slicks_original_players_title),
        slicks_original_players_footer,
        slicks_original_players_footer_percent,&labels);
    slicks_resource_archive_close(&archive);
    if(!g_slicks_player_menu) return -1;
    *state=(struct SlicksPlayerMenu){0,1,0,0};
    setup_resources.profile_count=g_slicks_profiles.count;
    slicks_setup_select(&g_slicks_setup_session,configuration,&setup_resources,0);
    if(slicks_amiga_player_menu_draw(g_slicks_player_menu,0,configuration->selected_profile,
        g_slicks_setup_session.players.participation,&g_slicks_profiles) ||
        slicks_amiga_platform_set_view(platform,0,g_slicks_player_menu->palette)) return -1;
    present_player_menu(platform);
    if(show_menu(platform)) return -1;
    state->redraw=0; g_slicks_diag_player_menu_action=0; g_slicks_diag_player_menu_row=0;
    slicks_diag_player_menu_ready();
    return 0;
}

static int present_profile_editor(struct SlicksAmigaPlatform *platform)
{
    const struct SlicksProfileEditorLabels labels={
        {slicks_original_editor_rows,slicks_original_editor_rows+8,slicks_original_editor_rows+16,
         slicks_original_editor_rows+24,slicks_original_editor_rows+32,slicks_original_editor_rows+40},
        {slicks_original_editor_roles,slicks_original_editor_roles+9},slicks_original_editor_percent,
        slicks_original_editor_random,slicks_original_editor_random_each,slicks_original_editor_unavailable};
    if(slicks_amiga_profile_editor_draw(g_slicks_player_menu,&g_slicks_profiles,&labels,
        slicks_original_editor_field_01a6)) return -1;
    present_player_menu(platform);
    if(!platform->active && slicks_amiga_platform_begin(platform,0)) return -1;
    slicks_diag_profile_editor_ready(); return 0;
}

static void redraw_service_options(struct SlicksAmigaPlatform *platform,
    unsigned char *logical, unsigned char *chunky, unsigned short selection,
    short fuel, short damage)
{
    char fuel_text[] = "FUEL   000";
    char damage_text[] = "DAMAGE 000";
    unsigned short row;
    for (row = 0; row < 3; ++row) {
        fuel_text[9 - row] = (char)('0' + fuel % 10);
        damage_text[9 - row] = (char)('0' + damage % 10);
        fuel /= 10;
        damage /= 10;
    }
    clear_title_rectangle(logical, 60, 80, 260, 160);
    slicks_draw_title_text(logical, "CUSTOM OPTIONS", 160, 85, 15);
    slicks_draw_title_text(logical, fuel_text, 160, 103, 15);
    slicks_draw_title_text(logical, damage_text, 160, 116, 15);
    slicks_draw_title_text(logical, "BACK", 160, 129, 15);
    slicks_draw_title_text(logical, ">", 100, (short)(103 + selection * 13), 15);
    slicks_draw_title_text(logical, "LEFT/RIGHT TO CHANGE", 160, 145, 15);
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

static void snapshot_display(unsigned char *snapshot, const struct BitMap *bitmap)
{
    const unsigned long *source = (const unsigned long *)bitmap->Planes[0];
    unsigned long *destination = (unsigned long *)snapshot;
    unsigned long at;
    for (at = 0; at < 64000UL / 4; ++at)
        destination[at] = source[at];
}

static int audit_display(unsigned char *snapshot,
                         const struct BitMap *bitmap,
                         const struct SlicksRaceRuntime *race)
{
    const unsigned char *current = bitmap->Planes[0];
    unsigned long at;
    for (at = 0; at < 64000UL; ++at) {
        unsigned char changed = snapshot[at] ^ current[at];
        unsigned short x, y, within_row, i, bit;
        unsigned char permitted = 0;
        if (!changed)
            continue;
        y = (unsigned short)(at / 320UL);
        within_row = (unsigned short)(at - mult320[y]);
        x = (unsigned short)((within_row % 40U) * 8U);
        for (i = 0; i < race->dirty_row_count; ++i) {
            const struct SlicksDirtyRows *r = &race->dirty_rows[i];
            if (y >= r->top && y < r->bottom && x >= r->left && x < r->right) {
                permitted = 255;
                break;
            }
        }
        if (permitted != 255)
            for (i = 0; i < race->dirty_pixel_count; ++i) {
                const struct SlicksDirtyPixel *p = &race->dirty_pixels[i];
                if (p->y == y && p->x >= x && p->x < x + 8)
                    permitted |= (unsigned char)(0x80U >> (p->x - x));
            }
        changed &= (unsigned char)~permitted;
        if (!changed)
            continue;
        for (bit = 0; bit < 8; ++bit)
            if (changed & (0x80U >> bit))
                break;
        g_slicks_diag_audit_frame = race->frame_count;
        g_slicks_diag_audit_offset = at;
        g_slicks_diag_audit_x = x + bit;
        g_slicks_diag_audit_y = y;
        g_slicks_diag_audit_plane = within_row / 40U;
        g_slicks_diag_audit_before = snapshot[at];
        g_slicks_diag_audit_after = current[at];
        slicks_diag_bitmap_audit_failed();
        return -1;
    }
    /* The write-bounds check above cannot detect missing dirty regions.
     * Reuse its debug-only snapshot buffer for an independently full-frame
     * conversion, then compare every displayed byte. Production never does
     * this extra conversion and allocates no shadow for dirty tracking. */
    struct BitMap reference=*bitmap;
    for(unsigned p=0;p<8;++p)reference.Planes[p]=snapshot+p*40;
    slicks_chunky_rows_to_amiga(race->chunky,&reference,0,200);
    for(at=0;at<64000UL;++at)if(snapshot[at]!=current[at]) {
        unsigned row=at/320UL,within=at-mult320[row];
        g_slicks_diag_audit_frame=race->frame_count;
        g_slicks_diag_audit_offset=at;
        g_slicks_diag_audit_x=(within%40)*8;
        g_slicks_diag_audit_y=row;
        g_slicks_diag_audit_plane=within/40;
        g_slicks_diag_audit_before=snapshot[at];
        g_slicks_diag_audit_after=current[at];
        slicks_diag_bitmap_audit_failed();
        return -1;
    }
    return 0;
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

/* Explicit disk boundary. Allocate the actual file length, not its upper
 * bound, so preview scratch and resident menus fit together in Chip RAM. */
static unsigned char *load_plain_allocated(const char *path,unsigned long limit,
    unsigned long *size)
{
    *size=0;
    BPTR file=Open((CONST_STRPTR)path,MODE_OLDFILE);
    if(!file) return 0;
    unsigned char *bytes=0;
    LONG length=-1;
    if(Seek(file,0,OFFSET_END)>=0) length=Seek(file,0,OFFSET_CURRENT);
    if(length>0 && (unsigned long)length<limit && Seek(file,0,OFFSET_BEGINNING)>=0) {
        bytes=AllocMem((unsigned long)length,MEMF_ANY);
        if(bytes && Read(file,bytes,length)!=length) {
            FreeMem(bytes,(unsigned long)length); bytes=0;
        }
    }
    Close(file);
    if(bytes) *size=(unsigned long)length;
    return bytes;
}

static int open_track_info(struct SlicksAmigaPlayerMenu *menu,
    const char *path,const unsigned char *name)
{
    struct SlicksResourceArchive archive={0};
    unsigned long ds=0,ts=0;
    unsigned char *arena=AllocMem(65536,MEMF_ANY);
    unsigned char *dat=arena?load_plain_allocated("SLICKS.DAT",65536,&ds):0;
    unsigned char *track=dat?load_plain_allocated(path,8192,&ts):0;
    int result=-1;
    if(dat && track && !slicks_resource_archive_cached(&archive,menu_cache)) {
            result=slicks_amiga_track_info_open(menu,&archive,dat,ds,track,ts,
                name,slicks_original_players_footer_percent,slicks_original_date_separator,slicks_original_date_order,arena);
    }
    if(dat) FreeMem(dat,ds);
    if(track) FreeMem(track,ts);
    if(arena) FreeMem(arena,65536);
    slicks_resource_archive_close(&archive);
    return result;
}

static void prepare_race_palette(unsigned char *palette,
    const struct SlicksConfiguration *configuration)
{
    /* Like DS:310c, initially zero and preserved for inactive slots. */
    static unsigned char car_ramps[4][6];
    static const unsigned char yellow_ramp[5][3] = {
        {63, 61, 1}, {63, 59, 11}, {63, 57, 21},
        {63, 55, 31}, {63, 53, 41}
    };
    unsigned short shade;

    /* The DOS race setup rewrites four five-shade car slots after loading
     * peli.@p.  Recreate the interpolation rather than displaying the raw
     * resource palette, whose first twenty entries are placeholders. */
    slicks_select_profile_colours(car_ramps,configuration->selected_profile,
        g_slicks_profiles.setup,g_slicks_profiles.count,
        slicks_original_fallback_colours,configuration->options[0],
        setup_resources.override_count);
    slicks_profile_palette(palette,car_ramps);
    palette[183 * 3] = 39;
    palette[183 * 3 + 1] = 43;
    palette[183 * 3 + 2] = 10;
    for (shade = 0; shade < 5; ++shade) {
        palette[(199 + shade) * 3] = yellow_ramp[shade][0];
        palette[(199 + shade) * 3 + 1] = yellow_ramp[shade][1];
        palette[(199 + shade) * 3 + 2] = yellow_ramp[shade][2];
    }
}

static struct SlicksDriverDeviceState driver_device_state[4];
static struct SlicksConfiguration driver_device_configuration_storage;
static const struct SlicksConfiguration *driver_device_configuration;
static void poll_driver_devices(struct SlicksRaceRuntime *race,unsigned short ticks)
{
    const struct SlicksConfiguration *c=driver_device_configuration;
    /* Shop regression: press/release the configured human fire/brake key.
     * Inventory still comes solely from the ordinary buy/sell menu actions. */
    unsigned release=shop_transition_test?151:g_slicks_diag_weapon_case?850:250;
    if(shop_test && g_slicks_diag_weapon_case==9 &&
       (race->frame_count==120 || race->frame_count==121))
        slicks_driver_key(race->driver_controls,c->keys,g_slicks_setup_session.players.order,
            (unsigned char)(c->keys[4]|(race->frame_count==121?128:0)));
    if(shop_test && !shop_resume_test && shop_transition_test!=5 && (!shop_transition_test || !shop_track_position) &&
       (race->frame_count==150 || race->frame_count==release))
        slicks_driver_key(race->driver_controls,c->keys,g_slicks_setup_session.players.order,
            (unsigned char)(c->keys[1]|(race->frame_count==release?128:0)));
    for(unsigned driver=0;driver<4;++driver) {
        struct SlicksDeviceSample sample={0,0,0};
        if(race->participation[driver]<0 && c->player_input[driver])
            (void)slicks_amiga_platform_joystick(c->player_input[driver],&sample);
        (void)slicks_driver_device(&driver_device_state[driver],&race->driver_controls[driver],
            c->player_input[driver],race->participation[driver],(signed char)ticks,
            slicks_original_device_interval,c->field_062e,race->weapons_enabled,&sample);
    }
}

__attribute__((noinline)) void slicks_diag_finish_rewarded(void) { __asm__ volatile("" ::: "memory"); }
static unsigned char race_statistics_dirty;
static void award_race_finish(struct SlicksRaceRuntime *race,unsigned driver,signed char rank)
{
    (void)race;
    if(driver<4 && !slicks_finish_statistics(&g_slicks_profiles,
        g_slicks_setup_session.players.selected[driver],rank)) race_statistics_dirty=1;
    slicks_setup_finish_reward(&g_slicks_setup_session,driver,rank,slicks_original_finish_points);
    slicks_diag_finish_rewarded();
}
__attribute__((noinline)) void slicks_diag_track_rewarded(void) { __asm__ volatile("" ::: "memory"); }
static void award_race_track(struct SlicksRaceRuntime *race)
{
    signed int best_laps[4];
    for(unsigned i=0;i<4;++i) best_laps[i]=(signed int)race->cars[i].best_lap_time_units;
    slicks_setup_track_reward(&g_slicks_setup_session,best_laps,slicks_original_fastest_points);
    slicks_diag_track_rewarded();
}

static __attribute__((noinline)) int run_shop(struct SlicksAmigaPlatform *platform,unsigned char *chunky,
    struct SlicksSetupSession *session)
{
    const struct SlicksShopRules *rules=&slicks_original_shop_rules;
    unsigned char extra=(shop_test && g_slicks_diag_weapon_case>=7 && g_slicks_diag_weapon_case<=8)?1:(registration.name[0]!=0);
    unsigned buyable=0;
    for(unsigned d=0;d<4;++d) for(unsigned i=0;i<13;++i)
        if(slicks_shop_price(rules,&session->options,session->inventory[d],
            session->players.participation[d],session->players.vehicle[d],i,extra)>0)
            buyable=1;
    shop_end_game=0;
    if(!buyable) return 0;
    struct SlicksResourceArchive archive={0};
    struct SlicksAmigaPlayerMenu *m=0;
    struct SlicksShopMenu state;
    int result=-1;
    struct SlicksShopContent c={.session=session,.rules=rules,.items=slicks_original_shop_items,
        .footer=slicks_original_shop_footer,.exit_label=slicks_original_shop_exit,
        .register_label=slicks_original_shop_register,.separator=slicks_original_track_separator,
        .extra=extra,
        .track=(short)(shop_track_position+1),.total=shop_track_total?shop_track_total:(short)g_slicks_track_playlist.count};
    for(unsigned d=0;d<4;++d) if(session->players.participation[d]) {
        short p=session->players.selected[d];
        if(p<0 || p>=g_slicks_profiles.count) goto done;
        c.names[d]=g_slicks_profiles.names[p];
    }
    if(slicks_resource_archive_cached(&archive,menu_cache)) goto done;
    m=slicks_amiga_shop_create(&archive,chunky,&c,&state);
    if(!m) goto done;
    slicks_shop_computers(rules,&session->options,session->inventory,session->cash,
        session->players.participation,session->players.vehicle,c.extra,&session->random_state);
    if(slicks_amiga_shop_draw(m,&c,&state)) goto done;
    if(state.driver<0) { result=0; goto done; }
    if(slicks_amiga_platform_set_view(platform,0,m->palette)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    slicks_amiga_player_menu_clear_dirty(m);
    platform->key_tail=platform->key_head;
    if(slicks_amiga_platform_begin(platform,0)) goto done;
    g_slicks_shop_menu=&state; slicks_diag_shop_ready();
    if(mode_transition_test) {
        platform->key_tail=0; platform->keys[0]=0x45; platform->key_head=1;
    }
    if(shop_test) {
        static const unsigned char keys[]={0x20,0x4c,0x4d,0x4c,0x4e,0x4f,0x41,0x5f,0x5f,0x44,0x44,0x41,0x50,0x45,0x45};
        platform->key_tail=0;
        if(g_slicks_diag_weapon_case) {
            unsigned n=0;
            if(!shop_resume_test && (!shop_transition_test || !shop_track_position)) {
                if(g_slicks_diag_weapon_case==9) {
                    platform->keys[n++]=0x44;platform->keys[n++]=0x44;
                    platform->keys[n++]=0x4d;
                } else for(unsigned i=1;i<g_slicks_diag_weapon_case;++i) platform->keys[n++]=0x4d;
                platform->keys[n++]=0x44;platform->keys[n++]=0x44;
            }
            platform->keys[n++]=shop_transition_test==3?0x59:0x45;
            platform->key_head=(unsigned char)n;
        } else {
            for(unsigned i=0;i<sizeof keys;++i) platform->keys[i]=keys[i];
            platform->key_head=sizeof keys;
        }
    }
    while(!state.done) {
        unsigned short raw;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&raw)) {
            if(raw&128) continue;
            /* Classic keyboards have no Scroll Lock. Help is the shop-only
             * capture shortcut; F1 retains the original help viewer. */
            unsigned char scan=raw==0x5f?70:(unsigned char)amiga_raw_to_menu_scan(raw);
            if(m->help_warning) {
                if(slicks_amiga_help_warning_close(m)) goto done;
                present_menu_surface(platform,m);
                continue;
            }
            if(m->help) {
                if(slicks_help_viewer_key(m->help,scan==1?27:0,scan)) goto done;
                if(m->help->navigation.done) {
                    if(slicks_amiga_help_close(m)) goto done;
                    if(shop_test) g_slicks_shop_help_phase=2;
                    present_menu_surface(platform,m);
                    if(show_menu(platform)) goto done;
                } else present_menu_surface(platform,m);
                continue;
            }
            signed char old_driver=state.driver,old_row=state.row;
            enum SlicksShopAction action=slicks_shop_key(&state,session->players.participation,scan);
            if(action==SLICKS_SHOP_HELP) {
                if(slicks_amiga_help_open(m,&archive,slicks_original_shop_help) &&
                    slicks_amiga_help_warning_open(m)) goto done;
                if(shop_test && m->help) g_slicks_shop_help_phase=1;
                present_menu_surface(platform,m);
                if(show_menu(platform)) goto done;
            } else if(action==SLICKS_SHOP_CAPTURE) {
                /* File I/O is performed only while AmigaOS owns the machine. */
                slicks_amiga_platform_end(platform);
                struct SlicksSetupStorageReport saved=slicks_amiga_store_capture(chunky,m->palette);
                if(saved.result!=SLICKS_SETUP_SAVED && slicks_amiga_warning_open(m,
                    (const unsigned char *)"SCREEN CAPTURE FAILED - PRESS A KEY")) goto done;
                present_menu_surface(platform,m);
                if(slicks_amiga_platform_begin(platform,0)) goto done;
            } else if(action==SLICKS_SHOP_BUY || action==SLICKS_SHOP_SELL) {
                int it=slicks_shop_item(rules,&session->options,session->inventory[0],session->players.participation[0],
                    session->players.vehicle[0],c.extra,state.row);
                if(it<0 || state.driver<0 || state.driver>=4) goto done;
                unsigned d=(unsigned)state.driver;
                unsigned changed;
                if(action==SLICKS_SHOP_BUY)
                    changed=slicks_shop_buy(rules,&session->options,session->inventory[d],&session->cash[d],
                        session->players.participation[d],session->players.vehicle[d],it,c.extra);
                else changed=slicks_shop_sell(rules,&session->options,session->inventory[d],&session->cash[d],
                    session->players.participation[d],session->players.vehicle[d],it,c.extra);
                if(shop_test) { ++g_slicks_shop_test_phase; slicks_diag_shop_ready(); }
                if(!changed) continue;
            }
            if(action==SLICKS_SHOP_REDRAW || action==SLICKS_SHOP_BUY || action==SLICKS_SHOP_SELL) {
                signed char refresh_driver=(signed char)(state.driver+1),refresh_row=-1;
                if(action==SLICKS_SHOP_REDRAW) {
                    if(state.driver==old_driver && state.row==old_row) continue;
                    if(state.driver!=old_driver) {
                        refresh_driver=-1; refresh_row=(signed char)(state.row+1);
                    }
                }
                if(slicks_amiga_shop_refresh(m,&c,&state,refresh_driver,refresh_row)) goto done;
                present_menu_surface(platform,m);
            }
        }
    }
    shop_end_game=state.end_game; result=0;
done:
    slicks_amiga_platform_end(platform);
    g_slicks_shop_menu=0;
    slicks_amiga_player_menu_destroy(m); slicks_resource_archive_close(&archive);
    return result;
}

static void audit_weapon_hud(const struct SlicksRaceRuntime *race)
{
    if(!shop_test || !g_slicks_diag_weapon_case || g_slicks_diag_weapon_case>9) return;
    signed char selected=race->selected_weapon[0];
    unsigned weapon=g_slicks_diag_weapon_case==9?(unsigned)(selected<0?weapon_hud_last_selection:selected):(unsigned)g_slicks_diag_weapon_case-1U;
    if(weapon>=8) return;
    short count=race->weapon_inventory[0][weapon+5];
    if(count==weapon_hud_last_count && selected==weapon_hud_last_selection) return;
    /* Compare the actual post-update pixels with the independently verified
     * original HUD layout. Empty selection retains the previous icon. */
    unsigned width=selected<0?0:(unsigned)(count*20/race->weapon_capacity[weapon+5]);
    for(unsigned x=0;x<20;++x) {
        unsigned char expected=x<width?race->weapon_hud_colour:race->status_colours[0];
        if(race->chunky[mult320[187]+106+x]!=expected) ++g_slicks_diag_weapon_hud_failures;
    }
    const struct SlicksHudIcon *icon=&race->hud_weapon_icons[weapon];
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<16;++x) {
        unsigned char expected=race->hud_background[mult320[y+8]+90+x];
        if(x && x<=icon->width && y<icon->height && icon->pixels[y*icon->width+x-1])
            expected=icon->pixels[y*icon->width+x-1];
        if(race->chunky[mult320[y+192]+90+x]!=expected) ++g_slicks_diag_weapon_hud_failures;
    }
    weapon_hud_last_count=count;weapon_hud_last_selection=selected;
    ++g_slicks_diag_weapon_hud_checks;slicks_diag_weapon_hud_checked();
}

static int prepare_race(struct SlicksAmigaPlatform *platform,
                      unsigned char *logical, unsigned char *chunky,
                      unsigned short *mode_state,
                      struct SlicksRaceRuntime *race,
                      const char *track_path,
                      unsigned char *race_palette, unsigned short vehicle,
                      struct SlicksConfiguration *configuration,
                      struct SlicksSetupSession *session,unsigned char new_game)
{
    race_checkpoint(0);
    struct SlicksRaceOptions options;
    /* A failed GO must not consume random choices or reset the user's game
     * state. Static backups avoid adding this transaction to the 4 KiB stack.
     * Preparation is synchronous and never reentrant. */
    static struct SlicksSetupSession saved_session;
    static struct SlicksConfiguration saved_configuration;
    static unsigned char saved_requests[4];
    g_slicks_diag_race_error=0;
    if(configuration->options[0]<0 || configuration->options[0]>=6) {
        g_slicks_diag_race_error=7;
        return -1;
    }
    if(session) {
        saved_session=*session; saved_configuration=*configuration;
        for(unsigned i=0;i<4;++i) saved_requests[i]=setup_vehicle_requests[i];
    }
    if(session && new_game) {
        shop_track_position=0;
        session->players.vehicle[0]=(signed char)vehicle;
        slicks_setup_new_game(session,configuration,&setup_resources,
            slicks_original_mode_flags[configuration->options[0]],
            1); /* GO prepares one track; original 2633c sets this to one. */
    }
    slicks_resolve_race_options(&options,configuration,
        slicks_original_mode_flags[configuration->options[0]]);
    if(session) session->options=options;
    struct SlicksResourceArchive archive = {0};
    struct SlicksTrackNavigation *navigation = 0;
    unsigned char *dat = 0;
    unsigned char *track = 0;
    unsigned char *arena = 0;
    unsigned char *car_resource = 0;
    unsigned char *font_resource = 0;
    long dat_size;
    long masks_size;
    long track_size;
    unsigned short car;
    unsigned short direction;
    int result = -1;

    if(session && run_shop(platform,chunky,session)) {
        g_slicks_diag_race_error=9; goto cleanup;
    }

    dat = (unsigned char *)AllocMem(65536UL, MEMF_ANY);
    track = (unsigned char *)AllocMem(8192UL, MEMF_ANY);
    arena = (unsigned char *)AllocMem(65536UL, MEMF_ANY);
    navigation = (struct SlicksTrackNavigation *)
        AllocMem(sizeof(*navigation), MEMF_ANY);
    car_resource = (unsigned char *)AllocMem(128UL, MEMF_ANY);
    /* Exercise partial-allocation cleanup without exhausting system memory. */
    if(g_slicks_diag_race_load_fault==1) g_slicks_diag_race_load_fault=0;
    else font_resource = (unsigned char *)AllocMem(2048UL, MEMF_ANY);
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
    slicks_race_set_timer(race,slicks_speed_timer_argument(configuration->field_05de));
    if(session) {
        race->finish_reward=award_race_finish;
        race->track_reward=title_demo.active?0:award_race_track;
    }
    slicks_race_set_mode(race,configuration->options[0],configuration->options[13]);
    if(slicks_race_set_demo(race,title_demo.active || demo_render_only?-1:0,
        slicks_original_demo_overlay)) {
        g_slicks_diag_race_error=7; goto cleanup;
    }
    if(session) {
        for(unsigned driver=0;driver<4;++driver) {
            if(session->players.participation[driver]<0 && configuration->player_input[driver]>2) {
                g_slicks_diag_race_error=7;
                goto cleanup; /* PC/LPT adapters are not Amiga game ports. */
            }
            driver_device_state[driver]=(struct SlicksDriverDeviceState){0,0,0,0};
        }
        driver_device_configuration_storage=*configuration;
        driver_device_configuration=&driver_device_configuration_storage;
        race->poll_driver_devices=poll_driver_devices;
    }
    slicks_race_set_vehicle(race,0,vehicle);
    if(session)
        for(unsigned driver=0;driver<4;++driver) {
            if(session->players.vehicle[driver]<0 ||
                session->players.vehicle[driver]>=SLICKS_VEHICLE_COUNT) goto cleanup;
            slicks_race_set_vehicle(race,driver,(unsigned char)session->players.vehicle[driver]);
            short profile=session->players.selected[driver];
            if(profile>=0 && profile<g_slicks_profiles.count)
                race->cars[driver].position_scale=g_slicks_profiles.setting[profile];
            if(session->saved_position_scale_valid)
                race->cars[driver].position_scale=session->saved_position_scale[driver];
        }
    slicks_race_set_service_options(race,options.fuel,options.damage);
    race->car_collisions_disabled=slicks_car_collisions_disabled(options.car_collisions);
    race->weapons_enabled=(unsigned char)(options.weapons_enabled!=0);
    race->weapons.rules=slicks_original_weapon_rules;
    for(unsigned item=0;item<13;++item) race->weapon_capacity[item]=slicks_original_item_capacity[item];
    if(session && slicks_race_set_participation(race,session->players.participation)) {
        g_slicks_diag_race_error=7;
        goto cleanup;
    }
    if(session && slicks_race_set_inventory(race,session->inventory)) {
        g_slicks_diag_race_error=7;
        goto cleanup;
    }
    race_checkpoint(1);
    dat_size = load_plain_file("SLICKS.DAT", dat, 65536UL);
    if(g_slicks_diag_race_load_fault==2) {
        g_slicks_diag_race_load_fault=0;
        track_size=load_plain_file("TRACKS/__MISSING__.SS",track,8192UL);
    } else track_size = load_plain_file(track_path, track, 8192UL);
    if (dat_size <= 0 || track_size <= 0) {
        g_slicks_diag_race_error = 2;
        goto cleanup;
    }
    if(slicks_race_set_track_info(race,track_path,track,(unsigned long)track_size)) {
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
    prepare_race_palette(race_palette,configuration);
    slicks_race_set_status_palette(race,race_palette);
    race_checkpoint(3);
    if (slicks_setup_basic_mode(logical, mode_state) != 0) {
        g_slicks_diag_race_error = 4;
        goto cleanup;
    }
    race_checkpoint(4);
    if (slicks_build_track_visuals(logical, race->material_map,
                                 race->surface_map, dat,
                                 (unsigned long)dat_size, track,
                                 (unsigned long)track_size, arena, 65536UL,
                                 navigation, race->fuel_option, race->damage_scale) <= 0) {
        g_slicks_diag_race_error = 5;
        goto cleanup;
    }
    race_checkpoint(5);

    race->track_actors_ready=0;
    if(slicks_decode_track_actor_assets(dat,(unsigned long)dat_size,arena,65536UL,
        race->track_actor_assets)) {
        g_slicks_diag_race_error=5;
        goto cleanup;
    }
    race->track_actors_ready=1;
    for(unsigned i=0;i<205;++i) race->track_flag_styles[i]=slicks_original_track_flag_styles[i];

    /* The /masks resource is independent of the visible DAT palette.
     * Reuse the DAT allocation after scenery decoding, and the sprite arena
     * after drawing. No extra persistent allocation is needed. */
    masks_size = slicks_resource_archive_load(&archive, "masks", dat, 65536UL);
    if (masks_size <= 0 || slicks_build_track_masks(
            race->material_map, race->surface_map, dat, (unsigned long)masks_size,
            track, (unsigned long)track_size, arena, 65536UL,
            race->fuel_option != 0 || race->damage_scale != 0,
            track[6 + 0x165 + 4], navigation) < 0) {
        g_slicks_diag_race_error = 5;
        goto cleanup;
    }

    race_checkpoint(10);
    race->navigation = *navigation;
    race->damage_enabled = navigation->service_available;
    g_slicks_diag_track_zones = navigation->zone_count;
    g_slicks_diag_material_checksum =
        checksum_material_map(race->material_map);
    g_slicks_diag_surface_checksum =
        checksum_surface_map(race->surface_map);
    {
        unsigned short material;
        unsigned long pixel;
        for (material = 0; material < 32; ++material)
            g_slicks_diag_material_count[material] = 0;
        for (pixel = 0; pixel < SLICKS_TRACK_MATERIAL_SIZE; ++pixel)
            ++g_slicks_diag_material_count[race->material_map[pixel] & 31];
    }
    {
        race_checkpoint(11);
        const char *hud_name="alamenu.@I";
        if(g_slicks_diag_race_load_fault==6) {
            g_slicks_diag_race_load_fault=0; hud_name="missing-race-hud";
        }
        long hud_size=slicks_resource_archive_load(&archive,hud_name,dat,65536UL);
        if(hud_size<=0 || slicks_race_add_hud_background(race,dat,(unsigned long)hud_size)) {
            g_slicks_diag_race_error=6;
            goto cleanup;
        }
        /* Original 19dd8 loads kirj into DS:0680; ddc0/2adbe use that
         * slot, not the smaller help font in DS:0684. Reuse the existing
         * scratch allocation rather than enlarge the icon buffer. */
        long font_size = slicks_resource_archive_load(
            &archive, SLICKS_RACE_FONT_NAME, dat, 65536UL);
        if (font_size <= 0 ||
            slicks_race_add_font(race, dat,
                                 (unsigned long)font_size) != 0) {
            g_slicks_diag_race_error = 6;
            goto cleanup;
        }
        static const char *const weapon_icons[8]={"vir5.@I","vir6.@I",
            "vir7.@I","vir8.@I","vir9.@I","vir10.@I","vir11.@I","vir12.@I"};
        for(unsigned weapon=0;weapon<8;++weapon) {
            long size=slicks_resource_archive_load(&archive,weapon_icons[weapon],
                                                   font_resource,2048UL);
            if(size<=0 || slicks_race_add_weapon_icon(race,weapon,font_resource,
                                                      (unsigned long)size)) {
                g_slicks_diag_race_error=6;
                goto cleanup;
            }
        }
    }
    if(race->weapons_enabled) {
        static const char *const assets[SLICKS_WEAPON_ASSET_COUNT]={
            "miina.ase","aikabomb.ase","flam_raj.@I",
            "ohjus.1","ohjus.2","ohjus.3","ohjus.4","ohjus.5","ohjus.6","ohjus.7","ohjus.8",
            "savu.1","savu.2","savu.3","rajahdys.1","rajahdys.2","rajahdys.3","rajahdys.4","flash.@I"};
        for(unsigned asset=0;asset<SLICKS_WEAPON_ASSET_COUNT;++asset) {
            long size=slicks_resource_archive_load(&archive,assets[asset],font_resource,2048UL);
            if(size<=0 || slicks_race_add_weapon_asset(race,asset,font_resource,(unsigned long)size)) {
                g_slicks_diag_race_error=6;goto cleanup;
            }
        }
        race->weapons.ready=1;
    }
    race_checkpoint(12);
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
    if(weapon_hud_fixture) set_weapon_hud_fixture(race);
    if(session) race->random_state=session->random_state;
    if (slicks_race_start(race, logical, chunky) != 0) {
        g_slicks_diag_race_error = 7;
        goto cleanup;
    }
    slicks_race_prepare_car_render_cache(race);
    if(session) {
        /* Original 1fd72..1fdc6 runs even with Weapons disabled. Do not
         * reseed or skip the computer's draw when its inventory is empty. */
        for(unsigned driver=0;driver<4;++driver)
            race->selected_weapon[driver]=slicks_initial_weapon(race->weapon_inventory[driver],
                session->players.participation[driver],&race->random_state);
    }
    if (slicks_race_draw_status(race,logical,status_clock.ticks)<0) {
        g_slicks_diag_race_error=9;
        goto cleanup;
    }
    race->profile_frame = g_slicks_diag_target_frame;
    audit_weapon_hud(race);
    race->profile_marker = slicks_diag_profile_race;
    race_checkpoint(7);

    if (slicks_amiga_platform_set_view(platform, 1, race_palette) != 0) {
        g_slicks_diag_race_error = 8;
        goto cleanup;
    }
    race_checkpoint(8);
    slicks_chunky_rows_to_amiga(chunky, platform->views[1].bitmap,0,200);
    slicks_race_use_chunky_surface(race);
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
#ifdef SLICKS_SHADOW_CHECK
    if (!result) {
        /* These buffers compare gameplay updates, not loading. Allocate
         * after releasing temporary assets so the 2 MiB renderer checks
         * cannot silently lose their 64 KiB comparison surface. */
        extern unsigned char *slicks_shadow_state,*slicks_shadow_chunky;
        extern volatile unsigned long slicks_shadow_sites;
        slicks_shadow_sites=SLICKS_SHADOW_SITES;
        if (!slicks_shadow_state)
            slicks_shadow_state = AllocMem(57344, MEMF_ANY);
        if ((SLICKS_SHADOW_SITES & (32|64)) && !slicks_shadow_chunky)
            slicks_shadow_chunky = AllocMem(64000, MEMF_ANY);
        if (!slicks_shadow_state ||
            ((SLICKS_SHADOW_SITES & (32|64)) && !slicks_shadow_chunky)) {
            g_slicks_diag_race_error=1; result=-1;
        }
    }
#endif
    if (result != 0) {
        if(session) {
            *session=saved_session; *configuration=saved_configuration;
            for(unsigned i=0;i<4;++i) setup_vehicle_requests[i]=saved_requests[i];
        }
        slicks_diag_frame_ready();
    }
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

static void sync_chunky_to_logical(const unsigned char *chunky,
                                   unsigned char *logical)
{
    unsigned short y;
    for (y = 0; y < 200; ++y) {
        unsigned short group;
        for (group = 0; group < 80; ++group) {
            unsigned long chunky_at =
                mult320[y] + (unsigned long)group * 4UL;
            unsigned long logical_at =
                (unsigned long)y * 100UL + group;
            logical[logical_at] = chunky[chunky_at];
            logical[0x10000UL + logical_at] = chunky[chunky_at + 1];
            logical[0x20000UL + logical_at] = chunky[chunky_at + 2];
            logical[0x30000UL + logical_at] = chunky[chunky_at + 3];
        }
    }
}

static void update_race_diagnostics(const struct SlicksRaceRuntime *race)
{
    unsigned short car;
    unsigned short sample;
    unsigned short progress = race->frame_count &&
        (((service_menu_test || completion_watch) && race->frame_count % 600 == 0) ||
         (service_menu_test && (g_slicks_diag_lap[3] != race->cars[3].lap ||
          g_slicks_diag_service[3] != race->cars[3].ai_service_state)));
    g_slicks_diag_race_frame = race->frame_count;
    g_slicks_diag_skidmarks = race->skidmark_count;
    g_slicks_diag_particles = race->trail_particle_count;
    g_slicks_diag_collisions = race->collision_count;
    if (service_menu_test && race->collision_impact > g_slicks_diag_damage_peak_impact)
        g_slicks_diag_damage_peak_impact = race->collision_impact;
    g_slicks_diag_track_collisions = race->track_collision_count;
    g_slicks_diag_countdown_stage = race->countdown_stage;
    g_slicks_diag_start_light_visible = race->start_light_visible;
    g_slicks_diag_start_light_stage_mask = race->start_light_stage_mask;
    for (sample = 0; sample < SLICKS_SOUND_SAMPLE_COUNT; ++sample)
        g_slicks_diag_sound_event_totals[sample] =
            race->sound_event_totals[sample];
    for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
        if (service_menu_test || fuel_race_test) {
            const struct SlicksRaceCar *state = &race->cars[car];
            if (state->damage[0] > (short)g_slicks_diag_damage_peak[car])
                g_slicks_diag_damage_peak[car] = state->damage[0];
            if (state->effective_surface == 30 && state->measured_speed < 300 &&
                state->damage[0] < previous_damage[car])
                ++g_slicks_diag_repair_frames[car];
            previous_damage[car] = state->damage[0];
        }
        if (fuel_race_test || service_menu_test) {
            const struct SlicksRaceCar *state = &race->cars[car];
            if (state->ai_service_state == -1 &&
                g_slicks_diag_service[car] != -1)
                ++g_slicks_diag_service_requests[car];
            if (race->frame_count && state->effective_surface == 30 &&
                (signed int)state->fuel > (signed int)g_slicks_diag_fuel[car])
                ++g_slicks_diag_refuel_frames[car];
            if (g_slicks_diag_service[car] > 0 && !state->ai_service_state &&
                g_slicks_diag_refuel_frames[car] && !state->finished &&
                state->ai_target_x == -1 && !state->ai_route_seen &&
                state->damage[0] < 10)
                ++g_slicks_diag_service_departures[car];
            if (race->frame_count && state->effective_surface == 30)
                ++g_slicks_diag_pit_frames[car];
            g_slicks_diag_fuel_surface[car] = state->effective_surface;
            g_slicks_diag_fuel_layer[car] = state->actor_layer;
            g_slicks_diag_fuel_speed[car] = state->measured_speed;
            g_slicks_diag_fuel_target_x[car] = state->ai_target_x;
            g_slicks_diag_fuel_target_y[car] = state->ai_target_y;
            g_slicks_diag_service[car] = state->ai_service_state;
            g_slicks_diag_fuel[car] = state->fuel;
            g_slicks_diag_fuel_capacity[car] = state->fuel_capacity;
        }
        g_slicks_diag_car_x[car] = race->cars[car].x;
        g_slicks_diag_car_y[car] = race->cars[car].y;
        g_slicks_diag_car_vx[car] = race->cars[car].velocity_x;
        g_slicks_diag_car_vy[car] = race->cars[car].velocity_y;
        g_slicks_diag_timer[car] = race->cars[car].elapsed_centiseconds;
        g_slicks_diag_speed[car] = race->cars[car].speed;
        {
            long x = race->cars[car].x / 100;
            long y = race->cars[car].y / 100;
            g_slicks_diag_material[car] =
                x >= 0 && x < 320 && y >= 0 && y < 190
                    ? race->material_map[mult320[y] +
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
    if (progress)
        slicks_diag_race_progress();
}

static int update_race_key(struct SlicksRaceRuntime *race,
                           unsigned short raw,const struct SlicksConfiguration *configuration)
{
    if(race->participation_ready) {
        unsigned short scan=amiga_raw_to_dos_scan(raw);
        if(!scan) return 0;
        slicks_driver_key(race->driver_controls,configuration->keys,
            g_slicks_setup_session.players.order,(unsigned char)(scan|(raw&128)));
        return 1;
    }
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

/* Blocking original menu boundary: no race step executes while it is open.
 * Resources are loaded only with AmigaOS restored. The race bitmap stays
 * untouched in view 1; all modal conversion targets view 0. */
static void pause_live_checkpoint(struct SlicksAmigaPlatform *platform,unsigned char diagnostic,unsigned *step)
{
    slicks_diag_pause_live_ready();
    if(championship_test) {
        static const unsigned char keys[]={0x44};
        championship_test_keys(platform,keys,1);
    }
    if(diagnostic==8) {
        static const unsigned char keys[]={0x4d,0x44,0x44,0x44,0x45,0x4d,0x44,0x44,0x44,0x44,0x44,0x45,0x45};
        if(*step==1) g_slicks_diag_help_fail_allocation=1;
        if(*step==6) g_slicks_diag_controllers_fault=1;
        if(*step==8) g_slicks_diag_controllers_fault=2;
        if(*step<sizeof keys) {
            platform->key_tail=0; platform->keys[0]=keys[(*step)++]; platform->key_head=1;
        }
        return;
    }
    if(diagnostic==7) {
        platform->key_tail=0; platform->keys[0]=0x44; platform->key_head=1;
        return;
    }
    if(diagnostic) {
        static const unsigned char keys[]={0x4d,0x44,0x45,0x4d,0x44,0x44,0x45,0x4d,0x44,0x4e,0x4c,0x45,0x45};
        if(*step<sizeof keys) {
            platform->key_tail=0; platform->keys[0]=keys[(*step)++]; platform->key_head=1;
        }
    }
}
static int pause_unavailable_notice(struct SlicksAmigaPlatform *platform,struct SlicksRaceRuntime *race,
    unsigned char *chunky,const unsigned char *palette,unsigned char diagnostic)
{
    /* Race HUD font and the shared warning save-under are already resident.
     * Even archive/surface allocation failure needs no further resources. */
    if(!race->font.ready || slicks_amiga_emergency_warning_open(chunky,palette,race->font.runtime,
        (const unsigned char *)"MENU UNAVAILABLE - PRESS A KEY")) return -1;
    int result=-1;
    if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    if(show_menu(platform)) goto done;
    platform->key_tail=platform->key_head;
    slicks_diag_pause_warning_ready();
    if(diagnostic) {
        platform->key_tail=0; platform->keys[0]=0x44; platform->key_head=1;
    }
    for(;;) {
        unsigned short raw;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&raw))
            if(!(raw&128) && raw<0x60 && amiga_raw_to_dos_scan(raw)) { result=0; goto done; }
    }
done:
    /* Both the warning save-under and the race view are resident. The
     * caller switches back to view 1 without releasing the display. */
    if(slicks_amiga_emergency_warning_close()) result=-1;
    return result;
}
/* Target resource/lifetime gate, separate from the interactive race loop.
 * Real fonts, icons and track data; private driver choices never get saved. */
static int test_intermission_surface(struct SlicksAmigaPlatform *platform,
    unsigned char *chunky,const unsigned char *palette,unsigned char *saved)
{
    struct SlicksResourceArchive archive={0}; struct SlicksAmigaPlayerMenu *m=0;
    unsigned long ds=0,ts=0;
    unsigned char *dat=0,*track=0;
    int result=-1;
    struct SlicksProfileSelection players={.participation={-1,0,1,-1},
        .vehicle={1,7,3,4},.selected={0,1,2,3},.count=3};
    struct SlicksSetupProfile profiles[4]={{0}};
    unsigned long seed=1234;
    const struct SlicksIntermissionContent content={.roles={-1,0,1,-1},.vehicles={1,7,3,4},
        .points={3,0,-1,8},.laps={100,0,200,100},.fastest=100,.track_index=0,.track_total=2,
        .names={(const unsigned char *)"FIRST",0,(const unsigned char *)"THIRD",(const unsigned char *)"FOURTH"},
        .labels={(const unsigned char *)"CHANGE CARS",(const unsigned char *)"SAVE GAME",
            (const unsigned char *)"NEXT TRACK",(const unsigned char *)"MAIN MENU"},
        .track_name=(const unsigned char *)"BASIC",.slash=(const unsigned char *)"/"};
    if(!saved || slicks_resource_archive_cached(&archive,menu_cache)) goto done;
    dat=load_plain_allocated("SLICKS.DAT",65536,&ds);
    track=load_plain_allocated("TRACKS/BASIC.SS",8192,&ts);
    if(!dat || !track) goto done;
    m=slicks_amiga_intermission_surface_create(&archive,chunky,palette);
    if(!m) goto done;
    g_slicks_diag_intermission_menu=m;
    unsigned char colour=m->fonts[0][6];
    for(unsigned long i=0;i<64000;++i) saved[i]=chunky[i];
    for(unsigned fault=1;fault<=3;++fault) {
        g_slicks_diag_intermission_fault=(unsigned char)fault;
        if(!slicks_amiga_intermission_open(m,&content,palette,dat,ds,track,ts) || m->intermission ||
           m->fonts[0][6]!=colour || g_slicks_diag_intermission_fault ||
           g_slicks_diag_intermission_fault_reached!=fault) goto done;
        for(unsigned long i=0;i<64000;++i) if(saved[i]!=chunky[i]) goto done;
        ++g_slicks_diag_intermission_phase; slicks_diag_intermission_checkpoint();
    }
    for(unsigned repeat=0;repeat<2;++repeat) {
        if(slicks_amiga_intermission_open(m,&content,palette,dat,ds,track,ts)) goto done;
        if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
        /* View zero previously held the title, not this race background. */
        slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
        present_menu_surface(platform,m);
        if(slicks_amiga_platform_begin(platform,0)) goto done;
        ++g_slicks_diag_intermission_phase; slicks_diag_intermission_checkpoint();
        slicks_amiga_platform_end(platform);
        for(unsigned long i=0;i<64000;++i) saved[i]=chunky[i];
        unsigned char menu_colour=m->fonts[0][6];
        for(unsigned fault=1;fault<=3;++fault) {
            g_slicks_diag_change_cars_fault=(unsigned char)fault;
            if(slicks_amiga_change_cars_open(m,&archive,&players,profiles,10,
                slicks_original_vehicle_weights,&seed,1,(const unsigned char *)"CARS")!=-1 ||
                m->change_cars || m->fonts[0][6]!=menu_colour || seed!=1234 ||
                g_slicks_diag_change_cars_fault_reached!=fault) goto done;
            for(unsigned long i=0;i<64000;++i) if(saved[i]!=chunky[i]) goto done;
            ++g_slicks_diag_intermission_phase; slicks_diag_intermission_checkpoint();
        }
        for(unsigned edit=0;edit<2;++edit) {
            if(slicks_amiga_intermission_key(m,60)!=SLICKS_INTERMISSION_CHANGE_CARS ||
               slicks_amiga_change_cars_open(m,&archive,&players,profiles,10,
                slicks_original_vehicle_weights,&seed,1,(const unsigned char *)"CARS")!=1) goto done;
            signed char before=players.vehicle[2];
            if(slicks_amiga_change_cars_key(m,80) || slicks_amiga_change_cars_key(m,77) ||
               players.vehicle[2]!=(before+1)%10 || players.vehicle[1]!=7 ||
               slicks_amiga_change_cars_key(m,edit?28:1)!=1 || slicks_amiga_change_cars_close(m) ||
               slicks_amiga_intermission_refresh_cars(m,players.vehicle) || seed!=1234 ||
               m->intermission->content.vehicles[2]!=players.vehicle[2]) goto done;
            for(unsigned i=0;i<4;++i) if(profiles[i].vehicle) goto done;
            present_menu_surface(platform,m);
            if(slicks_amiga_platform_begin(platform,0)) goto done;
            ++g_slicks_diag_intermission_phase; slicks_diag_intermission_checkpoint();
            slicks_amiga_platform_end(platform);
        }
        if(slicks_amiga_intermission_key(m,repeat?68:67) ||
           m->intermission->state.exit_code!=(repeat?2:1) || slicks_amiga_intermission_close(m) ||
           m->fonts[0][6]!=colour) goto done;
        for(unsigned long i=0;i<64000;++i) if(m->saved[i]!=chunky[i]) goto done;
        present_menu_surface(platform,m);
        if(slicks_amiga_platform_begin(platform,0)) goto done;
        ++g_slicks_diag_intermission_phase; slicks_diag_intermission_checkpoint();
        slicks_amiga_platform_end(platform);
    }
    result=0;
done:
    slicks_amiga_platform_end(platform);
    slicks_amiga_player_menu_destroy(m); g_slicks_diag_intermission_menu=0;
    slicks_resource_archive_close(&archive);
    if(dat) FreeMem(dat,ds);
    if(track) FreeMem(track,ts);
    return result;
}

/* Original 25937..25965: caller already refreshed selections exactly once.
 * Preview is the NEXT track, while the header counts the completed track.
 * Return 1=next, 2=end; never rerun profile selection after child edits. */
static int intermission_retry_notice(struct SlicksAmigaPlatform *platform,struct SlicksRaceRuntime *race,
    unsigned char *chunky,const unsigned char *palette,unsigned char diagnostic)
{
    if(!race->font.ready || slicks_amiga_emergency_warning_open(chunky,palette,race->font.runtime,
        (const unsigned char *)"ENTER: RETRY / ESC: END MATCH")) return -1;
    int result=-1;
    if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    if(slicks_amiga_platform_begin(platform,0)) goto done;
    platform->key_tail=platform->key_head;
    slicks_diag_intermission_retry();
    if(diagnostic) { platform->key_tail=0; platform->keys[0]=diagnostic==4?0x45:0x44; platform->key_head=1; }
    for(;;) {
        unsigned short raw;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&raw)) {
            if(raw==0x44) { result=1; goto done; }
            if(raw==0x45) { result=2; goto done; }
        }
    }
done:
    if(slicks_amiga_emergency_warning_close()) result=-1;
    return result;
}
static void intermission_test_key(struct SlicksAmigaPlatform *platform,unsigned diagnostic,unsigned step)
{
    /* Ordinary raw-key queue only: no mutation of driver or dialog state. */
    static const unsigned char edits[]={0x51,0x4e,0x4d,0x4e,0x45,0x51,0x4e,0x44,0x58};
    if(diagnostic==2 && step<sizeof edits) {
        platform->key_tail=0; platform->keys[0]=edits[step]; platform->key_head=1;
    } else if((diagnostic==1 || diagnostic==3) && !step) {
        platform->key_tail=0; platform->keys[0]=0x58; platform->key_head=1;
    }
}
__attribute__((noinline)) void slicks_diag_intermission_closed(void) { __asm__ volatile("" ::: "memory"); }
static int run_intermission(struct SlicksAmigaPlatform *platform,struct SlicksRaceRuntime *race,
    unsigned char *chunky,const unsigned char *palette,const char *next_path,
    const unsigned char *next_name,short position,short total,unsigned char diagnostic,
    void *track_names,unsigned track_count)
{
    struct SlicksResourceArchive archive={0}; struct SlicksAmigaPlayerMenu *m=0;
    unsigned char *dat=0,*track=0,*language=0;
    unsigned long ds=0,ts=0;
    int result=-1; unsigned used=0,test_step=0;
    struct SlicksIntermissionContent content={.track_index=position,.track_total=total,
        .track_name=next_name,.slash=(const unsigned char *)"/"};
    if(diagnostic==3 || diagnostic==4) g_slicks_diag_intermission_fault=3;
retry:
    slicks_amiga_platform_end(platform);
    /* Synchronous menu owner; keep decode staging off the default 4K stack. */
    static unsigned char language_resource[512];
    language=AllocMem(2048,MEMF_ANY);
    if(!language || slicks_resource_archive_cached(&archive,menu_cache)) goto unavailable;
    long size=slicks_resource_archive_load(&archive,menu_language_name,language_resource,sizeof language_resource);
    if(size<0 || slicks_language_table_load(language_resource,(unsigned)size,language,2048,&used)) goto unavailable;
    for(unsigned i=0;i<4;++i) {
        content.roles[i]=g_slicks_setup_session.players.participation[i];
        content.vehicles[i]=g_slicks_setup_session.players.vehicle[i];
        content.points[i]=g_slicks_setup_session.points[i];
        content.laps[i]=(signed int)race->cars[i].best_lap_time_units;
        short selected=g_slicks_setup_session.players.selected[i];
        if(content.roles[i] && (selected<0 || selected>=g_slicks_profiles.count)) goto unavailable;
        content.names[i]=content.roles[i]?g_slicks_profiles.names[selected]:0;
        content.labels[i]=slicks_intermission_resolve_label(language,used,i,slicks_original_intermission_keys[i]);
    }
    short fastest=29999;
    for(unsigned i=0;i<4;++i) if((signed int)fastest>content.laps[i]) fastest=(short)(unsigned short)content.laps[i];
    content.fastest=fastest;
    dat=load_plain_allocated("SLICKS.DAT",65536,&ds);
    track=load_plain_allocated(next_path,8192,&ts);
    if(!dat || !track) goto unavailable;
    m=slicks_amiga_intermission_surface_create(&archive,chunky,palette);
    if(!m || slicks_amiga_intermission_open(m,&content,palette,dat,ds,track,ts)) goto unavailable;
    /* Release preview-only data before taking over hardware. */
    FreeMem(dat,ds); dat=0; FreeMem(track,ts); track=0; FreeMem(language,2048); language=0;
    if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    slicks_amiga_player_menu_clear_dirty(m);
    platform->key_tail=platform->key_head;
    if(slicks_amiga_platform_begin(platform,0)) goto done;
    g_slicks_diag_intermission_menu=m;
    slicks_diag_intermission_checkpoint();
    if(championship_test==1 || championship_test==3 || championship_test==6) {
        /* Original hidden Save route: F2, close Change Cars, Down, Enter.
         * Up from NEXT TRACK must not expose the removed native extension. */
        static const unsigned char keys[]={0x51,0x45,0x4d,0x44};
        championship_test_keys(platform,keys,sizeof keys);
    }
    intermission_test_key(platform,diagnostic,test_step);
    while(!m->intermission->state.exit_code) {
        unsigned short raw;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&raw)) {
            if(raw&128) continue;
            unsigned char scan=(unsigned char)amiga_raw_to_dos_scan(raw);
            if(!scan) continue;
            if(m->help_warning) {
                if(slicks_amiga_help_warning_close(m)) goto done;
            } else if(m->change_cars) {
                int key=slicks_amiga_change_cars_key(m,scan);
                if(key<0) goto done;
                if(key) {
                    if(slicks_amiga_change_cars_close(m) ||
                       slicks_amiga_intermission_refresh_cars(m,g_slicks_setup_session.players.vehicle)) goto done;
                }
            } else {
                int action=slicks_amiga_intermission_key(m,scan);
                if(action<0) goto done;
                if(action==SLICKS_INTERMISSION_CHANGE_CARS) {
                    int opened=slicks_amiga_change_cars_open(m,&archive,&g_slicks_setup_session.players,
                        g_slicks_profiles.setup,setup_resources.vehicle_count,setup_resources.vehicle_weights,
                        &g_slicks_setup_session.random_state,1,slicks_original_change_cars_title);
                    if(opened<=0 && slicks_amiga_intermission_refresh_cars(m,g_slicks_setup_session.players.vehicle)) goto done;
                    if(opened<0 && slicks_amiga_warning_open(m,(const unsigned char *)"CARS UNAVAILABLE - PRESS A KEY")) goto done;
                } else if(action==SLICKS_INTERMISSION_SAVE_GAME) {
                    static unsigned char saved_tracks[256][8];
                    struct SlicksSavedGame game; unsigned char scales[4];
                    for(unsigned i=0;i<4;++i) {
                        scales[i]=race->cars[i].position_scale;
                        for(unsigned j=0;j<13;++j) g_slicks_setup_session.inventory[i][j]=race->weapon_inventory[i][j];
                    }
                    if(slicks_championship_export(&game,saved_tracks,track_selection,g_slicks_track_playlist.count,
                        position+1,track_count,native_track_name,track_names,&g_slicks_setup_session,&g_slicks_profiles,scales)) {
                        if(championship_notice(platform,m,(const unsigned char *)"CHAMPIONSHIP CANNOT BE SAVED")<0) goto done;
                    } else if(run_saved_game_dialog(platform,m,&game,saved_tracks,1)<0) goto done;
                }
            }
            if(diagnostic==2) {
                g_slicks_diag_intermission_phase=(unsigned short)++test_step;
                slicks_diag_intermission_input();
            }
            if(m->intermission->state.exit_code) break;
            present_menu_surface(platform,m);
            if(!platform->active && slicks_amiga_platform_begin(platform,0)) goto done;
            intermission_test_key(platform,diagnostic,test_step);
        }
    }
    result=m->intermission->state.exit_code;
    goto done;
unavailable:
    /* Release the failed attempt before the allocation-free retry notice.
     * Retry never returns through rewards, profile refresh or playlist advance. */
    slicks_amiga_player_menu_destroy(m); m=0;
    slicks_resource_archive_close(&archive);
    if(dat) { FreeMem(dat,ds); dat=0; }
    if(track) { FreeMem(track,ts); track=0; }
    if(language) { FreeMem(language,2048); language=0; }
    result=intermission_retry_notice(platform,race,chunky,palette,diagnostic);
    if(result==1) { result=-1; goto retry; }
done:
    slicks_amiga_player_menu_destroy(m); g_slicks_diag_intermission_menu=0;
    slicks_resource_archive_close(&archive);
    if(dat) FreeMem(dat,65536);
    if(track) FreeMem(track,8192);
    if(language) FreeMem(language,2048);
    if(result>=0) {
        if(platform->active) {
            slicks_amiga_platform_wait_display_blank(platform);
            slicks_amiga_platform_show(platform,1);
        } else if(slicks_amiga_platform_begin(platform,1)) result=-1;
    }
    slicks_diag_intermission_closed();
    return result;
}

volatile unsigned short g_slicks_diag_record_results_phase;
volatile struct SlicksRecordOutcome g_slicks_diag_record_outcome;
volatile struct SlicksTrackRecords g_slicks_diag_record_table;
volatile struct SlicksSetupStorageReport g_slicks_diag_record_save;
__attribute__((noinline)) void slicks_diag_record_results_ready(void) { __asm__ volatile("" ::: "memory"); }
static int result_wait(struct SlicksAmigaPlatform *,unsigned,unsigned char);
unsigned char g_slicks_diag_record_faults,g_slicks_diag_record_skip;
__attribute__((noinline)) void slicks_diag_record_recovery_ready(void) { __asm__ volatile("" ::: "memory"); }
/* Platform recovery, deliberately distinct from original game screens.
 * No allocation or file access while the machine is taken over. */
static int record_retry_notice(struct SlicksAmigaPlatform *platform,struct SlicksRaceRuntime *race,
    unsigned char *chunky,const unsigned char *palette,const unsigned char *message,unsigned char diagnostic)
{
    if(!race->font.ready || slicks_amiga_emergency_warning_open(chunky,palette,race->font.runtime,message)) return -1;
    int result=-1;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    if(slicks_amiga_platform_set_view(platform,0,palette) || slicks_amiga_platform_begin(platform,0)) goto done;
    platform->key_tail=platform->key_head;
    slicks_diag_record_recovery_ready();
    if(diagnostic) {
        unsigned char skip=(g_slicks_diag_record_skip==1 && g_slicks_diag_record_results_phase==5) ||
            (g_slicks_diag_record_skip==2 && g_slicks_diag_record_results_phase==4);
        platform->key_tail=0; platform->keys[0]=skip?0x45:0x44; platform->key_head=1;
    }
    for(;;) {
        unsigned short key;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&key)) {
            if(key==0x44) { result=1; goto done; }
            if(key==0x45) { result=0; goto done; }
        }
    }
done:
    if(slicks_amiga_emergency_warning_close()) result=-1;
    return result;
}

__attribute__((noinline)) void slicks_diag_record_results_closed(void) { __asm__ volatile("" ::: "memory"); }
/* Original 255ff..25934. Run once, before selection refresh can change the
 * profile/vehicle associated with a completed lap. Never reinsert on Retry. */
static int run_record_results(struct SlicksAmigaPlatform *platform,
    struct SlicksRaceRuntime *race,unsigned char *chunky,const unsigned char *palette,
    const char *path,unsigned char diagnostic)
{
    struct SlicksResourceArchive archive={0};
    struct SlicksAmigaPlayerMenu *m=0;
    unsigned char *bytes=0;
    struct SlicksTrackRecords records={0};
    struct SlicksRecordEntrant entrants[4];
    struct ClockData date;
    int result=-1;
    slicks_amiga_platform_end(platform);
    struct UtilityBase *UtilityBase=(struct UtilityBase *)OpenLibrary((CONST_STRPTR)"utility.library",37);
    if(!UtilityBase) goto done;
    struct DateStamp now; DateStamp(&now);
    Amiga2Date((unsigned long)now.ds_Days*86400UL+(unsigned long)now.ds_Minute*60UL+
        (unsigned long)now.ds_Tick/TICKS_PER_SECOND,&date);
    CloseLibrary((struct Library *)UtilityBase);
load_records:
    slicks_amiga_platform_end(platform);
    records=(struct SlicksTrackRecords){0};
    bytes=AllocMem(8192,MEMF_ANY);
    long size=bytes?load_plain_file(g_slicks_diag_record_faults&1?"missing-post-race-track":path,bytes,8192):-1;
    g_slicks_diag_record_faults&=(unsigned char)~1;
    if(size<0 || size>=8192 || slicks_track_records(bytes,(unsigned long)size,&records)<0) {
        if(bytes) { FreeMem(bytes,8192); bytes=0; }
        g_slicks_diag_record_results_phase=4;
        int choice=record_retry_notice(platform,race,chunky,palette,
            (const unsigned char *)"RECORD READ FAILED: ENTER RETRY / ESC SKIP",diagnostic);
        if(choice>0) goto load_records;
        result=choice; goto done;
    }
    for(unsigned i=0;i<4;++i) {
        short profile=g_slicks_setup_session.players.selected[i];
        if(profile<0 || profile>=g_slicks_profiles.count) goto done;
        entrants[i]=(struct SlicksRecordEntrant){
            .role=g_slicks_setup_session.players.participation[i],
            .vehicle=g_slicks_setup_session.players.vehicle[i],
            .setting=g_slicks_profiles.setting[profile],.name=g_slicks_profiles.names[profile],
            .best_lap=(signed int)race->cars[i].best_lap_time_units,
            .engine=race->weapon_inventory[i][0],.tyres=race->weapon_inventory[i][1]};
    }
    struct SlicksRecordOutcome outcome=slicks_post_race_records(&records,entrants,
        date.year<1997?0:(unsigned char)date.mday,(unsigned char)date.month,date.year);
    g_slicks_diag_record_outcome=outcome;
    g_slicks_diag_record_table=records;
    g_slicks_diag_record_results_phase=1; slicks_diag_record_results_ready();
    if(outcome.show) {
        if(slicks_resource_archive_cached(&archive,menu_cache)) goto done;
        m=slicks_amiga_race_surface_create(&archive,chunky,palette);
        if(!m || slicks_amiga_records_icons_load(m,&archive)) goto done;
        for(unsigned long i=0;i<64000;++i) m->saved[i]=chunky[i];
        unsigned char table[256];
        slicks_ui_tint_table(palette,table,10,10,30,75);
        if(slicks_ui_remap(&m->renderer.ui,35,75,270,180,table) ||
           slicks_amiga_records_draw(m,&records,outcome.ranks,40,55,
                slicks_original_date_separator,slicks_original_date_order)) goto done;
        /* Race rendering used view 1. View 0 can still contain a previous
         * menu: initialize its background as well as the records overlay. */
        slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
        slicks_amiga_player_menu_clear_dirty(m);
        if(slicks_amiga_platform_set_view(platform,0,palette) ||
           slicks_amiga_platform_begin(platform,0)) goto done;
        platform->key_tail=platform->key_head;
        g_slicks_diag_record_results_phase=2; slicks_diag_record_results_ready();
        if(result_wait(platform,300,diagnostic)) goto done;
        for(unsigned long i=0;i<64000;++i) chunky[i]=m->saved[i];
    }
    if(outcome.changed) {
        for(;;) {
            slicks_amiga_platform_end(platform);
            unsigned char changed;
            char obstruction[80]; unsigned at=0;
            unsigned char own_obstruction=0;
            if(g_slicks_diag_record_faults&2) {
                g_slicks_diag_record_faults&=(unsigned char)~2;
                while(path[at] && at<75) { obstruction[at]=path[at]; ++at; }
                if(path[at]) goto done;
                obstruction[at++]='.'; obstruction[at++]='n'; obstruction[at++]='e'; obstruction[at++]='w'; obstruction[at]=0;
                BPTR lock=CreateDir((CONST_STRPTR)obstruction);
                if(!lock) goto done;
                UnLock(lock); own_obstruction=1;
            }
            g_slicks_diag_record_save=slicks_amiga_store_track_records(path,&records,&changed);
            /* Remove only the empty directory this diagnostic just created.
             * Real recovery files are never removed by the UI. */
            if(own_obstruction && !DeleteFile((CONST_STRPTR)obstruction)) goto done;
            /* Old-format tracks are an intentional original no-op, not a
             * failed write requiring an endless Retry prompt. */
            if(g_slicks_diag_record_save.result==SLICKS_SETUP_SAVED) break;
            g_slicks_diag_record_results_phase=5;
            int choice=record_retry_notice(platform,race,chunky,palette,
                (const unsigned char *)(changed?"RECORDS SAVED; BACKUP KEPT. ENTER / ESC":
                    "RECORD SAVE FAILED: ENTER RETRY / ESC SKIP"),diagnostic);
            if(choice<0) goto done;
            if(changed || !choice) break;
        }
    }
    g_slicks_diag_record_results_phase=3; slicks_diag_record_results_ready();
    result=0;
done:
    slicks_amiga_player_menu_destroy(m);
    slicks_resource_archive_close(&archive);
    if(bytes) FreeMem(bytes,8192);
    if(!result) {
        /* Records and recovery notices only paint view 0. The unchanged
         * race bitmap in view 1 is already current; do not reconvert it. */
        if(slicks_amiga_platform_set_view(platform,1,palette)) result=-1;
        else if(platform->active) {
            slicks_amiga_platform_wait_display_blank(platform);
            slicks_amiga_platform_show(platform,1);
        } else if(slicks_amiga_platform_begin(platform,1)) result=-1;
    }
    slicks_diag_record_results_closed();
    return result;
}

volatile unsigned short g_slicks_diag_standings_phase;
struct SlicksChampionshipStandings g_slicks_diag_standings;
struct SlicksAmigaPlayerMenu *g_slicks_diag_standings_menu;
__attribute__((noinline)) void slicks_diag_standings_ready(void) { __asm__ volatile("" ::: "memory"); }

/* Both bitmaps contain the same native image. Build only the inactive
 * copper list, then swap at blanking: palette fades never modify live
 * copper instructions while the beam can fetch them. */
static int result_fade(struct SlicksAmigaPlatform *platform,const unsigned char *palette,
    short start,short end,short duration,unsigned short timer,unsigned short *view)
{
    /* Single modal owner: keep palette work off the 4 KiB CLI stack,
     * which also carries build_copper's 256-colour conversion. */
    static unsigned char scaled[768];
    short ticks=slicks_fade_duration(timer,duration),step=0;
    unsigned long phase=0,period=slicks_timer_divisor(timer)*50UL;
    do {
        unsigned long before=platform->vblank_count;
        slicks_fade_palette(scaled,palette,slicks_fade_weight(start,end,ticks,step));
        unsigned short next=(unsigned short)(*view^1);
        if(slicks_amiga_platform_set_view(platform,next,scaled)) return -1;
        slicks_amiga_platform_wait_display_blank(platform);
        slicks_amiga_platform_show(platform,next); *view=next;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) return -1;
        unsigned elapsed=0;
        unsigned long frames=platform->vblank_count-before;
        while(frames--) elapsed+=slicks_physics_clock_advance(&phase,period,0);
        /* At 50% the original IRQ is slower than PAL. Like 37a00, wait
         * for an actual clock change rather than advancing on a zero tick. */
        while(!elapsed) {
            slicks_amiga_platform_wait_vblank(platform);
            if(g_slicks_diag_force_exit) return -1;
            elapsed=slicks_physics_clock_advance(&phase,period,0);
        }
        step=slicks_fade_advance(step,ticks,(unsigned short)elapsed);
    } while(step<=ticks);
    return 0;
}

struct ResultWaitContext { struct SlicksAmigaPlatform *platform; short key; unsigned reads; unsigned char diagnostic; };
static short result_wait_key(void *context)
{
    struct ResultWaitContext *c=context;
    if(c->diagnostic && c->reads==1) {
        c->platform->key_tail=0; c->platform->keys[0]=0x40; c->platform->key_head=1;
    }
    ++c->reads;
    unsigned short raw;
    if(g_slicks_diag_force_exit) return 128;
    if(slicks_amiga_platform_poll_key(c->platform,&raw)) {
        unsigned short scan=amiga_raw_to_dos_scan(raw&127);
        if(scan) c->key=(short)(scan|(raw&128));
    } else if(c->key<128) slicks_amiga_platform_wait_vblank(c->platform);
    return c->key;
}
static unsigned char result_wait_button(void *context)
{
    (void)context;
    struct SlicksDeviceSample a={0},b={0};
    (void)slicks_amiga_platform_joystick(1,&a);
    (void)slicks_amiga_platform_joystick(2,&b);
    return (unsigned char)!!(g_slicks_diag_force_exit || a.buttons || b.buttons);
}
static void result_wait_delay(void *context,unsigned short ms)
{
    struct ResultWaitContext *c=context;
    for(unsigned frame=0;frame<ms/20;++frame) slicks_amiga_platform_wait_vblank(c->platform);
}
static void result_wait_clear(void *context)
{
    struct ResultWaitContext *c=context;
    c->platform->key_tail=c->platform->key_head;
}
static int result_wait(struct SlicksAmigaPlatform *platform,unsigned limit,unsigned char diagnostic)
{
    platform->key_tail=platform->key_head;
    struct ResultWaitContext context={platform,128,0,diagnostic};
    const struct SlicksResultWaitOps ops={result_wait_key,result_wait_button,result_wait_delay,result_wait_clear,&context};
    (void)slicks_result_wait((short)limit,0,&ops);
    return g_slicks_diag_force_exit?-1:0;
}

volatile unsigned short g_slicks_registration_screen;
/* Explicit REGCHECKY/F input fixtures; never set by a normal launch. */
static unsigned char registration_help_test;
__attribute__((noinline)) void slicks_diag_registration_screen_ready(void) { __asm__ volatile("" ::: "memory"); }
__attribute__((noinline)) void slicks_diag_registration_help_closed(void) { __asm__ volatile("" ::: "memory"); }
static void registration_delay(struct SlicksAmigaPlatform *p,unsigned milliseconds)
{
    unsigned long start=p->vblank_count;
    while(p->vblank_count-start<(milliseconds+19)/20 && !g_slicks_diag_force_exit)
        slicks_amiga_platform_wait_vblank(p);
}
/* Original 36d8b: keyboard release then next make, or a 20000-ms timeout.
 * Unlike results waiting this does not accept a joystick button. */
static short registration_wait(struct SlicksAmigaPlatform *p)
{
    unsigned long start=p->vblank_count;
    unsigned short held=0,raw;
    /* Discard prior make/release events and release the entry key before
     * accepting a new one, matching the original keyboard-state poll. */
    while(slicks_amiga_platform_poll_key(p,&raw))
        if(raw<0xe0) held=(raw&128)?0:(unsigned short)(raw+1);
    if(registration_help_test && (g_slicks_registration_screen==2 || g_slicks_registration_screen==3)) {
        unsigned char key=registration_help_test==1?0x15:0x50; /* Amiga Y/F1 */
        championship_test_keys(p,&key,1);
    }
    while(!g_slicks_diag_force_exit) {
        while(slicks_amiga_platform_poll_key(p,&raw)) {
            if(raw&128) { if((raw&127)+1==held) held=0; continue; }
            if(held) continue;
            if(!(raw&128) && raw<0x60) {
                unsigned short scan=amiga_raw_to_dos_scan(raw);
                if(scan) { p->key_tail=p->key_head;return (short)scan; }
            }
        }
        if(!held && p->vblank_count-start>=1000) return 0;
        slicks_amiga_platform_wait_vblank(p);
    }
    return -1;
}
void __attribute__((noinline)) slicks_diag_registration_warning_ready(void) { __asm__ volatile("" ::: "memory"); }
static int registration_help_unavailable(struct SlicksAmigaPlatform *p,unsigned char *chunky,
    const unsigned char *palette)
{
    struct SlicksMenuRect bounds;
    if(slicks_amiga_emergency_warning_open(chunky,palette,slicks_title_small_font,
        (const unsigned char *)"HELP UNAVAILABLE - PRESS A KEY")) return -1;
    int result=-1;
    if(slicks_amiga_emergency_warning_bounds(&bounds)) goto done;
    bounds.left=(short)(bounds.left&~15);
    bounds.right=(short)((bounds.right+15)&~15);
    /* Both views already contain the registration image; publish only the
     * warning in view 0, then restore that same rectangle on dismissal. */
    slicks_amiga_platform_wait_display_blank(p);
    slicks_chunky_rect_to_amiga(chunky,p->views[0].bitmap,bounds.left,bounds.top,bounds.right,bounds.bottom,0);
    if(slicks_amiga_platform_set_view(p,0,palette) || show_menu(p)) goto done;
    slicks_diag_registration_warning_ready();
    if(registration_help_test) {
        static const unsigned char keys[]={0x44,0xc4};
        championship_test_keys(p,keys,sizeof keys);
    }
    while(!g_slicks_diag_force_exit) {
        unsigned short key;slicks_amiga_platform_wait_vblank(p);
        while(slicks_amiga_platform_poll_key(p,&key))
            if(!(key&128)) { result=0;goto done; }
    }
done:
    if(slicks_amiga_emergency_warning_close()) return -1;
    if(!result) {
        slicks_amiga_platform_wait_display_blank(p);
        slicks_chunky_rect_to_amiga(chunky,p->views[0].bitmap,bounds.left,bounds.top,bounds.right,bounds.bottom,0);
    }
    return result;
}
static int registration_exit_help(struct SlicksAmigaPlatform *p,unsigned char *chunky,
    const unsigned char *palette)
{
    struct SlicksResourceArchive a={0};struct SlicksAmigaPlayerMenu *m=0;int result=-1;
    if(slicks_resource_archive_cached(&a,registration_help_test==4?0:menu_cache)) goto unavailable;
    if(registration_help_test!=5) m=slicks_amiga_help_surface_create(&a,chunky,palette);
    if(!m) goto unavailable;
    if(registration_help_test==3) g_slicks_diag_help_fail_allocation=1;
    if(open_help(p,m,slicks_registration_help_topic)) goto done;
    if(registration_help_test==6) {
        if(!m->help || !m->help->chapter_length) goto done;
        /* Exercise the real parser failure on the next navigation redraw. */
        m->help->chapter[0]=0;
    }
    if(registration_help_test) {
        static const unsigned char keys[]={0x4d,0xcd,0x4c,0xcc,0x45,0xc5};
        championship_test_keys(p,keys,sizeof keys);
    }
    while(!g_slicks_diag_force_exit) {
        unsigned short raw;slicks_amiga_platform_wait_vblank(p);
        while(slicks_amiga_platform_poll_key(p,&raw)) {
            unsigned char character=slicks_amiga_menu_character(m,(unsigned char)(raw&127));
            if(raw&128) continue;
            if(m->help_warning) { result=0;goto done; }
            struct SlicksAmigaHelpKey key=slicks_amiga_help_key((unsigned char)raw,character);
            if(!(key.ascii || key.scan)) continue;
            if(slicks_help_viewer_key(m->help,key.ascii,key.scan)) {
                if(slicks_amiga_help_close(m)) goto done;
                present_menu_surface(p,m);
                goto unavailable;
            }
            if(m->help->navigation.done) { result=0;goto done; }
            present_menu_surface(p,m);
        }
    }
    goto done;
unavailable:
    result=registration_help_unavailable(p,chunky,palette);
done:
    if(m && m->help && slicks_amiga_help_close(m)) result=-1;
    if(m && m->help_warning && slicks_amiga_help_warning_close(m)) result=-1;
    /* Help only paints view 0. Publish its restored bounds before destroying
     * the painter; view 1 still contains the original registration image. */
    if(m && !result) present_menu_surface(p,m);
    slicks_amiga_player_menu_destroy(m);
    slicks_resource_archive_close(&a);
    slicks_diag_registration_help_closed();return result;
}
/* Original registration-dependent presentation. Real archive BMPs and fonts,
 * never captured frames. kind 0=expired trial, 1=exit, 2=optional order form. */
extern short slicks_menu_measure(const unsigned char *,const unsigned char *);
static struct {
    struct SlicksChunkyUi ui;
    struct SlicksMenuRect rectangles[16];
    unsigned short count;
} registration_painter;
static void registration_dirty(void *context,short l,short t,short r,short b)
{
    (void)context;
    slicks_menu_dirty_add(registration_painter.rectangles,&registration_painter.count,l,t,r,b);
}
static void registration_text(void *context,const unsigned char *text,short x,short y,unsigned char flags)
{
    struct SlicksChunkyUi *ui=context;
    slicks_records_text(ui->pixels,slicks_title_small_font,text,x,y,flags,0);
    slicks_font_text_dirty(ui,slicks_title_small_font,text,x,y,1,flags,
        slicks_menu_measure(slicks_title_small_font,text),0);
}
__attribute__((noinline)) void slicks_diag_registration_prompt_ready(void) { __asm__ volatile("" ::: "memory"); }
static int registration_screen(struct SlicksAmigaPlatform *p,unsigned char *chunky,
    unsigned kind,unsigned short timer)
{
    struct SlicksResourceArchive a={0};unsigned char *resource=0;
    static unsigned char palette[768];static const unsigned char black[768]={0};
    unsigned w,h;unsigned short view=0;int result=-1;
    unsigned char old_colour=slicks_title_small_font[6];
    const char *name=kind==0?"loading.bmp":kind==2?"webf_ord.bmp":
        slicks_registration_exit_image(registration.name[0]);
    slicks_amiga_platform_end(p);
    if(slicks_resource_archive_open(&a,"SLICKS.000")) goto done;
    resource=AllocMem(70000,MEMF_ANY);
    if(!resource) goto done;
    long length=slicks_resource_archive_load(&a,name,resource,70000);
    if(length<0) {
        /* The original '/name' resolver also permits an external BMP. The
         * supplied archive has no webf_ord.bmp; absence is optional in DOS. */
        BPTR file=Open((CONST_STRPTR)name,MODE_OLDFILE);
        if(file) { length=Read(file,resource,70000);Close(file); }
    }
    if(length<0) { result=kind==0?-1:0;goto done; }
    if(slicks_decode_menu_bitmap(resource,(unsigned long)length,chunky,palette,&w,&h,0) || w!=320 || h!=200) goto done;
    FreeMem(resource,70000);resource=0;slicks_resource_archive_close(&a);
    struct SlicksChunkyUi ui={chunky,palette,0,0};
    registration_painter.ui=(struct SlicksChunkyUi){chunky,palette,registration_dirty,0};
    registration_painter.count=0;
    if(kind==0) {
        slicks_registration_trial_background(&ui);
        slicks_title_small_font[6]=slicks_ui_nearest(&ui,70,70,70);
        slicks_registration_trial_text(slicks_original_registration_text,0,registration_text,&registration_painter.ui);
    }
    for(unsigned i=0;i<2;++i) slicks_chunky_rows_to_amiga(chunky,p->views[i].bitmap,0,200);
    if(slicks_amiga_platform_set_view(p,0,black) || slicks_amiga_platform_begin(p,0)) goto done;
    if(result_fade(p,palette,0,100,kind?5:3,timer,&view)) goto done;
    g_slicks_registration_screen=(unsigned short)(kind==0?1:kind==2?4:registration.name[0]?3:2);
    slicks_diag_registration_screen_ready();
    if(kind==0) {
        registration_delay(p,2000);
        registration_painter.count=0;
        slicks_registration_trial_text(slicks_original_registration_text,1,registration_text,&registration_painter.ui);
        slicks_amiga_platform_wait_display_blank(p);
        for(unsigned i=0;i<2;++i) for(unsigned n=0;n<registration_painter.count;++n) {
            const struct SlicksMenuRect *r=&registration_painter.rectangles[n];
            slicks_chunky_rect_to_amiga(chunky,p->views[i].bitmap,r->left,r->top,r->right,r->bottom,0);
        }
        slicks_diag_registration_prompt_ready();
    } else if(kind==1 && !registration.name[0]) registration_delay(p,300);
    short key=registration_wait(p);
    if(key<0) goto done;
    if(kind==1 && slicks_registration_help_requested(key)) {
        if(registration_exit_help(p,chunky,palette)) goto done;
        if(slicks_amiga_platform_set_view(p,0,palette) || show_menu(p)) goto done;
        view=0;
    }
    if(kind && result_fade(p,palette,100,0,10,timer,&view)) goto done;
    if(kind==1) registration_delay(p,300);
    result=0;
done:
    slicks_amiga_platform_end(p);slicks_title_small_font[6]=old_colour;
    if(resource) FreeMem(resource,70000);
    slicks_resource_archive_close(&a);g_slicks_registration_screen=0;return result;
}

/* 25965..259d7 / 2a63e..2aad5. This owns the actual archive bitmap and
 * font, not a captured DOS screen. Profile statistics are published once,
 * after successful resource loading/drawing and before the original wait. */
__attribute__((noinline)) void slicks_diag_standings_io(void) { __asm__ volatile("" ::: "memory"); }
__attribute__((noinline)) void slicks_diag_standings_closed(void) { __asm__ volatile("" ::: "memory"); }
static int run_championship_results(struct SlicksAmigaPlatform *platform,unsigned char *chunky,
    const unsigned char *race_palette,unsigned short timer,unsigned char *setup_dirty,unsigned char diagnostic)
{
    struct SlicksResourceArchive archive={0};
    struct SlicksAmigaPlayerMenu *m=0;
    unsigned char *bitmap=0;
    static unsigned char palette[768];
    static const unsigned char black[768]={0};
    unsigned short view=0;
    int result=-1,nonzero=0;
    for(unsigned i=0;i<4;++i) nonzero|=g_slicks_setup_session.points[i];
    /* The record/intermission owner returns on view 1. Prepare view 0,
     * switch, then populate the now-inactive second bitmap for palette fades. */
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    if(slicks_amiga_platform_set_view(platform,0,race_palette) || show_menu(platform)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[1].bitmap,0,200);
    if(result_fade(platform,race_palette,100,0,2,timer,&view)) goto done;
    if(!nonzero) { result=0; goto done; }
    slicks_diag_standings_io();
    slicks_amiga_platform_end(platform);
    if(slicks_resource_archive_open(&archive,"SLICKS.000")) goto done;
    bitmap=AllocMem(64003,MEMF_ANY);
    if(!bitmap || slicks_resource_archive_load(&archive,"sskuppi.@I",bitmap,64003)!=64003 ||
       bitmap[0]!=1 || bitmap[1]!=64 || bitmap[2]!=200 ||
       slicks_resource_archive_load(&archive,"sskuppi.@p",palette,768)!=768) goto done;
    for(unsigned long i=0;i<64000;++i) chunky[i]=bitmap[i+3];
    FreeMem(bitmap,64003); bitmap=0;
    slicks_resource_archive_close(&archive);
    if(slicks_resource_archive_cached(&archive,menu_cache)) goto done;
    m=slicks_amiga_help_surface_create(&archive,chunky,palette);
    slicks_resource_archive_close(&archive);
    if(!m) goto done;
    const unsigned char *names[4];
    for(unsigned i=0;i<4;++i) {
        short p=g_slicks_setup_session.players.selected[i];
        if(p<0 || p>=g_slicks_profiles.count) goto done;
        names[i]=g_slicks_profiles.names[p];
    }
    slicks_championship_standings(&g_slicks_diag_standings,g_slicks_setup_session.points,
        g_slicks_setup_session.players.participation);
    slicks_amiga_standings_draw(m,&g_slicks_diag_standings,g_slicks_setup_session.players.colours,names);
    g_slicks_diag_standings_menu=m;
    g_slicks_diag_standings_phase=1; slicks_diag_standings_ready();
    if(slicks_championship_statistics(&g_slicks_profiles,g_slicks_setup_session.players.selected,
        &g_slicks_diag_standings)) goto done;
    *setup_dirty=1;
    for(unsigned i=0;i<2;++i) slicks_chunky_rows_to_amiga(chunky,platform->views[i].bitmap,0,200);
    if(slicks_amiga_platform_set_view(platform,0,black) || slicks_amiga_platform_begin(platform,0)) goto done;
    view=0;
    if(result_fade(platform,palette,0,100,4,timer,&view)) goto done;
    g_slicks_diag_standings_phase=2; slicks_diag_standings_ready();
    if(result_wait(platform,1200,diagnostic) || result_fade(platform,palette,100,0,6,timer,&view)) goto done;
    g_slicks_diag_standings_phase=3; slicks_diag_standings_ready();
    result=0;
done:
    g_slicks_diag_standings_menu=0;
    if(!result && (slicks_amiga_platform_set_view(platform,0,black) ||
                   slicks_amiga_platform_set_view(platform,1,black))) result=-1;
    slicks_amiga_player_menu_destroy(m);
    slicks_resource_archive_close(&archive);
    if(bitmap) FreeMem(bitmap,64003);
    /* Both palettes remain black until the caller has drawn the title. */
    slicks_diag_standings_closed();
    return result;
}

static void update_race_engines(struct SlicksAmigaAudio *audio,const struct SlicksRaceRuntime *race)
{
    unsigned long speeds[4];
    for(unsigned i=0;i<4;++i) speeds[i]=(unsigned long)race->cars[i].measured_speed;
    slicks_amiga_audio_update_speeds(audio,speeds);
}
static void start_race_engines(struct SlicksAmigaAudio *audio,const struct SlicksRaceRuntime *race,
    struct SlicksAmigaPlatform *platform)
{
    unsigned short vehicles[4]; unsigned mask=0;
    for(unsigned i=0;i<4;++i) {
        vehicles[i]=race->cars[i].vehicle;
        if(!race->participation_ready || race->participation[i]) mask|=1U<<i;
    }
    slicks_amiga_platform_wait_display_blank(platform);
    slicks_amiga_audio_start_engines(audio,vehicles,mask);
    update_race_engines(audio,race);
}

static int run_race_pause(struct SlicksAmigaPlatform *platform,struct SlicksAmigaAudio *audio,
    struct SlicksRaceRuntime *race,unsigned char *chunky,const unsigned char *palette,
    struct SlicksConfiguration *configuration,unsigned char *setup_dirty,unsigned char row,unsigned char diagnostic)
{
    struct SlicksResourceArchive archive={0}; struct SlicksAmigaPlayerMenu *m=0;
    const struct SlicksControllersLabels labels={slicks_original_controller_key_names,
        slicks_original_controller_scans,slicks_original_controller_defaults,slicks_original_controller_exit};
    int result=-3; unsigned char engine=audio->engine_started; unsigned test_step=0;
    g_slicks_diag_pause_unavailable=0;
    g_slicks_diag_paused_race=race;
    slicks_diag_pause_live_enter();
    slicks_amiga_platform_wait_display_blank(platform);
    slicks_amiga_audio_stop(audio);
    if(slicks_resource_archive_cached(&archive,diagnostic==5?0:menu_cache)) {
        g_slicks_diag_pause_unavailable=1; goto unavailable;
    }
    m=diagnostic==6?0:slicks_amiga_race_surface_create(&archive,chunky,palette);
    if(diagnostic>=2 && diagnostic<=4) g_slicks_diag_pause_fault=(unsigned char)(diagnostic-1);
    if(!m || slicks_amiga_race_menu_open(m,&archive,menu_language_name,slicks_original_race_menu_keys,
        row,slicks_original_players_footer_percent)) {
        g_slicks_diag_pause_unavailable=m?3:2; goto unavailable;
    }
    g_slicks_diag_pause_menu=m; g_slicks_diag_paused_race=race;
    /* The original keyboard helper flushes entry events. Clear drive latches
     * so a release consumed by a modal cannot leave a car accelerating. */
    platform->key_tail=platform->key_head;
    race->controls=0;
    for(unsigned i=0;i<4;++i) race->driver_controls[i]=0;
    if(slicks_amiga_platform_set_view(platform,0,palette)) goto done;
    slicks_chunky_rows_to_amiga(chunky,platform->views[0].bitmap,0,200);
    slicks_amiga_player_menu_clear_dirty(m);
    if(show_menu(platform)) goto done;
    pause_live_checkpoint(platform,diagnostic,&test_step);
    while(!m->race_menu->state.result) {
        unsigned short raw;
        slicks_amiga_platform_wait_vblank(platform);
        if(g_slicks_diag_force_exit) goto done;
        while(slicks_amiga_platform_poll_key(platform,&raw)) {
            unsigned char character=slicks_amiga_menu_character(m,(unsigned char)raw);
            if(raw&128) continue;
            unsigned char scan=(unsigned char)amiga_raw_to_dos_scan(raw);
            if(!scan) continue;
            if(m->help_warning) {
                if(slicks_amiga_help_warning_close(m)) goto done;
            } else if(m->message) {
                if(slicks_amiga_message_close(m)) goto done;
            } else if(m->help) {
                struct SlicksAmigaHelpKey key=slicks_amiga_help_key((unsigned char)raw,character);
                if(slicks_help_viewer_key(m->help,key.ascii,key.scan)) {
                    if(slicks_amiga_help_close(m) || slicks_amiga_help_warning_open(m)) goto done;
                } else if(m->help->navigation.done) {
                    if(slicks_amiga_help_close(m)) goto done;
                }
            } else if(m->controllers_dialog) {
                struct SlicksControllersDialog *state=&m->controllers_dialog->state;
                if(state->capturing) {
                    if(slicks_controllers_capture(state,configuration,slicks_original_controller_scans,scan)) goto done;
                } else if(slicks_controllers_key(state,configuration,slicks_original_configuration.keys,scan)) goto done;
                if(state->done) {
                    if(slicks_amiga_controllers_close(m)) goto done;
                } else if(state->capturing) {
                    if(slicks_amiga_controllers_capture_prompt(m,slicks_original_controller_prompt)) goto done;
                } else if(slicks_amiga_controllers_draw(m,configuration,&labels)) goto done;
            } else if(m->race_menu->speed_active) {
                if(slicks_amiga_race_speed_key(m,configuration,
                    (unsigned char)amiga_raw_to_menu_scan(raw))) goto done;
                if(m->race_menu->speed.done) {
                    unsigned short argument;
                    if(slicks_amiga_race_speed_close(m,configuration,&argument)) goto done;
                    slicks_race_set_timer(race,argument);
                }
            } else {
                enum SlicksRaceMenuAction action=slicks_race_menu_key(&m->race_menu->state,scan);
                if(action==SLICKS_RACE_MENU_HELP) {
                    if(slicks_amiga_help_open(m,&archive,(const unsigned char *)"") &&
                       slicks_amiga_help_warning_open(m)) goto done;
                } else if(action==SLICKS_RACE_MENU_CONTROLLERS) {
                    *setup_dirty=1;
                    if(slicks_amiga_controllers_open_at(m,&archive,45,65)) {
                        if(slicks_amiga_warning_open(m,(const unsigned char *)"CONTROLLERS UNAVAILABLE - PRESS A KEY")) goto done;
                    } else if(slicks_amiga_controllers_draw(m,configuration,&labels)) goto done;
                } else if(action==SLICKS_RACE_MENU_SPEED) {
                    if(slicks_amiga_race_speed_open(m,configuration,setup_dirty)) goto done;
                } else if(!m->race_menu->state.result && slicks_amiga_race_menu_draw(m)) goto done;
            }
            if(m->race_menu->state.result) break;
            present_menu_surface(platform,m);
            if(!platform->active && slicks_amiga_platform_begin(platform,0)) goto done;
            pause_live_checkpoint(platform,diagnostic,&test_step);
        }
    }
    result=m->race_menu->state.result;
    if(slicks_amiga_race_menu_close(m)) { result=-3; goto done; }
    driver_device_configuration_storage=*configuration;
    for(unsigned i=0;i<4;++i) driver_device_state[i]=(struct SlicksDriverDeviceState){0,0,0,0};
    platform->key_tail=platform->key_head;
    slicks_diag_pause_live_closed();
    goto done;
unavailable:
    /* Opening is transactional: neither owner creation nor a failed menu
     * open leaves edits in the race pixels/configuration. View 1 was never
     * touched. Keep the race alive and allow the next menu key to retry. */
    if(pause_unavailable_notice(platform,race,chunky,palette,diagnostic)) goto done;
    result=1;
    platform->key_tail=platform->key_head;
done:
    /* Exec allocation release and closing a borrowed cache need no DOS work. */
    unsigned char restore=(unsigned char)(result!=-3);
    slicks_amiga_player_menu_destroy(m); slicks_resource_archive_close(&archive);
    g_slicks_diag_pause_menu=0; g_slicks_diag_paused_race=0;
    if(restore) {
        if(show_view(platform,1)) return -3;
        status_clock_vblank=platform->vblank_count;
        /* Resume the engines stopped on entry; one-shots are not replayed. */
        if(result==1 && engine) {
            slicks_amiga_platform_wait_display_blank(platform);
            start_race_engines(audio,race,platform);
        }
        if(g_slicks_diag_pause_unavailable) slicks_diag_pause_live_recovered();
    }
    return result;
}

/* Keep opt-in bookkeeping out of the rendering loop and its register set. */
static __attribute__((noinline)) void collect_presentation_snapshot(
    const struct SlicksRaceRuntime *race,const struct SlicksAmigaAudio *audio)
{
    g_slicks_diag_effect_sample_block=audio->last_effect_sample_block;
    g_slicks_diag_effect_priority=audio->last_effect_priority;
    g_slicks_diag_engine_started=audio->engine_started;
    g_slicks_diag_engine_frequency=audio->engine_frequency;
    g_slicks_diag_engine_period=audio->engine_period;
    g_slicks_diag_music_started=audio->music_started;
    g_slicks_diag_dirty_ranges=race->dirty_row_count;
    g_slicks_diag_dirty_pixels=race->dirty_pixel_count;
}

static __attribute__((noinline)) void collect_conversion_statistics(
    const struct SlicksRaceRuntime *race)
{
    unsigned long area=0,rows=0;
    for(unsigned i=0;i<race->dirty_row_count;++i) {
        const struct SlicksDirtyRows *r=&race->dirty_rows[i];
        unsigned long pixels=(unsigned long)(r->right-r->left)*(r->bottom-r->top);
        area+=pixels;
        rows+=pixels/320UL; /* Preserve per-rectangle rounding. */
    }
    g_slicks_diag_sparse_converted=race->dirty_pixel_count;
    g_slicks_diag_profile_rect_pixels=area;
    g_slicks_diag_dirty_rows=rows;
    g_slicks_diag_dirty_c2p_rows+=rows;
    g_slicks_diag_dirty_c2p_calls+=race->dirty_row_count;
}

static void restore_demo_configuration(struct SlicksConfiguration *configuration,
    struct SlicksRaceRuntime *race,unsigned char played)
{
    if(!title_demo.active) return;
    if(played) {
        g_slicks_setup_session.random_state=race->random_state;
        slicks_race_reset_return(race);
    }
    slicks_title_demo_restore(&title_demo,configuration,&g_slicks_track_playlist,
        &title_arcade_refresh);
    (void)slicks_race_set_demo(race,0,0);
    /* Original title re-entry calls 2bb70(0,0), after clearing DS:0459. */
    slicks_setup_select(&g_slicks_setup_session,configuration,&setup_resources,0);
}

int main(void)
{
    int argc = 0;
    const char *argv = 0;
    static unsigned char source_palette[768];
    static unsigned char race_palette[768];
    static unsigned short mode_state[11];
    static char track_names[SLICKS_TRACK_FILE_MAX][SLICKS_TRACK_NAME_SIZE];
    struct SlicksResourceArchive archive = {0};
    struct SlicksAmigaPlatform platform = {0};
    struct SlicksAmigaAudio audio = {0};
    unsigned char *logical = 0;
    unsigned char *chunky = 0;
    unsigned char *title_asset = 0;
    unsigned char *title_frame = 0;
    unsigned long title_font_sizes[3]={0,0,0};
    struct SlicksRaceRuntime *race = 0;
    unsigned char *sample_resource = 0;
    unsigned long title_checksum = 0;
    unsigned long title_display_checksum = 0;
    unsigned char auto_race;
    unsigned char original_setup = 0;
    unsigned char restore_test;
    unsigned char shadow_fixture = 0;
    unsigned char jump_track_test = 0;
    short previous_jump_state[SLICKS_RACE_CAR_COUNT] = {0, 0, 0, 0};
    unsigned char shadow_background = 0;
    unsigned long shadow_at = 0;
    unsigned short menu_selection = 0;
    unsigned short service_menu_open = 0, service_selection = 0;
    struct SlicksPlayerMenu player_menu_state={0,0,0,0};
    struct SlicksStatusClock picker_clock={0};
    unsigned long picker_clock_vblank=0;
    unsigned short selected_vehicle = 5;
    unsigned short playlist_position = 0;
    unsigned short selected_track = 0;
    unsigned short track_count;
    unsigned short selected_laps = 4;
    unsigned short demo_track=0,demo_vehicle=0,demo_laps=0;
    unsigned long title_idle_started=0;
    unsigned char title_idle_active=0,title_idle_reset=0;
    struct SlicksConfiguration configuration = slicks_original_configuration;
    unsigned char setup_dirty=0,exit_requested=0,save_prompt=0,right_was_down=0;
    unsigned char registration_presentation=0;
    unsigned char registration_test=0;
    unsigned short registration_today=0;
    unsigned char race_load_prompt=0,race_load_retry=0;
    const char *track_path;
    char selected_track_path[19];
    int result = 20;

    g_slicks_diag_profile_platform = &platform;
    setup_resources.override_count=slicks_original_override_count;
    /* Query only: preserve the OS/user cache configuration. */
    g_slicks_diag_initial_cache_control = CacheControl(0,0);

    DOSBase = (struct DosLibrary *)OpenLibrary(
        (CONST_STRPTR)"dos.library", 37);
    GfxBase = (struct GfxBase *)OpenLibrary(
        (CONST_STRPTR)"graphics.library", 39);
    if (!DOSBase || !GfxBase)
        goto cleanup;
    g_slicks_registration_status=(short)load_registration();
    slicks_diag_registration_loaded();
    if(g_slicks_registration_status<0) {
        PutStr((CONST_STRPTR)"Invalid or unreadable SLICKS.REK; no files were changed.\n");
        goto cleanup;
    }
    {
        struct UtilityBase *UtilityBase=(struct UtilityBase *)OpenLibrary(
            (CONST_STRPTR)"utility.library",37);
        if(!UtilityBase) goto cleanup;
        struct DateStamp now;
        struct ClockData date;
        DateStamp(&now);
        Amiga2Date((unsigned long)now.ds_Days*86400UL+
            (unsigned long)now.ds_Minute*60UL+(unsigned long)now.ds_Tick/TICKS_PER_SECOND,&date);
        registration_today=(unsigned short)((date.year<1997?0:date.mday)+31UL*date.month+372UL*date.year);
        CloseLibrary((struct Library *)UtilityBase);
        slicks_player_profile_defaults(&g_slicks_profiles,slicks_original_profile_colours);
        /* Original 2c03e stores calendar day, one-based month and full year;
         * its pre-1997 guard clears the day. No DOS BIOS identity on Amiga. */
        g_slicks_setup_load_report=slicks_amiga_load_setup(&configuration,&g_slicks_profiles,
            date.year<1997?0:date.mday,date.month,date.year);
        if(g_slicks_setup_load_report.result!=SLICKS_SETUP_LOADED) {
            PutStr((CONST_STRPTR)"Slicks: setup could not be loaded; no files were changed.\n");
            if(g_slicks_setup_load_report.path) {
                PutStr((CONST_STRPTR)g_slicks_setup_load_report.path);
                PutStr((CONST_STRPTR)"\n");
            }
            switch(g_slicks_setup_load_report.result) {
            case SLICKS_SETUP_LOAD_RECOVERY:
                PutStr((CONST_STRPTR)"Recovery required: preserve and inspect .new/.bak files before restarting.\n"); break;
            case SLICKS_SETUP_LOAD_FOREIGN:
                PutStr((CONST_STRPTR)"Foreign CFG signature: this configuration requires explicit import.\n"); break;
            case SLICKS_SETUP_LOAD_INVALID:
                PutStr((CONST_STRPTR)"Malformed or incomplete setup file. Restore a known-good copy.\n"); break;
            default:
                PrintFault(g_slicks_setup_load_report.io_error,(CONST_STRPTR)"Setup read"); break;
            }
            slicks_diag_setup_load_failed();
            goto cleanup;
        }
        const char *labels[3]={slicks_original_profile_labels[0],
            slicks_original_profile_labels[1],slicks_original_profile_labels[2]};
        if(slicks_set_builtin_profiles(&g_slicks_profiles,SLICKS_VEHICLE_COUNT,labels))
            goto cleanup;
    }
    /* The shared framework invokes main() with no C arguments. Read the
     * CLI string from DOS, not the caller's saved registers on the stack. */
    argv = (const char *)GetArgStr();
    if (!argv)
        argv = "";
    while (*argv == ' ' || *argv == '\t')
        ++argv;
    while (argv[argc])
        ++argc;
    while (argc && (unsigned char)argv[argc - 1] <= ' ')
        --argc;
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='F' && argv[5]=='1' && argv[6]=='2') {
        demo_lifecycle_test=1;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='I' && argv[5]=='D' && argv[6]=='L') {
        demo_lifecycle_test=2;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='M' && argv[5]=='N' && argv[6]=='U') {
        demo_lifecycle_test=3;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='I' && argv[5]=='O' && argv[6]=='S') {
        demo_lifecycle_test=4;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='E' && argv[5]=='N' && argv[6]=='D') {
        demo_lifecycle_test=5;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='E' && argv[5]=='R' && argv[6]=='R') {
        demo_lifecycle_test=6;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='H' && argv[5]=='U' && argv[6]=='D') {
        demo_lifecycle_test=7;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='S' && argv[5]=='A' && argv[6]=='V') {
        demo_lifecycle_test=8;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='M' && argv[5]=='E' && argv[6]=='M') {
        demo_lifecycle_test=9;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='R' && argv[5]=='E' && argv[6]=='T') {
        demo_lifecycle_test=10;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='E' && argv[5]=='X' && argv[6]=='T') {
        demo_lifecycle_test=11;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='E' && argv[2]=='M' && argv[3]=='O' &&
       argv[4]=='V' && argv[5]=='I' && argv[6]=='W') {
        demo_lifecycle_test=12;argc=0;argv="";
    }
    if(argc==7 && argv[0]=='D' && argv[1]=='I' && argv[2]=='S' && argv[3]=='P' &&
       argv[4]=='M' && argv[5]=='E' && argv[6]=='M') {
        display_allocation_test=1;demo_lifecycle_test=11;argc=0;argv="";
    }
    if((argc==8 || (argc==9 && (argv[8]=='Y' || argv[8]=='F' || argv[8]=='G' || argv[8]=='H' || argv[8]=='I' || argv[8]=='J' || argv[8]=='D' || argv[8]=='T' || argv[8]=='A' || argv[8]=='B' || argv[8]=='C'))) && argv[0]=='R' && argv[1]=='E' && argv[2]=='G' &&
       argv[3]=='C' && argv[4]=='H' && argv[5]=='E' && argv[6]=='C' && argv[7]=='K') {
        if(argc==9 && (argv[8]=='D' || argv[8]=='T' || argv[8]=='A' || argv[8]=='B' || argv[8]=='C')) title_dirty_test=argv[8]=='D'?1:argv[8]=='T'?2:argv[8]=='A'?3:argv[8]=='B'?4:5;
        else if(argc==9) registration_help_test=argv[8]=='J'?6:argv[8]=='H'?4:argv[8]=='I'?5:argv[8]=='G'?3:argv[8]=='Y'?1:2;
        registration_test=1;argc=0;argv="";
    }
    /* Explicit diagnostic state, never a normal-game override.
     * Set natively: debugger writes are not reliable on every FS-UAE stub. */
    if(argc==11 && argv[0]=='N' && argv[1]=='A' && argv[2]=='T' && argv[3]=='U' &&
       argv[4]=='R' && argv[5]=='A' && argv[6]=='L' && argv[7]=='O' &&
       argv[8]>='0' && argv[8]<='3' && argv[9]=='Q' && argv[10]=='D') {
        /* Overlay-only display audit, not the title demo lifecycle. */
        demo_render_only=1; argc=10;
    }
    if(argc==11 && argv[0]=='N' && argv[1]=='A' && argv[2]=='T' && argv[3]=='U' &&
       argv[4]=='R' && argv[5]=='A' && argv[6]=='L' && argv[7]=='B' && argv[8]>='0' && argv[8]<='3' &&
       argv[9]=='P' && argv[10]>='0' && argv[10]<='3') {
        status_clock.remainder=819200UL*(unsigned)(argv[10]-'0');
        argc=9;
    }
    /* NATIVE is the normal interactive route with explicit debug snapshots.
     * Plain launches should not pay for per-update inspection/checksums. */
    unsigned char native_debug=(unsigned char)(argc==6 && argv[0]=='N' &&
        argv[1]=='A' && argv[2]=='T' && argv[3]=='I' && argv[4]=='V' && argv[5]=='E');
    if(native_debug) { argc=0;argv=""; }
    unsigned char continuous_diagnostics=(unsigned char)(argc!=0 || native_debug);
    if(!continuous_diagnostics)g_slicks_diag_target_frame=0;
    unsigned char weapon_case_test=(unsigned char)(argc==9 && argv[7]=='W' && argv[8]>='1' && argv[8]<='9');
    unsigned char actor_case_test=(unsigned char)((argc==9 || (argc==10 && argv[9]=='Q')) && argv[7]=='O' && argv[8]>='0' && argv[8]<='3');
    unsigned char gameplay_benchmark=(unsigned char)(argc==9 && (argv[7]=='M' || argv[7]=='B' || argv[7]=='S' || (argv[7]>='1' && argv[7]<='6')) && argv[8]>='0' && argv[8]<='3');
    if(gameplay_benchmark && argv[7]!='M')continuous_diagnostics=0;
    unsigned char audio_pcm_test=(unsigned char)(argc==9 && argv[7]=='Q' && argv[8]=='B');
    unsigned char natural_results_test=(unsigned char)((argc==8 || weapon_case_test || actor_case_test || audio_pcm_test || gameplay_benchmark) && argv[0]=='N' && argv[1]=='A' &&
        argv[2]=='T' && argv[3]=='U' && argv[4]=='R' && argv[5]=='A' && argv[6]=='L' &&
        (argv[7]=='D' || argv[7]=='F' || argv[7]=='W' || argv[7]=='P' || argv[7]=='R' || argv[7]=='E' || argv[7]=='C' || argv[7]=='A' || argv[7]=='T' || argv[7]=='I' || argv[7]=='Q' || argv[7]=='O' || gameplay_benchmark));
    if(natural_results_test) shop_transition_test=argv[7]=='P'?1:argv[7]=='R'?2:argv[7]=='E'?3:argv[7]=='C'?4:argv[7]=='A'?5:0;
    shop_test=(unsigned char)(natural_results_test && (argv[7]=='W' || shop_transition_test));
    if(shop_test && weapon_case_test) g_slicks_diag_weapon_case=(unsigned short)(argv[8]-'0');
    if(shop_transition_test) g_slicks_diag_weapon_case=1;
    if(natural_results_test) {
        if(argv[7]=='O') g_slicks_diag_audit_bitmap=1;
        /* Isolated input configuration, before original selection/new-game
         * setup. Never inject moving cars, finish state, or result pixels. */
        setup_dirty=1;
        configuration=slicks_original_configuration;
        configuration.options[0]=4; configuration.options[3]=1;
        if(argv[7]=='T' || argv[7]=='I' || argv[7]=='Q' || gameplay_benchmark) configuration.options[3]=4;
        configuration.options[7]=0; configuration.options[9]=10;
        if(shop_test) { configuration.options[7]=1; configuration.options[4]=1000; }
        configuration.options[10]=(argv[7]=='D' || argv[7]=='T' || argv[7]=='I' || argv[7]=='Q' || gameplay_benchmark)?300:0;
        g_slicks_profiles.count=7;
        static const unsigned char fleet[4]={5,2,0,0};
        /* Observed PC audio-comparison fleet; input selection only. */
        static const unsigned char pc_audio_fleet[4]={0,0,1,6};
        for(unsigned i=0;i<4;++i) {
            unsigned p=i+3;
            g_slicks_profiles.setup[p]=g_slicks_profiles.setup[1];
            if(shop_test && (!i || (!weapon_case_test && !shop_transition_test && i==1)))
                g_slicks_profiles.setup[p].flags&=(unsigned char)~1U;
            g_slicks_profiles.setup[p].vehicle=argv[7]=='I'?0:
                argv[7]=='Q'?pc_audio_fleet[i]:fleet[i];
            g_slicks_profiles.setting[p]=100;
            for(unsigned j=0;j<9;++j) g_slicks_profiles.statistics[p][j]=0;
            for(unsigned j=0;j<21;++j) g_slicks_profiles.names[p][j]=0;
            g_slicks_profiles.names[p][0]='C'; g_slicks_profiles.names[p][1]='P';
            g_slicks_profiles.names[p][2]='U'; g_slicks_profiles.names[p][3]=' ';
            g_slicks_profiles.names[p][4]=(unsigned char)('1'+i);
            configuration.selected_profile[i]=(short)p;
        }
    }
    unsigned char profile_dialog_failure_test=(unsigned char)(argc==9 &&
        argv[0]=='P' && argv[1]=='L' && argv[2]=='A' && argv[3]=='Y' &&
        argv[4]=='E' && argv[5]=='R' && argv[6]=='S' &&
        (argv[7]=='N' || argv[7]=='C') && (argv[8]=='F' || argv[8]=='L'));
    unsigned char profile_dialog_failure_sent=0;
    unsigned char profile_dialog_fault=profile_dialog_failure_test && argv[8]=='L'?2:1;
    if(profile_dialog_failure_test) --argc;
    unsigned char setup_session_test=(unsigned char)((argc==5 || (argc==6 && (argv[5]=='I' || argv[5]=='R' || argv[5]=='A' || argv[5]=='F' || argv[5]=='G'))) && argv[0]=='S' &&
        argv[1]=='E' && argv[2]=='T' && argv[3]=='U' && argv[4]=='P');
    unsigned char setup_input_test=(unsigned char)(setup_session_test && argc==6 && argv[5]=='I');
    unsigned char setup_reload_test=(unsigned char)(setup_session_test && argc==6 && (argv[5]=='R' || argv[5]=='A' || argv[5]=='F' || argv[5]=='G'));
    unsigned char setup_failure_test=(unsigned char)(setup_session_test && argc==6 && (argv[5]=='F' || argv[5]=='G'));
    if(setup_failure_test) g_slicks_diag_race_load_fault=argv[5]=='F'?2:6;
    unsigned char setup_abort_test=(unsigned char)(setup_session_test && argc==6 && argv[5]=='A'),setup_abort_sent=0;
    unsigned char pause_live_test=(unsigned char)((argc==8 || (argc==9 && (argv[8]=='F' || argv[8]=='N'))) && argv[0]=='L' && argv[1]=='I' &&
        argv[2]=='V' && argv[3]=='E' && argv[4]=='M' && argv[5]=='E' && argv[6]=='N' && argv[7]=='U');
    unsigned char pause_failure_test=(unsigned char)(pause_live_test && argc==9 && argv[8]=='F');
    unsigned char pause_nested_test=(unsigned char)(pause_live_test && argc==9 && argv[8]=='N');
    unsigned char pause_live_sent=0;
    if(setup_input_test) {
        configuration.selected_profile[0]=2;
        configuration.selected_profile[1]=0;
        configuration.selected_profile[2]=2;
        configuration.selected_profile[3]=1;
    }
    unsigned char player_menu_test=(unsigned char)((argc==7 || (argc==8 && (argv[7]=='R' || argv[7]=='K' || argv[7]=='G' || argv[7]=='D' || argv[7]=='E' || argv[7]=='N' || argv[7]=='C' || argv[7]=='P' || argv[7]=='T' || argv[7]=='U' || argv[7]=='V' || argv[7]=='W' || argv[7]=='H' || argv[7]=='X' || argv[7]=='Y'))) && argv[0]=='P' && argv[1]=='L' &&
        argv[2]=='A' && argv[3]=='Y' && argv[4]=='E' && argv[5]=='R' && argv[6]=='S');
    unsigned char vehicle_save_test=(unsigned char)(argc==8 && argv[0]=='P' && argv[1]=='L' &&
        argv[2]=='A' && argv[3]=='Y' && argv[4]=='E' && argv[5]=='R' && argv[6]=='S' && (argv[7]=='Z' || argv[7]=='B'));
    player_menu_test|=vehicle_save_test || (argc==8 && argv[0]=='P' && argv[1]=='L' &&
        argv[2]=='A' && argv[3]=='Y' && argv[4]=='E' && argv[5]=='R' && argv[6]=='S' && argv[7]=='S');
    unsigned char name_test_stage=0;
    unsigned char shared_human_test=(unsigned char)(player_menu_test && argc==8 && argv[7]=='H');
    unsigned char persistence_test=(unsigned char)(player_menu_test && argc==8 && (argv[7]=='P' || argv[7]=='T' || argv[7]=='U' || argv[7]=='V' || argv[7]=='W' || argv[7]=='X' || argv[7]=='Y'));
    unsigned char unique_setup_test=(unsigned char)(persistence_test && argv[7]=='Y'),unique_stage=0;
    unsigned char mixed_setup_test=(unsigned char)(persistence_test && argv[7]=='W');
    unsigned char combined_test=(unsigned char)(persistence_test && argv[7]=='U'),combined_stage=0;
    unsigned char failure_injected=0;
    unsigned char record_recovery_test=(unsigned char)(argc==9 && argv[7]=='B' && (argv[8]=='R' || argv[8]=='S' || argv[8]=='L'));
    if(record_recovery_test) { g_slicks_diag_record_faults=3; g_slicks_diag_record_skip=argv[8]=='S'?1:argv[8]=='L'?2:0; }
    unsigned char intermission_live_test=(unsigned char)(argc==9 && argv[7]=='T' && (argv[8]=='I' || argv[8]=='J' || argv[8]=='K'));
    unsigned char intermission_retry_test=(unsigned char)(intermission_live_test && argv[8]!='I'?(argv[8]=='K'?2:1):0);
    mode_transition_test=(unsigned char)(argc==9 && argv[0]=='O' && argv[1]=='P' &&
        argv[2]=='T' && argv[3]=='I' && argv[4]=='O' && argv[5]=='N' && argv[6]=='S' &&
        argv[7]=='T' && argv[8]>='0' && argv[8]<='5');
    if(mode_transition_test) {
        g_slicks_diag_mode_case=(unsigned short)(argv[8]-'0');
        g_slicks_diag_audit_bitmap=1;
    }
    unsigned char options_test=(unsigned char)((record_recovery_test || intermission_live_test || argc==7 || (argc==8 && (argv[7]=='A' || argv[7]=='B' || argv[7]=='C' || argv[7]=='D' || argv[7]=='E' || argv[7]=='F' || argv[7]=='H' || argv[7]=='J' || argv[7]=='K' || argv[7]=='L' || argv[7]=='M' || argv[7]=='N' || argv[7]=='P' || argv[7]=='Q' || argv[7]=='R' || argv[7]=='S' || argv[7]=='T' || argv[7]=='U' || argv[7]=='V' || argv[7]=='W' || argv[7]=='Z'))) && argv[0]=='O' && argv[1]=='P' &&
        argv[2]=='T' && argv[3]=='I' && argv[4]=='O' && argv[5]=='N' && argv[6]=='S');
    options_test|=mode_transition_test;
    unsigned char controllers_test=(unsigned char)(options_test && argc==8 && argv[7]=='C');
    unsigned char collisions_test=(unsigned char)(options_test && argc==8 && argv[7]=='D');
    unsigned char weapons_test=(unsigned char)(options_test && argc==8 && (argv[7]=='E' || argv[7]=='F'));
    unsigned char joystick_test=(unsigned char)(options_test && argc==8 && argv[7]=='Q');
    unsigned char arcade_test=(unsigned char)(options_test && (argc==8 || intermission_live_test || record_recovery_test) && (argv[7]=='A' || argv[7]=='B' || argv[7]=='P' || argv[7]=='T' || argv[7]=='U' || argv[7]=='Z'));
    arcade_test|=mode_transition_test;
    unsigned char arcade_save_test=(unsigned char)(arcade_test && argv[7]=='Z');
    unsigned char sequence_test=(unsigned char)(arcade_test && (argv[7]=='B' || argv[7]=='P' || argv[7]=='T' || argv[7]=='U'));
    unsigned char pause_save_test=(unsigned char)(sequence_test && argv[7]=='P');
    unsigned char pause_transition_test=(unsigned char)(sequence_test && (argv[7]=='T' || pause_save_test));
    unsigned char sequence_failure_test=(unsigned char)(sequence_test && argv[7]=='U');
    unsigned char sequence_returns=0;
    unsigned char completion_return_test=(unsigned char)(natural_results_test ||
        (argc==8 && argv[0]=='C' && argv[1]=='O' && argv[2]=='N' &&
         argv[3]=='F' && argv[4]=='I' && argv[5]=='G' && argv[6]=='D' && argv[7]=='R') ||
        (argc==5 && argv[0]=='F' && argv[1]=='U' && argv[2]=='E' && argv[3]=='L' && argv[4]=='R'));
    completion_watch=(unsigned char)(completion_return_test || sequence_test);
    unsigned char volume_test=(unsigned char)(options_test && argc==8 && (argv[7]=='V' || argv[7]=='W'));
    unsigned char volume_save_test=(unsigned char)(volume_test && argv[7]=='W');
    unsigned char clear_test=(unsigned char)(options_test && argc==8 && (argv[7]=='R' || argv[7]=='S')),clear_test_stage=0;
    unsigned char help_test=(unsigned char)(options_test && argc==8 && (argv[7]=='H' || argv[7]=='J' || argv[7]=='K' || argv[7]=='L' || argv[7]=='M' || argv[7]=='N'));
    unsigned char help_failure_test=(unsigned char)(help_test && (argv[7]=='L' || argv[7]=='M' || argv[7]=='N')),help_failure_stage=0;
    unsigned char options_test_stage=0;
    unsigned char tracks_test=(unsigned char)((argc==6 || (argc==7 && (argv[6]=='L' || argv[6]=='R' || argv[6]=='D' || argv[6]=='F' || argv[6]=='I' || argv[6]=='J' || argv[6]=='K' || argv[6]=='M' || argv[6]=='N'))) && argv[0]=='T' && argv[1]=='R' &&
        argv[2]=='A' && argv[3]=='C' && argv[4]=='K' && argv[5]=='S');
    unsigned char track_scroll_test=(unsigned char)(argc==7 && argv[0]=='T' && argv[1]=='R' &&
        argv[2]=='A' && argv[3]=='C' && argv[4]=='K' && argv[5]=='S' && argv[6]=='P');
    tracks_test|=track_scroll_test;
    unsigned char tracks_test_stage=0;
    unsigned char track_info_test=(unsigned char)(tracks_test && argc==7 && (argv[6]=='I' || argv[6]=='J' || argv[6]=='K'));
    unsigned char track_info_fault_test=(unsigned char)(track_info_test && argv[6]=='K'),track_info_fault_stage=1;
    unsigned char track_info_failure_test=(unsigned char)(track_info_test && argv[6]=='J'),track_info_failure_stage=0;
    unsigned char track_lists_test=(unsigned char)(tracks_test && argc==7 && !track_info_test && !track_scroll_test);
    unsigned char title_help_test=(unsigned char)((argc==4 || (argc==5 && argv[4]=='F')) && argv[0]=='H' && argv[1]=='E' && argv[2]=='L' && argv[3]=='P');
    unsigned char title_help_failure_test=(unsigned char)(title_help_test && argc==5),title_help_failure_stage=0;
    if(argc==6 && argv[0]=='H' && argv[1]=='E' && argv[2]=='L' && argv[3]=='P' &&
       argv[4]=='L' && argv[5]>='0' && argv[5]<='9') {
        title_help_test=1; configuration.field_05e1=(unsigned char)(argv[5]=='9'?0:argv[5]-'0');
        language_choice_test=(unsigned char)(argv[5]=='9'?2:argv[5]=='0');
    }
    unsigned char title_start_test=(unsigned char)(argc==7 && argv[0]=='S' && argv[1]=='T' &&
        argv[2]=='A' && argv[3]=='R' && argv[4]=='T' ?
        (argv[5]=='G' && argv[6]=='O'?1:argv[5]=='F' && argv[6]=='9'?2:0):0);
    shop_resume_test=(unsigned char)(argc==10 && argv[9]=='W');
    if((argc==9 || shop_resume_test) && argv[0]=='C' && argv[1]=='H' && argv[2]=='A' && argv[3]=='M' && argv[4]=='P')
        championship_test=(unsigned char)(argv[5]=='S'?(argv[8]=='F'?6:1):argv[5]=='L'?2:argv[5]=='E'?3:argv[5]=='F'?4:0);
    championship_delete_test=(unsigned char)(championship_test==1 && argc==9 && argv[8]=='D');
    if(championship_delete_test) championship_test=6;
    championship_scan_test=(unsigned char)(championship_test==1 && argc==9 ?
        (argv[8]=='X'?1:argv[8]=='Y'?2:0):0);
    if(championship_scan_test) championship_test=6;
    championship_cleanup_test=(unsigned char)(championship_test==1 && argc==9 && argv[8]=='B');
    if(championship_cleanup_test) { championship_test=6; g_slicks_diag_backup_protect=1; }
    if(shop_resume_test && championship_test==2) {shop_test=1;g_slicks_diag_weapon_case=1;}
    original_setup=(unsigned char)(!argc || title_start_test || natural_results_test || championship_test || setup_session_test || player_menu_test || options_test || title_help_test || tracks_test);
    if(original_setup) {
        struct DateStamp now;
        DateStamp(&now);
        /* DOS time() uses a 1970 epoch; Amiga DateStamp days start in 1978.
         * Use the local machine's clock as the platform seed source. This
         * does not promise identical clocks/time-zone settings on two hosts. */
        unsigned long seconds=252460800UL+(unsigned long)now.ds_Days*86400UL+
            (unsigned long)now.ds_Minute*60UL+(unsigned long)now.ds_Tick/TICKS_PER_SECOND;
        setup_resources=(struct SlicksSetupResources){g_slicks_profiles.setup,
            g_slicks_profiles.count,SLICKS_VEHICLE_COUNT,slicks_original_override_count,
            slicks_original_fallback_colours,slicks_original_vehicle_weights,
            slicks_original_item_flags,request_setup_vehicle,setup_vehicle_requests};
        slicks_setup_session_start(&g_slicks_setup_session,&configuration,&setup_resources,
            (setup_session_test || natural_results_test)?0x1234:(unsigned short)seconds);
        selected_vehicle=(unsigned char)g_slicks_setup_session.players.vehicle[0];
        if(configuration.options[3]>=1 && configuration.options[3]<=100)
            selected_laps=(unsigned short)configuration.options[3];
    }
    /* Preserve the existing default pending the original startup chooser;
     * positive saved selections must not silently become English. */
    (void)slicks_language_resource(menu_language_name,configuration.field_05e1);
    if(display_allocation_test) {
        g_slicks_display_allocation_checks=slicks_amiga_platform_check_create_failures(GfxBase);
        if(g_slicks_display_allocation_checks!=10) goto cleanup;
    }
    if (slicks_amiga_platform_create(&platform, GfxBase) != 0)
        goto cleanup;

    if (slicks_resource_archive_open(&archive, "SLICKS.000") != 0)
        goto cleanup;
    if(!configuration.field_05e1) {
        int language=choose_startup_language(&archive);
        if(language<1) {
            PutStr((CONST_STRPTR)"Slicks: language selection needs an interactive console.\n");
            goto cleanup;
        }
        configuration.field_05e1=(unsigned char)language; setup_dirty=1;
        if(slicks_language_resource(menu_language_name,(unsigned)language)) goto cleanup;
    }
    title_asset = (unsigned char *)AllocMem(64003UL, MEMF_ANY);
    title_frame = (unsigned char *)AllocMem(TITLE_FRAME_ALLOCATION_BYTES,
                                          MEMF_ANY | MEMF_CLEAR);
    if (!title_asset || !title_frame)
        goto cleanup;
    if (slicks_resource_archive_load(&archive, "mainmenu.@I", title_asset,
                                     64003UL) != 64003L ||
        slicks_resource_archive_load(&archive, "partII", source_palette,
                                     sizeof(source_palette)) != 768L)
        goto cleanup;
    slicks_title_prepare_background(title_asset+3,source_palette);
    if(slicks_prepare_title_frame(title_asset,title_frame)!=0)
        goto cleanup;
    slicks_title_background=title_frame;
    {
        static const char *const names[]={"val1.@I","val2.@I","pel_on.@I","pel_ei.@I","pel_t.@I"};
        for(unsigned i=0;i<5;++i) {
            long bytes=slicks_resource_archive_load(&archive,names[i],title_asset,64003UL);
            if(bytes<0 || slicks_decode_indexed_menu_icon(title_asset,(unsigned long)bytes,
                title_icons[i],sizeof title_icons[i],&title_icon_width[i],&title_icon_height[i])) goto cleanup;
        }
        title_configuration=&configuration;
    }

    {
        static const char *const font_names[]={"iso.@f","kirj.@f","pieni.@f"};
        unsigned char **fonts[]={&slicks_title_font,&slicks_title_small_font,&title_arcade_font};
        for(unsigned i=0;i<3;++i) {
            long bytes=slicks_resource_archive_load(&archive,font_names[i],title_asset,64003UL);
            long required=bytes>0?slicks_font_resource_size(title_asset,(unsigned long)bytes):-1;
            if(required<=0 || required>(long)TITLE_FONT_CAPACITY) goto cleanup;
            title_font_sizes[i]=(unsigned long)required;
            *fonts[i]=AllocMem(title_font_sizes[i],MEMF_ANY);
            if(!*fonts[i] || slicks_decode_font_resource(title_asset,(unsigned long)bytes,
                *fonts[i],title_font_sizes[i])!=required) goto cleanup;
        }
        long bytes=slicks_resource_archive_load(&archive,menu_language_name,title_asset,64003UL);
        if(bytes<0 || slicks_language_table_load(title_asset,(unsigned)bytes,title_language,
            sizeof title_language,&title_language_used)) goto cleanup;
        for(unsigned i=0;i<7;++i)
            slicks_title_labels[i]=slicks_language_lookup(title_language,title_language_used,
                slicks_title_labels[i],slicks_title_labels[i]);
    }

    logical = (unsigned char *)AllocMem(0x40000UL, MEMF_ANY);
    if (!logical)
        goto cleanup;
    g_slicks_diag_logical = logical;
    chunky = (unsigned char *)AllocMem(CHUNKY_ALLOCATION_BYTES,
                                      MEMF_ANY | MEMF_CLEAR);
    if (!chunky)
        goto cleanup;
    race = (struct SlicksRaceRuntime *)AllocMem(sizeof(*race), MEMF_ANY);
    if (!race)
        goto cleanup;
    if (slicks_setup_basic_mode(logical, mode_state) != 0)
        goto cleanup;
    track_count = discover_tracks(track_names,(unsigned char)!original_setup);
    g_slicks_track_state.random_order=slicks_original_track_random_order;
    if(original_setup) {
        /* Original startup 26164/26169: select every discovered track,
         * then shuffle the playlist (without reordering the name catalogue). */
        if(slicks_track_playlist_all(&g_slicks_track_playlist,track_count) ||
           slicks_track_playlist_shuffle(&g_slicks_track_playlist,&g_slicks_setup_session.random_state)) goto cleanup;
        selected_track=(unsigned short)track_selection[0];
        if(natural_results_test) {
            unsigned i;
            for(i=0;i<track_count;++i) {
                static const char *const actor_tracks[]={"BASIC.SS","F1.SS","CITY.SS","WHACKO.SS"};
                const char *name=actor_tracks[(actor_case_test || gameplay_benchmark)?argv[8]-'0':0]; unsigned j=0;
                while(name[j] && track_names[i][j]==name[j]) ++j;
                if(!name[j] && !track_names[i][j]) break;
            }
            if(i==track_count) goto cleanup;
            selected_track=(unsigned short)i; track_selection[0]=(short)i;
            g_slicks_track_playlist.count=1;
            if(shop_transition_test) {
                /* Repeated BASIC races isolate inventory from track changes;
                 * the AI case needs more shop visits. No race state injection. */
                unsigned total=shop_transition_test==5?16:2;
                for(unsigned t=1;t<total;++t) track_selection[t]=(short)i;
                g_slicks_track_playlist.count=(short)total;
            }
        }
    }
    g_slicks_diag_track_files = track_count;
    {
        unsigned short at;
        for (at = 0; at < sizeof(g_slicks_diag_first_track); ++at)
            g_slicks_diag_first_track[at] = track_names[0][at];
        if (track_count > 1)
            for (at = 0; at < sizeof(g_slicks_diag_second_track); ++at)
                g_slicks_diag_second_track[at] = track_names[1][at];
    }
    /* Draw only after native startup has selected the playlist's first
     * track. The former BASIC placeholder disagreed with immediate GO. */
    make_title_surface(logical, title_frame, source_palette);
    redraw_title_configuration(&platform, logical, chunky, source_palette,
                               menu_selection, selected_vehicle,
                               track_names[selected_track], selected_laps);
    title_checksum = checksum_planes(logical);
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
    slicks_amiga_audio_set_volume(&audio,configuration.options[1],configuration.options[2]);
    FreeMem(sample_resource, 131691UL);
    sample_resource = 0;
    slicks_resource_archive_close(&archive);

    /* Opt-in listening probe: the production sample loader, engine adapter
     * and VBI drive one Paula channel at labelled, fixed measured speeds.
     * No mixer, racing input or driver state is replaced in normal runs. */
    unsigned char extreme_pitch_probe=(unsigned char)(argc==9 && argv[0]=='A' &&
        argv[1]=='U' && argv[2]=='D' && argv[3]=='I' && argv[4]=='O' &&
        argv[5]=='H' && argv[6]=='I' && argv[7]=='G' && argv[8]=='H');
    unsigned char falling_pitch_probe=(unsigned char)(argc==9 && argv[0]=='A' &&
        argv[1]=='U' && argv[2]=='D' && argv[3]=='I' && argv[4]=='O' &&
        argv[5]=='F' && argv[6]=='A' && argv[7]=='L' && argv[8]=='L');
    if(extreme_pitch_probe || falling_pitch_probe || (argc==10 && argv[0]=='A' && argv[1]=='U' && argv[2]=='D' &&
       argv[3]=='I' && argv[4]=='O' && argv[5]=='P' && argv[6]=='I' &&
       argv[7]=='T' && argv[8]=='C' && argv[9]=='H')) {
        static const unsigned long probe_speeds[3]={0,500,1000};
        static const unsigned long extreme_speeds[5]={0,4000,8500,4000,0};
        static const unsigned long falling_speeds[4]={0,1000,500,0};
        if(slicks_amiga_platform_begin(&platform,0)) goto cleanup;
        slicks_amiga_platform_wait_display_blank(&platform);
        slicks_amiga_audio_start_engine(&audio,0,100);
        for(unsigned stage=0;stage<(extreme_pitch_probe?5U:falling_pitch_probe?4U:3U);++stage) {
            unsigned long speeds[4]={extreme_pitch_probe?extreme_speeds[stage]:
                falling_pitch_probe?falling_speeds[stage]:probe_speeds[stage],0,0,0};
            slicks_amiga_platform_wait_display_blank(&platform);
            slicks_amiga_audio_update_speeds(&audio,speeds);
            g_slicks_diag_engine_frequency=audio.engine_frequency;
            g_slicks_diag_engine_period=audio.engine_period;
            g_slicks_diag_pitch_bank_mask|=(unsigned short)(1U<<audio.engine_levels[0]);
            unsigned long start=platform.vblank_count;
            while(platform.vblank_count-start<(extreme_pitch_probe?125U:250U))
                slicks_amiga_platform_wait_vblank(&platform);
        }
        slicks_amiga_platform_wait_display_blank(&platform);
        result=0; goto cleanup;
    }

    /* Diagnostic modes consume the trimmed raw CLI string, not Unix argv. */
    restore_test = (unsigned char)(
        argc > 0 && ((const char *)argv)[0] == 'E');
    service_menu_test = (unsigned char)(argc > 0 && argv[0] == 'C');
    if (service_menu_test) {
        /* CONFIG/CONFIGD exercise edits from a known zero baseline, not the
         * original saved custom defaults used by normal interactive play. */
        configuration.options[9]=configuration.options[10]=0;
        g_slicks_diag_target_frame = 3600;
    }
    auto_race = (unsigned char)(argc > 0 && !title_start_test && !natural_results_test && !championship_test && !restore_test && !service_menu_test && !player_menu_test && !options_test && !setup_reload_test && !title_help_test && !tracks_test);
    if(argc>0 && argv[0]=='W') {
        weapon_hud_fixture=1;
        g_slicks_diag_audit_bitmap=1;
        g_slicks_diag_target_frame=700;
    }
    if (natural_results_test || (argc > 0 && ((const char *)argv)[0] == 'F')) {
        fuel_race_test = 1;
        g_slicks_diag_target_frame = 3600;
    }
    if (argc > 0 && ((const char *)argv)[0] == 'U') {
        jump_track_test = 1;
        g_slicks_diag_target_frame = 700;
    }
    if (argc > 0 && ((const char *)argv)[0] == 'J') {
        shadow_fixture = 1;
        g_slicks_diag_audit_bitmap = 1;
        g_slicks_diag_target_frame = 5;
    }
    if (gameplay_benchmark || (argc > 0 && ((const char *)argv)[0] == 'M')) {
        /* Mode 2 retains outer work/cadence timing but does not invoke
         * intra-update profiling callbacks. Keep mode 1 for comparisons
         * against the historical detailed benchmark. */
        g_slicks_diag_profile_all = gameplay_benchmark && (argv[7]=='B' || argv[7]=='S')?2:1;
        g_slicks_pc_sampling = (unsigned char)(gameplay_benchmark && argv[7]=='S');
        if(gameplay_benchmark && argv[7]>='1' && argv[7]<='6')
            g_slicks_diag_profile_all=(unsigned char)(argv[7]-'0'+2);
        g_slicks_diag_target_frame = 700;
    }
    if (argc > 0 && ((const char *)argv)[0] == 'V') {
        g_slicks_diag_audio_in_blank = 1;
        g_slicks_diag_target_frame = 700;
    }
    if (argc > 0 && ((const char *)argv)[0] == 'D') {
        extern unsigned char slicks_amiga_audio_disable_dma;
        slicks_amiga_audio_disable_dma = 1;
        g_slicks_diag_target_frame = 700;
    }
    if (argc > 0 && argv[0] == 'N' && !natural_results_test) {
        /* NOAUDIO is a diagnostic, distinct from muted host output. All
         * playback/update entry points return before hardware access when
         * ready is clear. Keep sample allocations unchanged for this A/B. */
        audio.ready = 0;
        g_slicks_diag_audio_ready = 0;
        g_slicks_diag_target_frame = 700;
    }
    /* PLAYERS/PLAYERSR also start with P; they must retain normal particles
     * and the normal gameplay checkpoint when handing off from the menu. */
    if (argc > 0 && argv[0] == 'P' && !player_menu_test) {
        extern unsigned char slicks_race_disable_particles;
        slicks_race_disable_particles = 1;
        g_slicks_diag_target_frame = 700;
    }
    /* SETUP also starts with S, but must advance the real race. Only the
     * SCANOUT family selects the frozen-scene display diagnostic. */
    if (argc >= 7 && argv[0]=='S' && argv[1]=='C' && argv[2]=='A' &&
        argv[3]=='N' && argv[4]=='O' && argv[5]=='U' && argv[6]=='T')
        g_slicks_diag_scanout_only =
            (argc > 7 && ((const char *)argv)[7] == 'F') ? 3 :
            (argc > 7 && ((const char *)argv)[7] == 'C') ? 2 : 1;
    if (argc > 0 && ((const char *)argv)[0] == 'B') {
        g_slicks_diag_audit_bitmap =
            (argc > 6 && ((const char *)argv)[6] == 'F') ? 2 :
            (argc > 6 && ((const char *)argv)[6] == 'M') ? 3 : 1;
        g_slicks_diag_target_frame = 700;
    }
    if (argc > 0 && ((const char *)argv)[0] == 'L')
        g_slicks_diag_target_frame = 700;
    if (argc > 0 && ((const char *)argv)[0] == 'R')
        g_slicks_diag_target_frame = 1800;
    if (jump_track_test)
        track_path = "TRACKS/BUMPS.SS";
    else if (argc > 0 && ((const char *)argv)[0] == 'T')
        track_path = "TRACKS/BASICTRK.SS";
    else if (argc > 0 && ((const char *)argv)[0] == 'I')
        track_path = "TRACKS/ICE.SS";
    else if (argc > 0 && ((const char *)argv)[0] == 'H')
        track_path = "TRACKS/HEIKKI30.SS";
    else
        track_path = "TRACKS/BASIC.SS";

    /* Automated gates prepare before takeover. Interactive play deliberately
     * waits until GO so the native menu can choose the track, car and laps;
     * its loader temporarily runs with AmigaOS restored. */
    if(!g_slicks_diag_audit_bitmap && !(argc==7 && argv[0]=='U' && argv[1]=='I' &&
       argv[2]=='M' && argv[3]=='E' && argv[4]=='N' && argv[5]=='U' && argv[6]=='2')) {
        FreeMem(title_asset,64003UL); title_asset=0;
    }
    if(slicks_amiga_menu_keymap_init() ||
       slicks_resource_archive_open(&archive,"SLICKS.000")) goto cleanup;
    menu_cache=slicks_resource_cache_create(&archive,slicks_menu_resources,
        SLICKS_MENU_RESOURCE_COUNT);
    slicks_resource_archive_close(&archive);
    if(!menu_cache) goto cleanup;
    g_slicks_menu_cache_bytes=slicks_resource_cache_bytes(menu_cache);
    slicks_amiga_track_list_cache_refresh(&track_list_cache);
    if(championship_scan_test==1) g_slicks_diag_saved_lock_failure=1;
    if(championship_scan_test==2) g_slicks_diag_saved_next_failure=1;
    slicks_amiga_saved_files_refresh(&saved_files_cache);
    if (auto_race) {
        struct SlicksConfiguration diagnostic_configuration=configuration;
        if(fuel_race_test && !natural_results_test) {
            diagnostic_configuration.options[0]=4;
            diagnostic_configuration.options[9]=10;
            diagnostic_configuration.options[10]=0;
        }
        if (prepare_race(&platform, logical, chunky, mode_state, race,
                         track_path, race_palette, selected_vehicle,
                         &diagnostic_configuration,original_setup?&g_slicks_setup_session:0,1) != 0)
            goto cleanup;
        if (argc > 0 && ((const char *)argv)[0] == 'R')
            slicks_race_set_laps(race, 1);
    }

    if(argc==7 && argv[0]=='U' && argv[1]=='I' && argv[2]=='M' &&
       argv[3]=='E' && argv[4]=='N' && argv[5]=='U' && argv[6]=='2') {
        /* Reuse the startup staging buffer for the audit's snapshot. The
         * fixture exits afterward; no extra 64 KB modal allocation needed. */
        if(test_intermission_surface(&platform,chunky,race_palette,title_asset)) goto cleanup;
        result=0; goto cleanup;
    }
    if(argc==6 && argv[0]=='U' && argv[1]=='I' && argv[2]=='M' &&
       argv[3]=='E' && argv[4]=='N' && argv[5]=='U') {
        if(test_pause_surface(&platform,chunky,race_palette)) goto cleanup;
        result=0; goto cleanup;
    }
    g_slicks_diag_checksum = title_checksum;
    g_slicks_diag_display_checksum = title_display_checksum;
    registration_presentation=(unsigned char)(argc==0);
    if(registration_presentation && slicks_registration_trial_expired(registration_today,
        configuration.date_code,registration.name[0])) {
        if(registration_screen(&platform,chunky,0,slicks_speed_timer_argument(configuration.field_05de))) goto cleanup;
        make_title_surface(logical,title_frame,source_palette);
        redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
            selected_vehicle,track_names[selected_track],selected_laps);
        if(slicks_amiga_platform_set_view(&platform,0,source_palette)) goto cleanup;
    }
    if (slicks_amiga_platform_begin(&platform, 0) != 0)
        goto cleanup;
    g_slicks_diag_ready = 1;
    if (service_menu_test) {
        /* CONFIG gate: only input events, never setup/race-state injection.
         * Open Options, fuel +5 twice, damage +20, Escape, then GO. */
        static const unsigned char keys[] = {
            0x4d, 0x4d, 0x4d, 0x44, 0x4e, 0x4e, 0x4d,
            0x4e, 0x45, 0x4c, 0x4c, 0x4c, 0x44
        };
        unsigned short key;
        platform.key_tail = 0;
        for (key = 0; key < sizeof(keys); ++key)
            platform.keys[key] = keys[key];
        /* CONFIGD uses the same menu's Return-to-maximum action for damage. */
        if (argc > 6 && argv[6] == 'D')
            platform.keys[7] = 0x44;
        platform.key_head = sizeof(keys);
    }
    slicks_diag_frame_ready();
    if(championship_test) {
        static const unsigned char save[]={0x4d,0x4d,0x44},load[]={0x4d,0x4d,0x4d,0x4d,0x44};
        unsigned char start_new=(unsigned char)(championship_test==1 || championship_test==3 || championship_test==6);
        if(championship_test==3) championship_dialog_step=1; /* Skip obsolete Load dialog phase. */
        championship_test_keys(&platform,start_new?save:load,start_new?3:5);
    }
    if(setup_reload_test || natural_results_test) {
        platform.key_tail=0; platform.keys[0]=0x44; platform.key_head=1;
    }
    if(tracks_test) {
        /* Ordinary input: open Tracks, None, select first, close/reopen.
         * A second batch below starts the race, respecting the 16-key queue. */
        static const unsigned char keys[]={0x4d,0x4d,0x44,0x4e,0x4d,0x4d,0x4d,
            0x44,0x4f,0x44,0x45,0x44,0x45};
        platform.key_tail=0;
        for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
        platform.key_head=track_lists_test?10:sizeof keys;
        if(track_info_test) { platform.keys[3]=0x51; platform.key_head=4; }
        if(track_info_failure_test) platform.key_head=3;
        if(track_info_fault_test) g_slicks_diag_track_info_fault=1;
        if(track_scroll_test) {
            static const unsigned char scroll_keys[]={0x4d,0x4d,0x44,
                0x1b,0x1f,0x1a,0x3f,0x1d,0x4d,0x3d,0x4c,0x1d,0x45,0x44};
            for(unsigned i=0;i<sizeof scroll_keys;++i) platform.keys[i]=scroll_keys[i];
            platform.key_head=sizeof scroll_keys;
        }
    }
    if(title_start_test) {
        platform.key_tail=0;
        if(title_start_test==1) { platform.keys[0]=0x44; platform.key_head=1; }
        else {
            platform.keys[0]=0x4d; platform.keys[1]=0x4d;
            platform.keys[2]=0x58; platform.key_head=3;
        }
    }
    if(title_help_test) {
        /* F2 is ignored by the original title at both GO and Read This.
         * Keep its Change Cars meaning confined to the intermission owner. */
        static const unsigned char keys[]={0x51,0x4d,0x4d,0x4d,0x4d,0x51,0x44,0x45,0x50,0x45,0x4d,0x44};
        platform.key_tail=0;
        for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
        platform.key_head=sizeof keys;
        if(title_help_failure_test) platform.key_head=0;
    }
    if(help_test) {
        static const unsigned char keys[]={0x4d,0x4d,0x4d,0x44,0x50,0x4d,0x44,0x41,0x45,0x50,0x45,0x45,0x45};
        platform.key_tail=0;
        for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
        platform.key_head=sizeof keys;
        if(help_failure_test) platform.key_head=4; /* Enter Options; faults below. */
        if(help_failure_test && argv[7]=='M') platform.key_tail=2; /* Players */
        if(help_failure_test && argv[7]=='N') platform.key_tail=1; /* Tracks */
        if(argv[7]=='J') {
            /* Same navigation/reopen gate entered through Players. */
            platform.key_tail=2;
        }
        if(argv[7]=='K') {
            /* Players Help: previous page, next page, Contents, return. */
            static const unsigned char page_keys[]={0x4d,0x44,0x50,0x3f,0x1f,0x50,0x45,0x45,0x45};
            for(unsigned i=0;i<sizeof page_keys;++i) platform.keys[i]=page_keys[i];
            platform.key_head=sizeof page_keys;
        }
    } else if(options_test) {
        static const unsigned char keys[]={0x4d,0x4d,0x4d,0x44,0x4e,0x4e,0x4e,0x4e,
            0x4d,0x4d,0x4d,0x4e,0x45,0x44};
        platform.key_tail=0;
        unsigned count=(controllers_test || clear_test)?8:sizeof keys;
        for(unsigned i=0;i<count;++i) platform.keys[i]=keys[i];
        platform.key_head=(unsigned char)count;
        if(collisions_test || weapons_test) {
            static const unsigned char collision_keys[]={0x4d,0x4d,0x4d,0x44,0x3d,0x4e,0x4e,0x4e,0x4e};
            for(unsigned i=0;i<sizeof collision_keys;++i) platform.keys[i]=collision_keys[i];
            platform.key_head=sizeof collision_keys; options_test_stage=1;
        }
        if(joystick_test) {
            /* Reload the unique-profile fixture; choose Custom, Controllers,
             * driver 3 / joystick 1, then return to GO using ordinary keys. */
            static const unsigned char joystick_keys[]={
                0x4d,0x4d,0x4d,0x44,0x3d,0x4e,0x4e,0x4e,0x4e};
            for(unsigned i=0;i<sizeof joystick_keys;++i) platform.keys[i]=joystick_keys[i];
            platform.key_head=sizeof joystick_keys;
            g_slicks_diag_target_frame=3600;
        }
        if(clear_test) {
            /* Keypad 7 selects the original minimum before choosing Custom;
             * a prior diagnostic may already have saved another mode. */
            static const unsigned char clear_keys[]={0x4d,0x4d,0x4d,0x44,0x3d,0x4e,0x4e,0x4e,0x4e};
            for(unsigned i=0;i<sizeof clear_keys;++i) platform.keys[i]=clear_keys[i];
            platform.key_head=sizeof clear_keys;
        }
        if(volume_test) {
            static const unsigned char volume_keys[]={0x4d,0x4d,0x4d,0x44,0x4d,0x3d,0x4e,0x4e,0x4e,0x4e,0x4e};
            for(unsigned i=0;i<sizeof volume_keys;++i) platform.keys[i]=volume_keys[i];
            platform.key_head=sizeof volume_keys;
        }
        if(arcade_test) {
            /* Original Options: mode minimum then five increments, down
             * past both sound rows to Arcade time, select its minimum 5s. */
            static const unsigned char arcade_keys[]={0x4d,0x4d,0x4d,0x44,
                0x3d,0x4e,0x4e,0x4e,0x4e,0x4e,0x4d,0x4d,0x4d,0x3d};
            for(unsigned i=0;i<sizeof arcade_keys;++i) platform.keys[i]=arcade_keys[i];
            platform.key_head=sizeof arcade_keys;
            if(mode_transition_test) {
                /* Select each mode using the original minimum/increment keys. */
                unsigned n=5;
                for(unsigned i=0;i<g_slicks_diag_mode_case;++i) platform.keys[n++]=0x4e;
                platform.key_head=(unsigned char)n;
            }
        }
    }
    if(player_menu_test) {
        menu_selection=1;
        if(open_player_menu(&platform,chunky,&configuration,&player_menu_state)) goto cleanup;
        /* Like CONFIG: inject ordinary input events on the target, not game state. */
        static const unsigned char menu_keys[]={0x4d,0x33,0x4e,0x4f,0x4c};
        platform.key_tail=0;
        for(unsigned i=0;i<sizeof menu_keys;++i) platform.keys[i]=menu_keys[i];
        platform.key_head=sizeof menu_keys;
        if(vehicle_save_test) {
            /* Change only slot 2's vehicle shortcut, then exit and save.
             * No picker/editor action may accidentally set the dirty flag. */
            unsigned count=argv[7]=='B'?10:1;
            platform.keys[0]=0x4d; platform.keys[1]=0x4d;
            for(unsigned i=0;i<count;++i) platform.keys[2+i]=0x33;
            platform.keys[2+count]=0x45; platform.keys[3+count]=0x45;
            platform.key_head=(unsigned char)(count+4);
        } else if(argc==8 && (argv[7]=='N' || argv[7]=='C' || persistence_test)) {
            /* Create ABC through the real prompt, then select it for Edit.
             * The main loop supplies subsequent ordinary input events. */
            static const unsigned char name_keys[]={0x4d,0x4d,0x4d,0x4d,0x44,
                0x44,0x20,0x35,0x33,0x44,0x51,0x4d,0x44,0x1d,0x44};
            for(unsigned i=0;i<sizeof name_keys;++i) platform.keys[i]=name_keys[i];
            platform.key_head=sizeof name_keys;
        } else if(argc==8 && argv[7]=='H') {
            /* Fresh setup: shared Human in slots 0/1/3, None in slot 2. */
            static const unsigned char shared_keys[]={0x4d,0x4e,0x4d,0x4f,0x4d,0x4e,0x45,0x45};
            for(unsigned i=0;i<sizeof shared_keys;++i) platform.keys[i]=shared_keys[i];
            platform.key_head=sizeof shared_keys;
        } else if(argc==8 && argv[7]=='E') {
            /* Add, change type/percentage/vehicle, cancel, reopen, then
             * reject F2 with an empty name. Only ordinary input events. */
            static const unsigned char editor_keys[]={0x4d,0x4d,0x4d,0x4d,0x44,
                0x4d,0x44,0x4e,0x4d,0x4e,0x45,0x44,0x51};
            for(unsigned i=0;i<sizeof editor_keys;++i) platform.keys[i]=editor_keys[i];
            platform.key_head=sizeof editor_keys;
        } else if(argc==8 && argv[7]=='D') {
            /* Delete row, cancel one prompt with N, then confirm with Y.
             * Memory-only profile lifecycle; no profile-file writer here. */
            static const unsigned char delete_keys[]={0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,
                0x44,0x4d,0x44,0x36,0x44,0x4d,0x44,0x15};
            for(unsigned i=0;i<sizeof delete_keys;++i) platform.keys[i]=delete_keys[i];
            platform.key_head=sizeof delete_keys;
        } else if(argc==8 && argv[7]=='R') {
            static const unsigned char handoff_keys[]={0x45,0x44,0x45,0x4c,0x44};
            for(unsigned i=0;i<sizeof handoff_keys;++i) platform.keys[sizeof menu_keys+i]=handoff_keys[i];
            platform.key_head=sizeof menu_keys+sizeof handoff_keys;
        } else if(argc==8 && argv[7]=='S') {
            /* Full-catalogue scrolling fixture: both bracket and numeric
             * keypad page keys, then accept/reopen/cancel. */
            static const unsigned char scroll_keys[]={0x44,0x1b,0x1b,0x1f,
                0x1a,0x3f,0x1d,0x3d,0x1a,0x1d,0x44,0x44,0x1a,0x45};
            for(unsigned i=0;i<sizeof scroll_keys;++i) platform.keys[i]=scroll_keys[i];
            platform.key_head=sizeof scroll_keys;
        } else if(argc==8 && (argv[7]=='K' || argv[7]=='G')) {
            static const unsigned char picker_keys[]={0x44,0x4c,0x44,0x44,0x4d,0x45};
            for(unsigned i=0;i<sizeof picker_keys;++i) platform.keys[sizeof menu_keys+i]=picker_keys[i];
            platform.key_head=sizeof menu_keys+sizeof picker_keys;
            if(argv[7]=='G') {
                static const unsigned char race_keys[]={0x45,0x4c,0x44};
                for(unsigned i=0;i<sizeof race_keys;++i) platform.keys[platform.key_head+i]=race_keys[i];
                platform.key_head+=sizeof race_keys;
            }
        }
    }
    if (restore_test || g_slicks_diag_force_exit) {
        result = 0;
        goto cleanup;
    }

    if (auto_race) {
        enter_prepared_race(&platform, logical, race);
        if(setup_input_test) {
            /* Real input-event path: hold default group-0 Q and group-1
             * keypad 7 for the two selected human drivers (0 and 2). */
            platform.key_tail=0;
            platform.keys[0]=0x10; platform.keys[1]=0x3d;
            platform.key_head=2;
        }
        if (g_slicks_diag_audio_in_blank)
            slicks_amiga_platform_wait_display_blank(&platform);
        start_race_engines(&audio,race,&platform);
        g_slicks_diag_engine_sample_block = audio.engine_sample_block;
    }

    /* NATIVE and diagnostic fixtures opt into live inspection. Outer-only
     * benchmarks measure the normal presentation path, not rectangle stats. */
    /* NATURALO<track>Q audits the normal presentation path without relying
     * on debugger writes to target variables. Other audit checks stay on. */
    g_slicks_diag_live_stats=(unsigned char)(!(actor_case_test && argc==10) &&
        (continuous_diagnostics || (g_slicks_diag_profile_all && g_slicks_diag_profile_all!=2)));
    for (;;) {
        unsigned short code;
        if(demo_lifecycle_test && platform.key_head==platform.key_tail) {
            if(demo_lifecycle_test==5 && g_slicks_diag_ingame && race->frame_count>15000) {
                g_slicks_demo_test_error=14;slicks_diag_demo_test_done();goto cleanup;
            }
            static struct SlicksConfiguration before;
            static short playlist_before[256];static unsigned short count_before;
            static unsigned long idle_test_started;
            static unsigned long menu_wait_started;
            static unsigned char save_edit_done;
            if(demo_test_stage>=8 && demo_test_stage<=12 &&
                (g_slicks_diag_ingame || platform.vblank_count-idle_test_started>10000)) {
                g_slicks_demo_test_error=9;slicks_diag_demo_test_done();goto cleanup;
            }
            if(!demo_test_stage && !g_slicks_diag_ingame) {
                if((demo_lifecycle_test==8 || demo_lifecycle_test==10) && !save_edit_done) {
                    /* Make a real Options edit, not a forced dirty flag. */
                    static const unsigned char edit_keys[]={0x4d,0x4d,0x4d,0x44,0x4d,0x3d,0x4e,0x45};
                    platform.key_tail=0;
                    for(unsigned i=0;i<sizeof edit_keys;++i) platform.keys[i]=edit_keys[i];
                    platform.key_head=sizeof edit_keys;save_edit_done=1;demo_test_stage=13;
                    continue;
                }
                if(demo_lifecycle_test==4) {
                    unsigned long before_bitmap=checksum_bitmap(platform.views[0].bitmap);
                    if(slicks_amiga_platform_begin_io(&platform)) goto cleanup;
                    BPTR file=Open((CONST_STRPTR)"SLICKS.000",MODE_OLDFILE);
                    unsigned char block[256];long got;
                    g_slicks_loading_io_bytes=0;g_slicks_loading_io_hash=0;
                    if(!file) goto cleanup;
                    while((got=Read(file,block,sizeof block))>0) {
                        g_slicks_loading_io_bytes+=(unsigned long)got;
                        for(long i=0;i<got;++i)
                            g_slicks_loading_io_hash=g_slicks_loading_io_hash*33UL+block[i];
                    }
                    Close(file);
                    /* A file cannot be traversed as a directory. Exercise a
                     * real DOS open failure without modifying any assets. */
                    BPTR missing=Open((CONST_STRPTR)"SLICKS.000/no-file",MODE_OLDFILE);
                    if(missing) {
                        Close(missing);g_slicks_demo_test_error=13;
                        slicks_diag_demo_test_done();goto cleanup;
                    }
                    for(unsigned i=0;i<50;++i) slicks_amiga_platform_wait_vblank(&platform);
                    if(got<0 || !g_slicks_loading_io_bytes || platform.gfx_base->ActiView ||
                        before_bitmap!=checksum_bitmap(platform.views[0].bitmap)) {
                        g_slicks_demo_test_error=12;slicks_diag_demo_test_done();goto cleanup;
                    }
                    if(slicks_amiga_platform_end_io(&platform)) goto cleanup;
                    ++g_slicks_loading_io_checks;
                }
                before=configuration;count_before=g_slicks_track_playlist.count;
                g_slicks_demo_expected_configuration=&before;
                for(unsigned i=0;i<256;++i) playlist_before[i]=track_selection[i];
                demo_expected_playlist=playlist_before;demo_expected_playlist_count=count_before;
                static const unsigned char keys[]={0x60,0x51,0xd1,0xe0};
                unsigned char shifts=0;platform.key_tail=0;
                for(unsigned i=0;i<sizeof keys;++i)
                    platform.keys[i]=slicks_amiga_key_event(keys[i],&shifts);
                platform.key_head=sizeof keys;demo_test_stage=1;
                if(demo_lifecycle_test==2) {
                    platform.key_head=0;demo_test_stage=6;
                    idle_test_started=platform.vblank_count;
                }
                if(demo_lifecycle_test==3) {
                    static const unsigned char menu_keys[]={0x4d,0x4d,0x4d,0x44};
                    for(unsigned i=0;i<sizeof menu_keys;++i) platform.keys[i]=menu_keys[i];
                    platform.key_head=sizeof menu_keys;demo_test_stage=8;
                    idle_test_started=platform.vblank_count;
                }
            } else if(demo_test_stage==13 && !g_slicks_options_menu) {
                if(configuration.options[1]!=slicks_original_option_specs[1].minimum+
                    slicks_original_option_specs[1].step || !g_slicks_options_state.dirty) {
                    g_slicks_demo_test_error=17;slicks_diag_demo_test_done();goto cleanup;
                }
                demo_test_stage=0;
            } else if((demo_lifecycle_test==8 || demo_lifecycle_test==10) && demo_test_stage==4 && save_prompt) {
                const unsigned char *actual=(const unsigned char *)&configuration;
                const unsigned char *expected=(const unsigned char *)&before;
                for(unsigned i=0;i<sizeof before;++i)
                    if(actual[i]!=expected[i]) g_slicks_demo_test_error=2;
                for(unsigned i=0;i<256;++i)
                    if(track_selection[i]!=playlist_before[i]) g_slicks_demo_test_error=3;
                if(title_demo.active || race->demo_flag ||
                   g_slicks_track_playlist.count!=count_before) g_slicks_demo_test_error=4;
                if(g_slicks_demo_test_error) {slicks_diag_demo_test_done();goto cleanup;}
                platform.key_tail=0;platform.keys[0]=demo_lifecycle_test==10?0x44:0x45;platform.key_head=1;
                if(demo_lifecycle_test==10) {
                    /* Supply a complete key press: exit registration waits
                     * for release before its normal timeout may expire. */
                    platform.keys[1]=0xc4;platform.key_head=2;
                }
            } else if(demo_test_stage==8 && g_slicks_options_menu) {
                platform.key_tail=0;platform.keys[0]=0x50;platform.key_head=1;
                demo_test_stage=9;
            } else if(demo_test_stage==9 && g_slicks_options_menu && g_slicks_options_menu->help) {
                menu_wait_started=platform.vblank_count;demo_test_stage=10;
            } else if(demo_test_stage==10 && platform.vblank_count-menu_wait_started>=1100) {
                if(!g_slicks_options_menu || !g_slicks_options_menu->help || title_demo.active) {
                    g_slicks_demo_test_error=10;slicks_diag_demo_test_done();goto cleanup;
                }
                ++g_slicks_demo_menu_waits;
                platform.key_tail=0;platform.keys[0]=0x45;platform.key_head=1;
                menu_wait_started=platform.vblank_count;demo_test_stage=11;
            } else if(demo_test_stage==11 && platform.vblank_count-menu_wait_started>=1100) {
                if(!g_slicks_options_menu || g_slicks_options_menu->help || title_demo.active) {
                    g_slicks_demo_test_error=11;slicks_diag_demo_test_done();goto cleanup;
                }
                ++g_slicks_demo_menu_waits;
                platform.key_tail=0;platform.keys[0]=0x45;platform.key_head=1;
                demo_test_stage=12;
            } else if(demo_test_stage==12 && !g_slicks_options_menu) {
                demo_idle_input_at=platform.vblank_count;demo_test_stage=7;
            } else if(demo_test_stage==6 &&
                platform.vblank_count-idle_test_started>=500) {
                /* Real ten-second wait, then ordinary navigation must restart
                 * the idle clock. No clock or elapsed-state injection. */
                platform.key_tail=0;platform.keys[0]=0x4d;platform.key_head=1;
                demo_idle_input_at=platform.vblank_count;
                demo_test_stage=7;
            } else if(demo_test_stage==7 && g_slicks_diag_ingame) {
                if(platform.vblank_count-idle_test_started<1500)
                    g_slicks_demo_test_error=7;
                demo_test_stage=1;
            } else if((demo_lifecycle_test==6 || demo_lifecycle_test==7 || demo_lifecycle_test==9 || demo_lifecycle_test==12) && !demo_test_round &&
                demo_test_stage==1 && race_load_prompt) {
                if(title_demo.active || race->demo_flag ||
                    g_slicks_diag_race_error!=(demo_lifecycle_test==12?8:demo_lifecycle_test==9?1:demo_lifecycle_test==6?2:6)) {
                    g_slicks_demo_test_error=16;slicks_diag_demo_test_done();goto cleanup;
                }
                platform.key_tail=0;platform.keys[0]=0x44;platform.key_head=1;
                demo_test_stage=4;
            } else if(demo_test_stage>=1 && demo_test_stage<=3 &&
                g_slicks_diag_ingame && race->frame_count>=10) {
                if(!title_demo.active || race->demo_flag!=-1 || race->race_mode!=5 ||
                   g_slicks_track_playlist.count!=1) g_slicks_demo_test_error=1;
                unsigned char keys[4]={0x60,0x50,0xd0,0xe0};
                unsigned count=4;
                if(demo_test_stage==2) {keys[1]=0x51;keys[2]=0xd1;}
                if(demo_test_stage==3) {keys[0]=0x33;count=demo_lifecycle_test==5?0:1;}
                if(demo_test_stage==3 && (demo_lifecycle_test==8 || demo_lifecycle_test==10 || demo_lifecycle_test==11)) {count=0;exit_requested=1;}
                unsigned char shifts=0;platform.key_tail=0;
                for(unsigned i=0;i<count;++i)
                    platform.keys[i]=slicks_amiga_key_event(keys[i],&shifts);
                platform.key_head=(unsigned char)count;++demo_test_stage;
            } else if(demo_test_stage==4 && !g_slicks_diag_ingame && !save_prompt) {
                const unsigned char *actual=(const unsigned char *)&configuration;
                const unsigned char *expected=(const unsigned char *)&before;
                for(unsigned i=0;i<sizeof before;++i)
                    if(actual[i]!=expected[i]) g_slicks_demo_test_error=2;
                for(unsigned i=0;i<256;++i)
                    if(track_selection[i]!=playlist_before[i]) g_slicks_demo_test_error=3;
                if(title_demo.active || race->demo_flag ||
                   g_slicks_track_playlist.count!=count_before) g_slicks_demo_test_error=4;
                if(++demo_test_round<2 && !g_slicks_demo_test_error) demo_test_stage=0;
                else {
                    slicks_diag_demo_test_done();exit_requested=1;demo_test_stage=5;
                }
            }
        }
        if(registration_test && title_dirty_test) {
            static const unsigned char keys[]={0x4d,0x4d,0x4f,0x4e,0x4c,0x4c,0x45};
            static const unsigned char transition_keys[]={
                0x4d,0x44,0x4f,0x45, /* Human -> Computer; return to title. */
                0x44,0x4f,0x45,      /* Computer -> None; compact icons. */
                0x44,0x4e,0x4e,0x45, /* None -> Computer -> Human. */
                0x4d,0x4f,0x4e,      /* TRACKS: 195 -> 194 -> 195. */
                0x4d,0x4e,0x4e,0x4e,0x4e, /* OPTIONS: all normal badges. */
                0x4f,0x4f,0x4f,0x4f};
            unsigned at=registration_test-1;
            if(title_dirty_test==4 || title_dirty_test==5) {
                static const unsigned char arcade_keys[]={0x4d,0x4d,0x4d,
                    0x4e,0x4e,0x4e,0x4e,0x4e, /* mode 0 -> Arcade */
                    0x4c,0x4c,0x4c, /* visual row zero */
                    0x4e,0x4e,0x4e,0x4e,0x4f,0x4f,0x4f,0x4e,
                    0x4d,0x44,0x45,0x4c,0x44}; /* Options, return, GO */
                if(title_dirty_test==5 && at==sizeof arcade_keys-2) {
                    /* F9 from Settings must start, not activate Options. */
                    static const unsigned char f9=0x58;
                    championship_test_keys(&platform,&f9,1);registration_test=0;
                } else if(at==sizeof arcade_keys) registration_test=0;
                else {championship_test_keys(&platform,&arcade_keys[at],1);++registration_test;}
            } else if(title_dirty_test==3) {
                if(++registration_test==74) {g_slicks_diag_force_exit=1;registration_test=0;}
            } else if(title_dirty_test==2) {
                if(at==sizeof transition_keys) {
                    /* Rendering/input fixture only: do not save its edits. */
                    g_slicks_diag_force_exit=1;registration_test=0;
                } else {
                    championship_test_keys(&platform,&transition_keys[at],1);
                    ++registration_test;
                }
            } else {
                championship_test_keys(&platform,&keys[at],1);
                if(++registration_test>sizeof keys) registration_test=0;
            }
        } else if(registration_test && ++registration_test==10) {
            /* Exercise the title pulse before ordinary Escape make/release.
             * Key file, trial date, save handling and clocks are untouched. */
            platform.key_tail=0;platform.keys[0]=0x45;platform.keys[1]=0xc5;platform.key_head=2;
            registration_test=0;
        }
        if(shop_transition_test && g_slicks_diag_ingame && platform.key_head==platform.key_tail) {
            unsigned char key=0;
            if(shop_transition_test==5) {
                /* Real computer shopping on successive races, then real AI
                 * controls. Never populate AI ammunition/projectiles here. */
                if(race->weapons.shots || race->frame_count>=450)
                    key=race->weapons.shots?0x59:0x58;
            } else if(!shop_transition_phase && race->frame_count>=200) {key=0x45;shop_transition_phase=1;}
            else if(shop_transition_phase==1 && race->frame_count>=230) {
                key=0x58;shop_transition_phase=2;
                if(shop_transition_test==4) {championship_test=1;championship_test_stage=2;}
            } else if(shop_transition_phase==2 && shop_track_position && race->frame_count>=100) {key=0x59;shop_transition_phase=3;}
            if(key) championship_test_keys(&platform,&key,1);
        }
        if((championship_test==1 || championship_test==3 || championship_test==6) && !championship_test_stage && g_slicks_track_menu && platform.key_head==platform.key_tail) {
            static const unsigned char keys[]={0x4e,0x4d,0x4d,0x4d,0x44,0x4f,0x44,0x4d,0x44,0x4d,0x44,0x45,0x4c,0x4c,0x44};
            championship_test_keys(&platform,keys,sizeof keys); championship_test_stage=1;
        }
        if(championship_test && g_slicks_diag_ingame && race->frame_count>=40 && championship_test_stage<2) {
            unsigned char key=championship_test==2?0x59:0x58;
            championship_test_keys(&platform,&key,1); championship_test_stage=2;
        }
        if(championship_test==2 && championship_test_stage==2 && !g_slicks_diag_ingame) {
            static const unsigned char keys[]={0x45};
            championship_test_keys(&platform,keys,1); championship_test_stage=3;
        }
        if (!g_slicks_diag_ingame)
            slicks_amiga_platform_wait_vblank(&platform);
        if((title_dirty_test==4 || title_dirty_test==5) && g_slicks_diag_ingame && race->frame_count>=2)
            g_slicks_diag_force_exit=1;
        if (g_slicks_diag_force_exit || (natural_results_test && argv[7]=='O' &&
            g_slicks_diag_ingame && race->frame_count>=600) ||
            (natural_results_test && audio_pcm_test && g_slicks_diag_ingame && race->frame_count>=150) ||
            (pause_live_test && pause_live_sent && race->frame_count>=150)) {
            result = 0;
            goto cleanup;
        }
        setup_dirty|=player_menu_state.dirty;
        setup_dirty|=race_statistics_dirty;
        setup_dirty|=g_slicks_options_state.dirty;
        unsigned char right_down=(unsigned char)!!slicks_amiga_platform_right_mouse();
        if(g_slicks_diag_ingame && race->participation_ready)
            for(unsigned driver=0;driver<4;++driver)
                if(race->participation[driver]<0 && configuration.player_input[driver]==2)
                    right_down=0; /* Port-zero second button belongs to the driver. */
        if(right_down && !right_was_down && !save_prompt) exit_requested=1;
        right_was_down=right_down;
        if(exit_requested) {
            exit_requested=0;
            /* Native program-exit adaptation must never persist temporary
             * demo options or profiles, even if profile statistics changed. */
            if(title_demo.active) {
                restore_demo_configuration(&configuration,race,(unsigned char)g_slicks_diag_ingame);
                selected_track=demo_track;selected_vehicle=demo_vehicle;selected_laps=demo_laps;
            }
            /* Only explicit persistence fixtures use this transaction.
             * The normal caller saves on exit, not on every modal close. */
            if(demo_lifecycle_test==11) {
                const unsigned char *actual=(const unsigned char *)&configuration;
                const unsigned char *expected=(const unsigned char *)g_slicks_demo_expected_configuration;
                if(!expected || !demo_expected_playlist || setup_dirty || title_demo.active || race->demo_flag)
                    g_slicks_demo_test_error=20;
                else {
                    for(unsigned i=0;i<sizeof configuration;++i)
                        if(actual[i]!=expected[i]) g_slicks_demo_test_error=2;
                    for(unsigned i=0;i<256;++i)
                        if(track_selection[i]!=demo_expected_playlist[i]) g_slicks_demo_test_error=3;
                    if(g_slicks_track_playlist.count!=demo_expected_playlist_count) g_slicks_demo_test_error=4;
                }
                slicks_diag_demo_test_done();
                if(g_slicks_demo_test_error) goto cleanup;
            }
            if((demo_lifecycle_test && demo_lifecycle_test!=8 && demo_lifecycle_test!=10 && demo_lifecycle_test!=11) || (argc && language_choice_test!=2 && !natural_results_test && !persistence_test && !shared_human_test && !volume_save_test && !arcade_save_test && !vehicle_save_test && !pause_save_test && !(sequence_test && argv[7]=='B')) || !setup_dirty) { result=0; goto cleanup; }
            slicks_amiga_platform_end(&platform);
            if(((persistence_test && (argv[7]=='T' || argv[7]=='V')) || demo_lifecycle_test==8 || demo_lifecycle_test==10) && !failure_injected) {
                /* Isolated diagnostic fault, using AmigaDOS throughout so
                 * emulator host-directory caching cannot affect recovery. */
                BPTR obstruction=CreateDir((CONST_STRPTR)"SLICKS.CFG.new");
                if(!obstruction) goto cleanup;
                UnLock(obstruction); failure_injected=1;
            }
            g_slicks_setup_save_report=slicks_amiga_store_setup(&configuration,&g_slicks_profiles,
                SLICKS_AMIGA_CONFIG_SIGNATURE);
            slicks_diag_setup_saved();
            if(g_slicks_setup_save_report.result==SLICKS_SETUP_SAVED) {
                if(demo_lifecycle_test==8 || demo_lifecycle_test==10) {
                    unsigned char disk[142],expected[142];
                    if(demo_test_round!=(demo_lifecycle_test==10?0:1) || load_plain_file("SLICKS.CFG",disk,sizeof disk)!=142 ||
                       slicks_save_configuration(g_slicks_demo_expected_configuration,expected,sizeof expected,
                           SLICKS_AMIGA_CONFIG_SIGNATURE)!=142) {
                        g_slicks_demo_test_error=18;slicks_diag_demo_test_done();goto cleanup;
                    }
                    for(unsigned i=0;i<sizeof disk;++i) if(disk[i]!=expected[i]) g_slicks_demo_test_error=19;
                    g_slicks_demo_saved_roundtrip=1;slicks_diag_demo_test_done();
                }
                setup_dirty=0; result=0; goto cleanup;
            }
            /* Keep live settings and ownership on failure. This platform
             * error screen is intentionally not presented as original DOS UI. */
            save_prompt=1;
            if(g_slicks_setup_save_report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING) {
                setup_dirty=0; race_statistics_dirty=0;
                player_menu_state.dirty=0; g_slicks_options_state.dirty=0; save_prompt=2;
            }
            slicks_amiga_audio_stop(&audio);
            g_slicks_diag_ingame=0;
            clear_title_rectangle(logical,0,0,320,200);
            slicks_draw_title_text(logical,save_prompt==2?"SAVED - BACKUP CLEANUP FAILED":"SETUP SAVE FAILED",160,70,15);
            slicks_draw_title_text(logical,g_slicks_setup_save_report.result==SLICKS_SETUP_RECOVERY_REQUIRED?
                "PRESERVE NEW AND BAK FILES":"YOUR SETTINGS ARE STILL IN MEMORY",160,90,15);
            slicks_draw_title_text(logical,save_prompt==2?"ENTER TO EXIT":"ENTER RETRIES - ESC RETURNS",160,110,15);
            slicks_convert_to_amiga(logical,chunky,platform.views[0].bitmap);
            if(slicks_amiga_platform_set_view(&platform,0,source_palette) ||
               slicks_amiga_platform_begin(&platform,0)) goto cleanup;
            slicks_diag_setup_save_failed();
            if(failure_injected==1) {
                slicks_amiga_platform_end(&platform);
                if(!DeleteFile((CONST_STRPTR)"SLICKS.CFG.new")) goto cleanup;
                failure_injected=2;
                if(slicks_amiga_platform_begin(&platform,0)) goto cleanup;
            }
            if(persistence_test && save_prompt==1) {
                /* Diagnostic retry is still ordinary input; the fault and
                 * recovery are supplied outside the target's game state. */
                platform.key_tail=0; platform.keys[0]=0x44; platform.key_head=1;
                if(argv[7]=='X') {
                    platform.keys[0]=0x45; platform.keys[1]=0x44; platform.key_head=2;
                }
                if(argv[7]=='V') {
                    /* Cancel failure, reopen Players, close, and exit again.
                     * The second exit must still save the retained edits. */
                    static const unsigned char keys[]={0x45,0x44,0x45,0x45};
                    for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
                    platform.key_head=sizeof keys;
                }
            }
        }
        if(pause_live_test && pause_live_sent<(pause_failure_test?6:1) && g_slicks_diag_ingame && race->frame_count>=100U+pause_live_sent) {
            g_slicks_diag_target_frame=150; race->profile_frame=150;
            platform.key_tail=0; platform.keys[0]=0x45; platform.key_head=1; ++pause_live_sent;
        }
        if(setup_abort_test && !setup_abort_sent && g_slicks_diag_ingame &&
           race->frame_count>=10 && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.keys[0]=0x45; platform.key_head=1;
            setup_abort_sent=1;
        }
        if(pause_save_test && g_slicks_diag_ingame && sequence_returns<2 &&
            race->frame_count>=100U+50U*sequence_returns && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.keys[0]=(unsigned char)(sequence_returns?0x59:0x45);
            platform.key_head=1; ++sequence_returns;
        }
        if(pause_transition_test && !pause_save_test && g_slicks_diag_ingame && race->frame_count>=100 &&
            sequence_returns<playlist_position+1 && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.keys[0]=(unsigned char)(playlist_position?0x59:0x58);
            platform.key_head=1; ++sequence_returns;
        }
        if((title_demo.active || (original_setup && !argc) || ((sequence_test || completion_return_test) &&
            !pause_transition_test && sequence_returns<playlist_position+1)) &&
            g_slicks_diag_ingame && race->race_complete && platform.key_head==platform.key_tail) {
            /* Natural completion returns to the original post-race caller;
             * share the existing advance handler without another key wait. */
            platform.key_tail=0; platform.keys[0]=0x44; platform.key_head=1;
            ++sequence_returns;
        }
        for (;;) {
            unsigned char title_owner=(unsigned char)(original_setup &&
                !g_slicks_diag_ingame && !save_prompt && !race_load_prompt &&
                !service_menu_open && !g_slicks_player_menu && !g_slicks_options_menu &&
                !g_slicks_track_menu && !g_slicks_title_help &&
                !g_slicks_title_help_warning && !g_slicks_diag_saved_menu);
            /* PAL interrupt time advances independently of rendered updates.
             * Quantize to whole seconds like DOS time()*1000. Ownership entry
             * and the tail after input/modal handling restart the interval. */
            unsigned long title_now=title_owner?(platform.vblank_count/50UL)*1000UL:0;
            if(!title_owner) title_idle_active=0;
            else if(!title_idle_active || title_idle_reset) {
                title_idle_started=title_now;title_idle_active=1;
            }
            title_idle_reset=0;
            unsigned char have_key=(unsigned char)slicks_amiga_platform_poll_key(&platform,&code);
            unsigned short title_scan=have_key?amiga_raw_to_demo_scan(code,platform.key_shifts):0;
            unsigned char idle_demo=(unsigned char)(title_owner &&
                slicks_title_demo_scan(0,title_now,title_idle_started)==0x58);
            if(!have_key && !idle_demo) break;
            if(idle_demo) {
                /* Original replaces even a simultaneous scan when overdue. */
                title_scan=0x58;code=0x100;
                if(demo_lifecycle_test>=2) {
                    ++g_slicks_demo_idle_entries;
                    g_slicks_demo_idle_wait_frames=platform.vblank_count-demo_idle_input_at;
                    if(g_slicks_demo_idle_wait_frames<1000 || title_now-title_idle_started!=21000)
                        g_slicks_demo_test_error=8;
                }
            }
            if(title_owner && title_scan) title_idle_reset=1;
            if(race_load_prompt) {
                if(code==0x44 && race_load_retry) {
                    /* Retry only preparation: rewards, selection refresh and
                     * playlist advancement already happened exactly once. */
                    g_slicks_diag_ready=0;
                    slicks_amiga_platform_end(&platform);
                    if(prepare_race(&platform,logical,chunky,mode_state,race,
                        selected_track_path,race_palette,selected_vehicle,
                        &configuration,&g_slicks_setup_session,0)) {
                        if(show_race_load_error(&platform,logical,chunky,mode_state,source_palette,1)) goto cleanup;
                        continue;
                    }
                    race_load_prompt=race_load_retry=0;
                    slicks_race_set_laps(race,selected_laps);
                    if(slicks_amiga_platform_begin(&platform,1)) goto cleanup;
                    g_slicks_diag_ready=1;
                    enter_prepared_race(&platform,logical,race);
                    start_race_engines(&audio,race,&platform);
                    g_slicks_diag_engine_sample_block=audio.engine_sample_block;
                    continue;
                }
                if(code==0x44 || code==0x45) {
                    race_load_prompt=race_load_retry=0;
                    make_title_surface(logical,title_frame,source_palette);
                    redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                        selected_vehicle,track_names[selected_track],selected_laps);
                    slicks_diag_race_load_dismissed();
                }
                continue;
            }
            if(save_prompt) {
                if(code==0x44) {
                    if(save_prompt==2) { result=0; goto cleanup; }
                    exit_requested=1;
                } else if(code==0x45) {
                    save_prompt=0;
                    /* Return to the title even when the exit request originated
                     * inside a modal or race. Edited profile data stays alive.
                     * All assets are resident; retain hardware ownership. */
                    slicks_amiga_player_menu_destroy(g_slicks_player_menu); g_slicks_player_menu=0;
                    slicks_amiga_player_menu_destroy(g_slicks_options_menu); g_slicks_options_menu=0;
                    slicks_amiga_player_menu_destroy(g_slicks_track_menu); g_slicks_track_menu=0;
                    g_slicks_track_renderer.surface=0;
                    slicks_amiga_player_menu_destroy(g_slicks_title_help); g_slicks_title_help=0;
                    g_slicks_options_renderer.surface=0;
                    g_slicks_options_configuration=0;
                    service_menu_open=0;
                    make_title_surface(logical,title_frame,source_palette);
                    redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                        selected_vehicle,track_names[selected_track],selected_laps);
                    if(show_menu(&platform)) goto cleanup;
                    slicks_diag_setup_save_cancelled();
                }
                continue;
            }
            if (g_slicks_diag_ingame) {
                int pause_result=0;
                unsigned char scan=(unsigned char)amiga_raw_to_dos_scan(code);
                if(title_demo.active) {
                    scan=(unsigned char)amiga_raw_to_demo_scan(code,platform.key_shifts);
                    if(code&128) continue;
                    if(scan==0x57 || scan==0x58) {
                        struct SlicksChunkyUi ui={chunky,race_palette,0,0};
                        if(slicks_track_data_view_draw(&ui,race->material_map,
                            race->surface_map,(unsigned char)(scan-0x57))) goto cleanup;
                        slicks_race_invalidate_retention(race);
                        slicks_amiga_platform_wait_display_blank(&platform);
                        slicks_chunky_rect_to_amiga(chunky,platform.views[1].bitmap,
                            0,0,320,190,0);
                        if(demo_lifecycle_test) {
                            const unsigned char *source=scan==0x57?race->material_map:race->surface_map;
                            for(unsigned at=0;at<320U*190U;++at)
                                if(chunky[at]!=(source[at]&31)) g_slicks_demo_test_error=5;
                            const struct BitMap *bitmap=platform.views[1].bitmap;
                            for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
                                unsigned char pixel=0;
                                for(unsigned p=0;p<8;++p)
                                    if(bitmap->Planes[p][y*bitmap->BytesPerRow+(x>>3)] & (128U>>(x&7)))
                                        pixel|=(unsigned char)(1U<<p);
                                if(pixel!=chunky[mult320[y]+x]) g_slicks_demo_test_error=6;
                            }
                            ++g_slicks_demo_test_views;
                        }
                        continue;
                    }
                    if(!slicks_title_demo_exit_key(race->demo_flag,scan)) continue;
                    if(demo_lifecycle_test==5) {
                        unsigned n=g_slicks_demo_natural_returns;
                        if(!race->race_complete || n>=2 || race->frame_count<=10 ||
                            !slicks_finish_expired(race->game_clock_ticks,race->finish_deadline)) {
                            g_slicks_demo_test_error=15;slicks_diag_demo_test_done();goto cleanup;
                        }
                        g_slicks_demo_return_frames[n]=race->frame_count;
                        g_slicks_demo_return_clocks[n]=race->game_clock_ticks;
                        g_slicks_demo_return_deadlines[n]=race->finish_deadline;
                        ++g_slicks_demo_natural_returns;
                    }
                    slicks_amiga_platform_wait_display_blank(&platform);
                    slicks_amiga_audio_stop(&audio);
                    restore_demo_configuration(&configuration,race,1);
                    selected_track=demo_track;selected_vehicle=demo_vehicle;selected_laps=demo_laps;
                    if(slicks_setup_basic_mode(logical,mode_state)) goto cleanup;
                    make_title_surface(logical,title_frame,source_palette);
                    redraw_title_configuration(&platform,logical,chunky,source_palette,
                        menu_selection,selected_vehicle,track_names[selected_track],selected_laps);
                    if(slicks_amiga_platform_set_view(&platform,0,source_palette)) goto cleanup;
                    slicks_amiga_platform_show(&platform,0);
                    g_slicks_diag_ingame=0;
                    continue;
                }
                if((!argc || championship_test || pause_live_test || pause_transition_test || shop_transition_test) && !race->race_complete && !(code&128) &&
                   (scan==1 || scan==0x1d || scan==0x3b || scan==0x3c || scan==0x43 || scan==0x44)) {
                    pause_result=run_race_pause(&platform,&audio,race,chunky,race_palette,&configuration,
                        &setup_dirty,(unsigned char)(scan==0x43?4:scan==0x44?5:0),
                        (unsigned char)(shop_transition_test?7:pause_nested_test?8:pause_transition_test?(pause_save_test && sequence_returns==1?1:7):pause_failure_test && pause_live_sent<=5?pause_live_sent+1:pause_live_test));
                    if(pause_result==-3) goto cleanup;
                    /* The pause surface may reuse the chunky buffer. */
                    slicks_race_invalidate_retention(race);
                    if(pause_result==1) continue;
                }
                if (!(code & 0x80) &&
                    (pause_result<0 || (code & 0x7f) == 0x45 ||
                     (race->race_complete && (code & 0x7f) == 0x44))) {
                    unsigned char advance=(unsigned char)(pause_result==-1 ||
                        (race->race_complete && (code&0x7f)==0x44));
                    slicks_amiga_platform_wait_display_blank(&platform);
                    slicks_amiga_audio_stop(&audio);
                    /* Original race-loop return reaches 2557e for a normal
                     * game even on early exit; only demo DS:0459 skips it.
                     * Completed races have already paid before results. */
                    if(original_setup) slicks_race_award_track(race);
                    if(original_setup) g_slicks_setup_session.random_state=race->random_state;
                    if(original_setup && run_record_results(&platform,race,chunky,race_palette,
                        selected_track_path,(unsigned char)(argc!=0))) goto cleanup;
                    if(original_setup && (race->race_complete || advance))
                        slicks_setup_after_race(&g_slicks_setup_session,&configuration,&setup_resources);
                    if(original_setup && advance && !shop_end_game &&
                       (int)playlist_position+1 < slicks_arcade_track_count(configuration.options[0],
                            configuration.options[14],(short)g_slicks_track_playlist.count)) {
                        char next_path[64];
                        const unsigned char *next_name=(const unsigned char *)track_names[track_selection[playlist_position+1]];
                        make_track_path(next_path,(const char *)next_name);
                        int choice=run_intermission(&platform,race,chunky,race_palette,next_path,next_name,
                            (short)playlist_position,(short)slicks_arcade_track_count(configuration.options[0],
                                configuration.options[14],(short)g_slicks_track_playlist.count),shop_transition_test==4?0:shop_transition_test?1:intermission_retry_test?intermission_retry_test+2:intermission_live_test?2:sequence_test,
                            track_names,track_count);
                        if(choice<0) goto cleanup;
                        advance=(unsigned char)(choice==1);
                    }
                    if(original_setup && advance && !shop_end_game &&
                        (int)playlist_position+1 < slicks_arcade_track_count(
                            configuration.options[0],configuration.options[14],
                            (short)g_slicks_track_playlist.count)) {
                        /* Continue the same game: no inventory initialization
                         * or RNG reseeding. The original selection refresh
                         * above may deliberately reroll random-per-race cars. */
                        for(unsigned driver=0;driver<4;++driver)
                            for(unsigned item=0;item<13;++item)
                                g_slicks_setup_session.inventory[driver][item]=race->weapon_inventory[driver][item];
                        ++playlist_position;
                        shop_track_position=(short)playlist_position;
                        selected_track=(unsigned short)track_selection[playlist_position];
                        make_track_path(selected_track_path,track_names[selected_track]);
                        g_slicks_diag_ready=0; g_slicks_diag_ingame=0;
                        slicks_amiga_platform_end(&platform);
                        if((sequence_failure_test || shop_transition_test==2) && playlist_position==1)
                            g_slicks_diag_race_load_fault=6;
                        if(prepare_race(&platform,logical,chunky,mode_state,race,
                            selected_track_path,race_palette,selected_vehicle,
                            &configuration,&g_slicks_setup_session,0)) {
                            race_load_prompt=race_load_retry=1;
                            if(show_race_load_error(&platform,logical,chunky,mode_state,source_palette,1)) goto cleanup;
                            if(sequence_failure_test || shop_transition_test==2) {
                                platform.key_tail=0; platform.keys[0]=0x44; platform.key_head=1;
                            }
                            continue;
                        }
                        slicks_race_set_laps(race,selected_laps);
                        if(slicks_amiga_platform_begin(&platform,1)) goto cleanup;
                        g_slicks_diag_ready=1;
                        enter_prepared_race(&platform,logical,race);
                        start_race_engines(&audio,race,&platform);
                        continue;
                    }
                    if(original_setup && run_championship_results(&platform,chunky,race_palette,
                        slicks_speed_timer_argument(configuration.field_05de),&setup_dirty,(unsigned char)(argc!=0))) goto cleanup;
                    if (slicks_setup_basic_mode(logical, mode_state) != 0)
                        goto cleanup;
                    make_title_surface(logical, title_frame, source_palette);
                    redraw_title_configuration(
                        &platform, logical, chunky, source_palette,
                        menu_selection, selected_vehicle,
                        track_names[selected_track],
                        selected_laps);
                    if(slicks_amiga_platform_set_view(&platform,0,source_palette)) goto cleanup;
                    if(!platform.active && slicks_amiga_platform_begin(&platform,0)) goto cleanup;
                    slicks_amiga_platform_show(&platform, 0);
                    g_slicks_diag_ingame = 0;
                    if(completion_return_test || shop_transition_test || intermission_retry_test==2 || (sequence_test && !pause_transition_test)) {
                        /* Complete the natural-race diagnostic with normal
                         * title Escape input, then verify system restoration. */
                        platform.key_tail=0; platform.keys[0]=0x45; platform.key_head=1;
                    }
                    if(pause_transition_test && sequence_returns==2) {
                        if(pause_save_test) {
                            platform.key_tail=0; platform.keys[0]=0x45; platform.key_head=1;
                        } else g_slicks_diag_force_exit=1;
                    }
                    continue;
                }
                (void)update_race_key(race, code,&configuration);
                continue;
            }
            {
                unsigned char pressed = (unsigned char)!(code & 0x80);
                unsigned short raw = code & 0x7f;
                unsigned char redraw = 0;
                struct SlicksAmigaPlayerMenu *input_menu=g_slicks_title_help?g_slicks_title_help:
                    (g_slicks_player_menu?g_slicks_player_menu:
                    (g_slicks_track_menu?g_slicks_track_menu:g_slicks_options_menu));
                unsigned char character=input_menu?
                    slicks_amiga_menu_character(input_menu,(unsigned char)code):0;
                if (!pressed)
                    continue;
                if(g_slicks_title_help_warning) {
                    if(raw>=0x60 || !amiga_raw_to_dos_scan(code)) continue;
                    unsigned at=0;
                    for(unsigned y=90;y<112;++y) for(unsigned x=20;x<300;++x)
                        logical[y*100UL+(x>>2)+((x&3)<<16)]=title_help_saved[at++];
                    slicks_title_dirty_add(&title_dirty,20,90,300,112);
                    publish_title_dirty(&platform,logical,chunky);
                    g_slicks_title_help_warning=0;
                    if(show_menu(&platform)) goto cleanup;
                    slicks_diag_help_warning_closed(); continue;
                }
                if(input_menu && input_menu->help_warning) {
                    if(raw>=0x60 || (!character && !amiga_raw_to_dos_scan(code))) continue;
                    if(slicks_amiga_help_warning_close(input_menu)) goto cleanup;
                    present_menu_surface(&platform,input_menu);
                    if(input_menu==g_slicks_title_help) {
                        slicks_amiga_player_menu_destroy(g_slicks_title_help); g_slicks_title_help=0;
                    }
                    if(show_menu(&platform)) goto cleanup;
                    slicks_diag_help_warning_closed();
                    continue;
                }
                if(input_menu && input_menu->help) {
                    struct SlicksAmigaHelpKey key=slicks_amiga_help_key((unsigned char)code,character);
                    /* DOS getch returns printable ASCII or an extended scan,
                     * never both. Amiga modifier events have neither. */
                    if(key.ascii || key.scan) {
                        if(slicks_help_viewer_key(input_menu->help,key.ascii,key.scan)) {
                            if(slicks_amiga_help_close(input_menu) || slicks_amiga_help_warning_open(input_menu)) goto cleanup;
                            present_menu_surface(&platform,input_menu);
                            if(show_menu(&platform)) goto cleanup;
                            slicks_diag_help_failed(); continue;
                        }
                        if(input_menu->help->navigation.done) {
                            if(slicks_amiga_help_close(input_menu)) goto cleanup;
                            present_menu_surface(&platform,input_menu);
                            if(input_menu==g_slicks_title_help) {
                                slicks_amiga_player_menu_destroy(g_slicks_title_help); g_slicks_title_help=0;
                            }
                            if(show_menu(&platform)) goto cleanup;
                            slicks_diag_help_closed();
                        } else {
                            present_menu_surface(&platform,input_menu); slicks_diag_help_ready();
                        }
                    }
                    continue;
                }
                if(g_slicks_track_menu) {
                    if(g_slicks_track_menu->track_info) {
                        if(!character && !amiga_raw_to_dos_scan(code)) continue;
                        slicks_amiga_track_info_close(g_slicks_track_menu);
                        present_menu_surface(&platform,g_slicks_track_menu);
                        slicks_diag_track_info_closed();
                        continue;
                    }
                    if(g_slicks_track_menu->track_lists) {
                        if(track_lists_key(&platform,(short)track_count,track_names,character,
                            (unsigned char)amiga_raw_to_menu_scan(code),picker_clock.ticks)) goto cleanup;
                        present_menu_surface(&platform,g_slicks_track_menu);
                        if(show_menu(&platform)) goto cleanup;
                        slicks_diag_track_lists_ready();
                        continue;
                    }
                    if(g_slicks_track_menu->message) {
                        if(slicks_amiga_message_close(g_slicks_track_menu)) goto cleanup;
                        present_menu_surface(&platform,g_slicks_track_menu);
                        slicks_diag_track_info_warning_closed();
                        continue;
                    }
                    struct SlicksTrackMenu *state=&g_slicks_track_state;
                    g_slicks_track_action=(unsigned short)slicks_track_menu_key(state,(short)track_count,
                        (unsigned char)amiga_raw_to_menu_scan(code));
                    switch(g_slicks_track_action) {
                    case SLICKS_TRACK_MENU_TOGGLE:
                        if(state->cursor>=0 && slicks_track_playlist_toggle(&g_slicks_track_playlist,state->cursor,
                            track_names[state->cursor][0]!=0)) goto cleanup;
                        break;
                    case SLICKS_TRACK_MENU_ALL:
                        if(slicks_track_playlist_all(&g_slicks_track_playlist,track_count)) goto cleanup;
                        break;
                    case SLICKS_TRACK_MENU_CLEAR:
                        if(slicks_track_playlist_all(&g_slicks_track_playlist,0)) goto cleanup;
                        break;
                    case SLICKS_TRACK_MENU_SHUFFLE:
                        if(slicks_track_playlist_shuffle(&g_slicks_track_playlist,&g_slicks_setup_session.random_state)) goto cleanup;
                        break;
                    case SLICKS_TRACK_MENU_RANDOM:
                        if(state->random_count<0 || slicks_track_playlist_random(&g_slicks_track_playlist,track_count,
                            (unsigned short)state->random_count,&g_slicks_setup_session.random_state)) goto cleanup;
                        break;
                    default: break;
                    }
                    if(configuration.field_0626!=state->random_count) {
                        configuration.field_0626=state->random_count; setup_dirty=1;
                    }
                    if(state->done) {
                        slicks_amiga_player_menu_destroy(g_slicks_track_menu); g_slicks_track_menu=0;
                        g_slicks_track_renderer.surface=0;
                        if(g_slicks_track_playlist.count) selected_track=(unsigned short)track_selection[0];
                        make_title_surface(logical,title_frame,source_palette);
                        redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                            selected_vehicle,track_names[selected_track],selected_laps);
                        if(show_menu(&platform)) goto cleanup;
                        slicks_diag_tracks_closed();
                    } else if(g_slicks_track_action==SLICKS_TRACK_MENU_HELP) {
                        if(open_help(&platform,g_slicks_track_menu,slicks_original_track_help)) goto cleanup;
                    } else if(g_slicks_track_action==SLICKS_TRACK_MENU_LISTS) {
                        g_slicks_track_lists_load=slicks_amiga_track_lists_open(g_slicks_track_menu,
                            &track_list_cache,slicks_original_track_list_actions,slicks_original_players_footer_percent);
                        if(g_slicks_track_lists_load.result!=SLICKS_SETUP_LOADED) {
                            const char *error=g_slicks_track_lists_load.result==SLICKS_SETUP_LOAD_RECOVERY?
                                "KEEP SLICKS.TRK NEW/BAK FILES":"SLICKS.TRK LOAD FAILED";
                            if(track_lists_finish(&platform,(short)track_count,error)) goto cleanup;
                        } else if(slicks_amiga_profile_picker_draw(g_slicks_track_menu,picker_clock.ticks)) goto cleanup;
                        present_menu_surface(&platform,g_slicks_track_menu);
                        if(show_menu(&platform)) goto cleanup;
                        slicks_diag_track_lists_ready();
                    } else if(g_slicks_track_action==SLICKS_TRACK_MENU_RECORDS) {
                        slicks_amiga_platform_end(&platform);
                        char path[SLICKS_TRACK_NAME_SIZE+8];
                        if(state->cursor<0 || state->cursor>=track_count) goto cleanup;
                        make_track_path(path,track_names[state->cursor]);
                        if(open_track_info(g_slicks_track_menu,path,native_track_name(track_names,(unsigned)state->cursor)) &&
                            slicks_amiga_message_open(g_slicks_track_menu,(const unsigned char *)"TRACK RECORDS LOAD FAILED",
                                slicks_original_players_footer_percent)) goto cleanup;
                        present_menu_surface(&platform,g_slicks_track_menu);
                        if(slicks_amiga_platform_begin(&platform,0)) goto cleanup;
                        slicks_diag_track_info_ready();
                    } else {
                        if(draw_track_menu(&platform,(short)track_count)) goto cleanup;
                        slicks_diag_tracks_ready();
                    }
                    continue;
                }
                if(g_slicks_options_menu) {
                    if(g_slicks_options_menu->message) {
                        unsigned char scan=(unsigned char)amiga_raw_to_dos_scan(code);
                        if(!scan || raw>=0x60) continue;
                        slicks_amiga_platform_end(&platform);
                        if(slicks_amiga_message_close(g_slicks_options_menu)) goto cleanup;
                        if(g_slicks_track_clear_phase==1 && scan==0x15) {
                            static char path[SLICKS_TRACK_NAME_SIZE+8];
                            g_slicks_track_clear_report=(struct SlicksSetupStorageReport){SLICKS_SETUP_SAVED,0,0};
                            g_slicks_track_clear_changed=0;
                            for(unsigned i=0;i<track_count;++i) {
                                make_track_path(path,track_names[i]); unsigned char changed=0;
                                g_slicks_track_clear_report=slicks_amiga_clear_track_records(path,&changed);
                                g_slicks_track_clear_changed+=changed;
                                if(g_slicks_track_clear_report.result!=SLICKS_SETUP_SAVED) break;
                            }
                            const unsigned char *message=slicks_original_clear_complete;
                            g_slicks_track_clear_phase=2;
                            if(g_slicks_track_clear_report.result!=SLICKS_SETUP_SAVED) {
                                g_slicks_track_clear_phase=3;
                                message=(const unsigned char *)(g_slicks_track_clear_report.result==SLICKS_SETUP_RECOVERY_REQUIRED?
                                    "PARTIAL CLEAR - KEEP NEW/BAK FILES":g_slicks_track_clear_report.result==SLICKS_SETUP_SAVED_CLEANUP_PENDING?
                                    "PARTIAL CLEAR - BACKUP REMAINS":"CLEAR FAILED - MAY BE PARTIAL");
                            }
                            if(present_track_clear_message(&platform,message)) goto cleanup;
                        } else if(g_slicks_track_clear_phase==3 && g_slicks_track_clear_report.path) {
                            g_slicks_track_clear_phase=4;
                            if(present_track_clear_message(&platform,(const unsigned char *)g_slicks_track_clear_report.path)) goto cleanup;
                        } else {
                            unsigned char cancelled=(unsigned char)(g_slicks_track_clear_phase==1);
                            g_slicks_track_clear_phase=0;
                            present_menu_surface(&platform,g_slicks_options_menu);
                            if(slicks_amiga_platform_begin(&platform,0)) goto cleanup;
                            if(cancelled) slicks_diag_track_clear_cancelled();
                        }
                        continue;
                    }
                    if(g_slicks_options_menu->controllers_dialog) {
                        struct SlicksControllersDialog *state=&g_slicks_options_menu->controllers_dialog->state;
                        unsigned char scan=(unsigned char)amiga_raw_to_dos_scan(code);
                        if(state->capturing) {
                            if(slicks_controllers_capture(state,&configuration,slicks_original_controller_scans,scan)) goto cleanup;
                        } else if(slicks_controllers_key(state,&configuration,slicks_original_configuration.keys,scan)) goto cleanup;
                        if(state->done) {
                            if(slicks_amiga_controllers_close(g_slicks_options_menu)) goto cleanup;
                            present_menu_surface(&platform,g_slicks_options_menu);
                            if(show_menu(&platform)) goto cleanup;
                            slicks_diag_controllers_closed();
                        } else if(state->capturing) {
                            if(slicks_amiga_controllers_capture_prompt(g_slicks_options_menu,
                                slicks_original_controller_prompt)) goto cleanup;
                            present_menu_surface(&platform,g_slicks_options_menu);
                            slicks_diag_controllers_ready();
                        } else if(draw_controllers_dialog(&platform,&configuration)) goto cleanup;
                        continue;
                    }
                    g_slicks_options_action=(unsigned short)slicks_options_menu_key(&g_slicks_options_state,
                        &configuration,slicks_original_option_specs,(unsigned char)amiga_raw_to_menu_scan(code));
                    setup_dirty|=g_slicks_options_state.dirty;
                    slicks_amiga_audio_set_volume(&audio,configuration.options[1],configuration.options[2]);
                    if(g_slicks_options_state.done) {
                        slicks_amiga_player_menu_destroy(g_slicks_options_menu); g_slicks_options_menu=0;
                        g_slicks_options_renderer.surface=0;
                        selected_laps=(unsigned short)configuration.options[3];
                        slicks_setup_select(&g_slicks_setup_session,&configuration,&setup_resources,0);
                        if(g_slicks_setup_session.players.vehicle[0]>=0)
                            selected_vehicle=(unsigned short)g_slicks_setup_session.players.vehicle[0];
                        make_title_surface(logical,title_frame,source_palette);
                        redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                            selected_vehicle,track_names[selected_track],selected_laps);
                        if(show_menu(&platform)) goto cleanup;
                        slicks_diag_options_closed();
                    } else if(g_slicks_options_action==SLICKS_OPTIONS_CONTROLLERS) {
                        if(open_controllers_dialog(&platform,&configuration)) goto cleanup;
                    } else if(g_slicks_options_action==SLICKS_OPTIONS_HELP) {
                        if(open_help(&platform,g_slicks_options_menu,(const unsigned char *)"options")) goto cleanup;
                    } else if(g_slicks_options_action==SLICKS_OPTIONS_CLEAR_RECORDS) {
                        slicks_amiga_platform_end(&platform);
                        g_slicks_track_clear_phase=1;
                        if(present_track_clear_message(&platform,slicks_original_clear_question)) goto cleanup;
                    } else {
                        if(draw_options_menu(&platform,&configuration)) goto cleanup;
                        slicks_diag_options_ready();
                    }
                    continue;
                }
                if(g_slicks_player_menu) {
                    enum SlicksPlayerMenuAction pending=SLICKS_PLAYER_MENU_NONE;
                    if(g_slicks_player_menu->colour_dialog) {
                        struct SlicksColourPicker *state=&g_slicks_player_menu->colour_dialog->renderer.state;
                        slicks_colour_picker_key(state,(unsigned char)amiga_raw_to_dos_scan(code));
                        if(!state->result) {
                            if(slicks_amiga_colour_dialog_draw(g_slicks_player_menu,picker_clock.ticks)) goto cleanup;
                            present_player_menu(&platform); slicks_diag_colour_dialog_ready(); continue;
                        }
                        if(slicks_amiga_colour_dialog_close(g_slicks_player_menu)<0 || present_profile_editor(&platform)) goto cleanup;
                        continue;
                    } else if(g_slicks_player_menu->name_dialog) {
                        int done=slicks_name_dialog_key(&g_slicks_player_menu->name_dialog->renderer,character);
                        if(done<0 || g_slicks_player_menu->error) goto cleanup;
                        g_slicks_player_menu->name_dialog->hidden=0;
                        if(!done) {
                            present_player_menu(&platform); slicks_diag_name_dialog_ready(); continue;
                        }
                        if(slicks_amiga_name_dialog_close(g_slicks_player_menu) || present_profile_editor(&platform)) goto cleanup;
                        continue;
                    } else if(g_slicks_player_menu->editor_active) {
                        g_slicks_player_menu->editor_pending=(unsigned char)slicks_profile_editor_key(
                            &g_slicks_player_menu->editor,&g_slicks_profiles,(unsigned)g_slicks_player_menu->editor_index,
                            SLICKS_VEHICLE_COUNT,(unsigned char)amiga_raw_to_dos_scan(code));
                        if(g_slicks_player_menu->editor_pending==SLICKS_PROFILE_EDIT_NAME) {
                            if(profile_dialog_failure_test && argv[7]=='N' && !profile_dialog_failure_sent) {
                                extern unsigned char g_slicks_diag_profile_dialog_fault;
                                g_slicks_diag_profile_dialog_fault=profile_dialog_fault; profile_dialog_failure_sent=1;
                            }
                            if(slicks_amiga_name_dialog_open(g_slicks_player_menu,slicks_original_editor_name_caption,
                                slicks_original_players_footer_percent)) {
                                if(profile_dialog_warning(&platform,profile_dialog_failure_test)) goto cleanup;
                                continue;
                            }
                            present_player_menu(&platform);
                            if(show_menu(&platform)) goto cleanup;
                            slicks_diag_name_dialog_ready(); continue;
                        }
                        if(g_slicks_player_menu->editor_pending==SLICKS_PROFILE_EDIT_COLOUR_FIRST ||
                           g_slicks_player_menu->editor_pending==SLICKS_PROFILE_EDIT_COLOUR_SECOND) {
                            if(profile_dialog_failure_test && argv[7]=='C' && !profile_dialog_failure_sent) {
                                extern unsigned char g_slicks_diag_profile_dialog_fault;
                                g_slicks_diag_profile_dialog_fault=profile_dialog_fault; profile_dialog_failure_sent=1;
                            }
                            if(slicks_amiga_colour_dialog_open(g_slicks_player_menu,&g_slicks_profiles,
                                slicks_original_editor_colour_caption)) {
                                if(profile_dialog_warning(&platform,profile_dialog_failure_test)) goto cleanup;
                                continue;
                            }
                            picker_clock=(struct SlicksStatusClock){0}; picker_clock_vblank=platform.vblank_count;
                            if(slicks_amiga_colour_dialog_draw(g_slicks_player_menu,0)) goto cleanup;
                            present_player_menu(&platform);
                            if(show_menu(&platform)) goto cleanup;
                            slicks_diag_colour_dialog_ready(); continue;
                        }
                        if(!g_slicks_player_menu->editor.result) {
                            if(present_profile_editor(&platform)) goto cleanup;
                            continue;
                        }
                        if(slicks_amiga_profile_editor_close(g_slicks_player_menu,&g_slicks_profiles)<0) goto cleanup;
                        player_menu_state.redraw=1;
                    } else if(g_slicks_player_menu->delete_pending) {
                        if(slicks_profile_delete_confirmed(amiga_raw_to_dos_scan(code)) &&
                           slicks_delete_player_profile(&g_slicks_profiles,configuration.selected_profile,
                               (unsigned)g_slicks_player_menu->delete_index)) goto cleanup;
                        slicks_amiga_player_menu_restore(g_slicks_player_menu);
                        player_menu_state.redraw=1;
                    } else if(g_slicks_player_menu->picker) {
                        slicks_list_dialog_key(&g_slicks_player_menu->picker->renderer.state,
                            (unsigned char)amiga_raw_to_menu_scan(code));
                        if(!g_slicks_player_menu->picker->renderer.state.done) {
                            if(slicks_amiga_profile_picker_draw(g_slicks_player_menu,picker_clock.ticks)) goto cleanup;
                            present_player_menu(&platform); slicks_diag_profile_picker_ready();
                            continue;
                        }
                        short selected=slicks_amiga_profile_picker_close(g_slicks_player_menu);
                        if(player_menu_state.row<4) {
                            if(selected<0 || selected>=SLICKS_PROFILE_MAX) goto cleanup;
                            slicks_menu_assign_profile(configuration.selected_profile,g_slicks_profiles.setup,
                                player_menu_state.row,selected);
                        } else {
                            short index=0;
                            unsigned char flags=selected>=0 && selected<SLICKS_PROFILE_MAX?
                                g_slicks_profiles.setup[selected].flags:0;
                            enum SlicksProfileListAction action=slicks_profile_list_action(selected,flags,&index);
                            if(action!=SLICKS_PROFILE_LIST_NONE && (index<0 || index>=g_slicks_profiles.count)) goto cleanup;
                            if(action==SLICKS_PROFILE_LIST_CONFIRM_DELETE) {
                                if(slicks_amiga_profile_delete_prompt(g_slicks_player_menu,index,&g_slicks_profiles,
                                    slicks_original_players_delete_question,slicks_original_players_footer_percent)) goto cleanup;
                                present_player_menu(&platform);
                                if(show_menu(&platform)) goto cleanup;
                                g_slicks_diag_player_menu_action=SLICKS_PLAYER_MENU_DELETE;
                                slicks_diag_player_menu_ready(); continue;
                            }
                            if(action==SLICKS_PROFILE_LIST_EDIT) {
                                if(slicks_amiga_profile_editor_open(g_slicks_player_menu,&g_slicks_profiles,index,0,
                                    &g_slicks_setup_session.random_state) || present_profile_editor(&platform)) goto cleanup;
                                continue;
                            }
                            slicks_amiga_player_menu_restore(g_slicks_player_menu);
                        }
                        player_menu_state.redraw=1;
                    } else {
                        short profile=player_menu_state.row<4?
                            configuration.selected_profile[player_menu_state.row]:-1;
                        unsigned char old_vehicle=profile>=0 && profile<g_slicks_profiles.count?
                            g_slicks_profiles.setup[profile].vehicle:0;
                        pending=slicks_player_menu_key(&player_menu_state,
                            configuration.selected_profile,g_slicks_profiles.setup,g_slicks_profiles.count,
                            SLICKS_VEHICLE_COUNT,(unsigned char)amiga_raw_to_dos_scan(code));
                        /* Keep the original key helper's dirty-byte semantics;
                         * our save-on-exit adapter must also persist shortcut
                         * vehicle edits made without opening an editor/picker. */
                        if(profile>=0 && profile<g_slicks_profiles.count &&
                           g_slicks_profiles.setup[profile].vehicle!=old_vehicle)
                            setup_dirty=1;
                    }
                    if(pending==SLICKS_PLAYER_MENU_HELP) {
                        if(open_help(&platform,g_slicks_player_menu,(const unsigned char *)"plr_menu")) goto cleanup;
                        continue;
                    }
                    if(pending==SLICKS_PLAYER_MENU_ADD) {
                        if(slicks_amiga_profile_editor_open(g_slicks_player_menu,&g_slicks_profiles,g_slicks_profiles.count,1,
                            &g_slicks_setup_session.random_state) || present_profile_editor(&platform)) goto cleanup;
                        continue;
                    }
                    if((pending==SLICKS_PLAYER_MENU_PICK || pending==SLICKS_PLAYER_MENU_DELETE ||
                        pending==SLICKS_PLAYER_MENU_EDIT) && !player_menu_state.redraw) {
                        if(slicks_amiga_profile_picker_open(g_slicks_player_menu,player_menu_state.row,
                            player_menu_state.row<4?configuration.selected_profile[player_menu_state.row]:0,&g_slicks_profiles,
                            player_menu_state.row<4?slicks_original_players_select:slicks_original_players_actions,
                            slicks_original_players_footer_percent)) goto cleanup;
                        picker_clock=(struct SlicksStatusClock){0}; picker_clock_vblank=platform.vblank_count;
                        if(slicks_amiga_profile_picker_draw(g_slicks_player_menu,0)) goto cleanup;
                        present_player_menu(&platform);
                        if(show_menu(&platform)) goto cleanup;
                        g_slicks_diag_player_menu_action=0;
                        slicks_diag_profile_picker_ready(); continue;
                    }
                    /* Other modal actions remain pending, never treated as completed edits. */
                    g_slicks_diag_player_menu_action=(unsigned short)pending;
                    if(player_menu_state.done) {
                        setup_dirty|=player_menu_state.dirty;
                        if(g_slicks_setup_session.players.vehicle[0]>=0 &&
                           g_slicks_setup_session.players.vehicle[0]<SLICKS_VEHICLE_COUNT)
                            selected_vehicle=(unsigned short)g_slicks_setup_session.players.vehicle[0];
                        slicks_amiga_player_menu_destroy(g_slicks_player_menu); g_slicks_player_menu=0;
                        if(slicks_amiga_platform_set_view(&platform,0,source_palette)) goto cleanup;
                        make_title_surface(logical,title_frame,source_palette);
                        redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                            selected_vehicle,track_names[selected_track],selected_laps);
                        if(show_menu(&platform)) goto cleanup;
                        slicks_diag_player_menu_closed();
                    } else if(player_menu_state.redraw) {
                        setup_resources.profile_count=g_slicks_profiles.count;
                        slicks_setup_select(&g_slicks_setup_session,&configuration,&setup_resources,0);
                        if(slicks_amiga_player_menu_draw(g_slicks_player_menu,player_menu_state.row,
                            configuration.selected_profile,g_slicks_setup_session.players.participation,&g_slicks_profiles)) goto cleanup;
                        present_player_menu(&platform); player_menu_state.redraw=0;
                        if(!platform.active && slicks_amiga_platform_begin(&platform,0)) goto cleanup;
                        g_slicks_diag_player_menu_row=player_menu_state.row;
                        slicks_diag_player_menu_ready();
                    }
                    continue;
                }
                if (service_menu_open) {
                    unsigned short change = slicks_service_menu_key(
                        &service_selection, &configuration.options[9], &configuration.options[10],
                        amiga_raw_to_menu_scan(code));
                    if (change == 2) {
                        service_menu_open = 0;
                        make_title_surface(logical, title_frame, source_palette);
                        redraw_title_configuration(&platform, logical, chunky,
                            source_palette, menu_selection, selected_vehicle,
                            track_names[selected_track], selected_laps);
                    } else if (change) {
                        setup_dirty=1;
                        redraw_service_options(&platform, logical, chunky,
                            service_selection, configuration.options[9], configuration.options[10]);
                    }
                    continue;
                }
                if(original_setup && (raw==0x4c || raw==0x4d || raw==0x4f || raw==0x4e)) {
                    short selection=(short)menu_selection;
                    short count=(short)g_slicks_track_playlist.count;
                    short old_mode=configuration.options[0];
                    unsigned char refresh=0;
                    redraw=slicks_title_mode_navigation(&selection,&count,(short)track_count,
                        &configuration.options[0],&setup_resources.override_count,&refresh,amiga_raw_to_dos_scan(raw));
                    if(refresh) title_arcade_refresh=refresh;
                    menu_selection=(unsigned short)selection;
                    g_slicks_track_playlist.count=(unsigned short)count;
                    if(configuration.options[0]!=old_mode) setup_dirty=1;
                } else if (raw == 0x4c) {
                    menu_selection = (unsigned short)(
                        menu_selection ? menu_selection - 1 : 6);
                    redraw = 1;
                } else if (raw == 0x4d) {
                    menu_selection = (unsigned short)(
                        menu_selection < 6 ? menu_selection + 1 : 0);
                    redraw = 1;
                } else if (raw == 0x4f || raw == 0x4e) {
                    int delta = raw == 0x4e ? 1 : -1;
                    if (!original_setup && menu_selection == 1) {
                        selected_vehicle = (unsigned short)(
                            (selected_vehicle + SLICKS_VEHICLE_COUNT + delta) %
                            SLICKS_VEHICLE_COUNT);
                        redraw = 1;
                    } else if (!original_setup && menu_selection == 2) {
                        selected_track = (unsigned short)(
                            (selected_track + track_count + delta) %
                            track_count);
                        redraw = 1;
                    } else if (!original_setup && menu_selection == 3) {
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
                        menu_selection, selected_vehicle,
                        track_names[selected_track], selected_laps);
                    continue;
                }
                unsigned short action = slicks_dispatch_title_key(
                    title_scan);
                unsigned short action_selection=(unsigned short)slicks_title_action_selection(configuration.options[0],(short)menu_selection);
                const unsigned char *help_topic=slicks_title_help_topic(action,action_selection);
                if(help_topic) {
                    if(open_title_help(&platform,logical,chunky,source_palette,help_topic)) goto cleanup;
                    continue;
                }
                if(action==2 && action_selection==1 && original_setup) {
                    if(open_player_menu(&platform,chunky,&configuration,&player_menu_state)) goto cleanup;
                    continue;
                }
                if(action==2 && menu_selection==2 && original_setup) {
                    if(open_track_menu(&platform,chunky,track_names,(short)track_count,configuration.field_0626)) goto cleanup;
                    continue;
                }
                if (action == 2 && action_selection == 3) {
                    if(original_setup) {
                        if(open_options_menu(&platform,chunky,source_palette,&configuration)) goto cleanup;
                        continue;
                    }
                    if(configuration.options[0]!=4) setup_dirty=1;
                    configuration.options[0]=4; /* Enter the custom options. */
                    service_menu_open = 1;
                    redraw_service_options(&platform, logical, chunky,
                        service_selection, configuration.options[9], configuration.options[10]);
                    continue;
                }
                if(action==2 && menu_selection==4 && original_setup) {
                    static unsigned char saved_tracks[256][8];
                    static struct SlicksSavedGame game;
                    static struct SlicksSavedGameResolved resolved;
                    static struct SlicksSetupSession staged,previous;
                    struct SlicksConfiguration next_config=configuration;
                    struct SlicksResourceArchive archive={0};
                    if(slicks_resource_archive_cached(&archive,menu_cache)) goto cleanup;
                    struct SlicksAmigaPlayerMenu *m=slicks_amiga_help_surface_create(&archive,chunky,source_palette);
                    slicks_resource_archive_close(&archive);
                    if(!m) goto cleanup;
                    int loaded=run_saved_game_dialog(&platform,m,&game,saved_tracks,0);
                    if(loaded==1) {
                        enum SlicksSavedGameResolveResult status=slicks_championship_stage(&staged,&resolved,
                            &game,&g_slicks_setup_session,&configuration,&g_slicks_profiles,
                            slicks_original_fallback_colours,SLICKS_VEHICLE_COUNT,track_count,native_track_name,track_names);
                        if(status!=SLICKS_RESUME_READY) {
                            const char *message=status==SLICKS_RESUME_MISSING_TRACK?"SAVED TRACK IS MISSING":
                                status==SLICKS_RESUME_MISSING_PROFILE?"SAVED PLAYER PROFILE IS MISSING":"INVALID NEXT TRACK OR PLAYER SELECTION";
                            loaded=championship_notice(&platform,m,(const unsigned char *)message)<0?-1:0;
                        }
                    }
                    slicks_amiga_player_menu_destroy(m);
                    if(loaded<0) goto cleanup;
                    if(!loaded) {
                        redraw_title_configuration(&platform,logical,chunky,source_palette,menu_selection,
                            selected_vehicle,track_names[selected_track],selected_laps);
                        if(show_menu(&platform)) goto cleanup;
                        continue;
                    }
                    for(unsigned i=0;i<4;++i) next_config.selected_profile[i]=staged.players.selected[i];
                    previous=g_slicks_setup_session;
                    g_slicks_setup_session=staged;
                    short previous_shop_position=shop_track_position;
                    shop_track_position=game.next_track;
                    shop_track_total=game.track_count;
                    make_track_path(selected_track_path,track_names[resolved.tracks[game.next_track]]);
                    int prepare_status=prepare_race(&platform,logical,chunky,mode_state,race,selected_track_path,race_palette,
                        selected_vehicle,&next_config,&g_slicks_setup_session,0);
                    shop_track_total=0;
                    if(prepare_status) {
                        shop_track_position=previous_shop_position;
                        g_slicks_setup_session=previous;
                        race_load_prompt=1; race_load_retry=0;
                        if(show_race_load_error(&platform,logical,chunky,mode_state,source_palette,0)) goto cleanup;
                        continue;
                    }
                    configuration=next_config; setup_dirty=1;
                    for(unsigned i=0;i<(unsigned)game.track_count;++i) track_selection[i]=resolved.tracks[i];
                    g_slicks_track_playlist.count=game.track_count;
                    playlist_position=(unsigned short)game.next_track;
                    selected_track=(unsigned short)track_selection[playlist_position];
                    slicks_race_set_laps(race,selected_laps);
                    if(slicks_amiga_platform_begin(&platform,1)) goto cleanup;
                    g_slicks_diag_ready=1;
                    enter_prepared_race(&platform,logical,race);
                    start_race_engines(&audio,race,&platform);
                    continue;
                }
                if (action == 1 || (action == 2 && menu_selection == 6)) {
                    exit_requested=1;
                    continue;
                }
                /* Original F9 (2a4c5) returns 99 regardless of the selected
                 * row, sharing the GO playlist/player preparation tail. */
                if (action == 4 || (action == 5 && original_setup) || (action == 2 && menu_selection == 0)) {
                    g_slicks_diag_ready = 0;
                    if(action==5) {
                        demo_track=selected_track;demo_vehicle=selected_vehicle;demo_laps=selected_laps;
                        if(slicks_title_demo_begin(&title_demo,&configuration,&g_slicks_track_playlist,
                            track_count,&g_slicks_setup_session.random_state,(short *)&menu_selection)) goto cleanup;
                        selected_laps=(unsigned short)configuration.options[3];
                    }
                    slicks_amiga_platform_end(&platform);
                    /* Original 2a593..2a5e4 supplies one random track when
                     * GO is selected with an empty playlist. */
                    if(original_setup) {
                        if(slicks_title_start_shuffle(&g_slicks_track_playlist,action,
                            g_slicks_track_state.random_order,&g_slicks_setup_session.random_state)) goto cleanup;
                        if(!g_slicks_track_playlist.count && slicks_track_playlist_random(
                            &g_slicks_track_playlist,track_count,1,&g_slicks_setup_session.random_state)) goto cleanup;
                        selected_track=(unsigned short)track_selection[0];
                    }
                    make_track_path(selected_track_path,
                                    track_names[selected_track]);
                    track_path = selected_track_path;
                    /* Existing real loader failure boundaries; only the first
                     * diagnostic attempt fails. The next uses real resources. */
                    if((demo_lifecycle_test==6 || demo_lifecycle_test==7 || demo_lifecycle_test==9 || demo_lifecycle_test==12) && !demo_test_round)
                        g_slicks_diag_race_load_fault=demo_lifecycle_test==12?8:demo_lifecycle_test==9?1:demo_lifecycle_test==6?2:6;
                    if (prepare_race(&platform, logical, chunky, mode_state,
                                     race, track_path, race_palette, selected_vehicle,
                                     &configuration,original_setup?&g_slicks_setup_session:0,1) != 0) {
                        if(title_demo.active) {
                            restore_demo_configuration(&configuration,race,0);
                            selected_track=demo_track;selected_vehicle=demo_vehicle;selected_laps=demo_laps;
                        }
                        /* This is a native platform error, not invented DOS
                         * gameplay. Keep setup alive so files/controllers can
                         * be corrected and GO retried from the actual menus. */
                        race_load_prompt=1; race_load_retry=0;
                        if(show_race_load_error(&platform,logical,chunky,mode_state,source_palette,0)) goto cleanup;
                        if(setup_failure_test) {
                            static const unsigned char keys[]={0x45,0x4d,0x44,0x45,0x4c,0x44};
                            platform.key_tail=0;
                            for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
                            platform.key_head=sizeof keys;
                        }
                        continue;
                    }
                    playlist_position = 0;
                    slicks_race_set_laps(race, selected_laps);
                    if (slicks_amiga_platform_begin(&platform, 1) != 0)
                        goto cleanup;
                    g_slicks_diag_ready = 1;
                    enter_prepared_race(&platform, logical, race);
                    start_race_engines(&audio,race,&platform);
                    g_slicks_diag_engine_sample_block =
                        audio.engine_sample_block;
                }
            }
        }
        if(player_menu_test && argc==8 && (argv[7]=='N' || argv[7]=='C' || persistence_test) && g_slicks_player_menu &&
           platform.key_head==platform.key_tail && !g_slicks_player_menu->name_dialog) {
            if(!name_test_stage && g_slicks_player_menu->editor_active && !g_slicks_player_menu->editor_new) {
                static const unsigned char keys[]={0x44,0x41,0x32,0x44,0x51};
                static const unsigned char colour_keys[]={0x4d,0x4d,0x4d,0x44,0x4e,0x4d,0x4f,0x44,
                    0x4d,0x44,0x4e,0x45,0x51};
                unsigned count=argv[7]!='N'?sizeof colour_keys:sizeof keys;
                platform.key_tail=0;
                for(unsigned i=0;i<count;++i) platform.keys[i]=argv[7]!='N'?colour_keys[i]:keys[i];
                platform.key_head=(unsigned char)count; name_test_stage=1;
            } else if(name_test_stage==1 && !g_slicks_player_menu->editor_active) {
                static const unsigned char keys[]={0x44,0x1d,0x44,0x44,0x41,0x45,0x51};
                static const unsigned char colour_keys[]={0x44,0x1d,0x44,0x4d,0x4d,0x4d,0x4d,
                    0x44,0x4e,0x4d,0x4e,0x44,0x51};
                static const unsigned char save_keys[]={0x4c,0x4c,0x4c,0x4c,0x4c,
                    0x44,0x1d,0x44,0x45,0x45};
                unsigned count=persistence_test?sizeof save_keys:argv[7]=='C'?sizeof colour_keys:sizeof keys;
                if(combined_test || mixed_setup_test || unique_setup_test) --count; /* Return to title, don't exit yet. */
                platform.key_tail=0;
                for(unsigned i=0;i<count;++i) platform.keys[i]=persistence_test?save_keys[i]:argv[7]=='C'?colour_keys[i]:keys[i];
                platform.key_head=(unsigned char)count; name_test_stage=2;
            }
        }
        if(unique_setup_test && name_test_stage==2 && unique_stage<6 && platform.key_head==platform.key_tail) {
            /* Move ABC through every driver slot using the actual picker.
             * Include a None destination so the displaced slot is tested. */
            static const unsigned char steps[6][7]={
                {0x44,0x4d,0x4f,0x44,0x1d,0x44},
                {0x4d,0x44,0x1d,0x44},
                {0x4d,0x44,0x1d,0x44},
                {0x4c,0x4c,0x4c,0x44,0x1d,0x44},
                {0x4d,0x4d,0x44,0x1d,0x44},
                {0x45,0x45}};
            static const unsigned char counts[]={6,4,4,6,5,2};
            g_slicks_diag_unique_stage=unique_stage; slicks_diag_unique_selection();
            platform.key_tail=0;
            for(unsigned i=0;i<counts[unique_stage];++i) platform.keys[i]=steps[unique_stage][i];
            platform.key_head=counts[unique_stage++];
        }
        if(mixed_setup_test && name_test_stage==2 && !g_slicks_player_menu && platform.key_head==platform.key_tail) {
            /* Keep ABC human and the shared Computer in slots 1/3; choose
             * None for slot 2 through the ordinary Players menu. */
            static const unsigned char keys[]={0x44,0x4d,0x4d,0x4f,0x45,0x45};
            platform.key_tail=0;
            for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
            platform.key_head=sizeof keys; name_test_stage=3;
        }
        if(combined_test && name_test_stage==2 && !g_slicks_player_menu && combined_stage<3 &&
           platform.key_head==platform.key_tail) {
            static const unsigned char steps[3][16]={
                {0x4d,0x4d,0x44,0x4e,0x4e,0x4e,0x4e,0x4d,0x4d,0x4d,0x4e},
                {0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x44,0x4e,0x44,0x11},
                {0x45,0x45,0x45}};
            static const unsigned char counts[]={11,14,3};
            unsigned count=counts[combined_stage]; platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=steps[combined_stage][i];
            platform.key_head=(unsigned char)count; ++combined_stage;
        }
        if(track_lists_test && argv[6]=='D' && tracks_test_stage<4 && platform.key_head==platform.key_tail) {
            static const unsigned char steps[4][12]={
                {0x4e,0x44,0x42,0x42,0x44,0x36}, /* Delete, N */
                {0x44,0x42,0x44,0x20,0x45},      /* Save, A, Escape */
                {0x44,0x42,0x42,0x44,0x15},      /* Delete, Y */
                {0x44,0x45,0x45,0x4c,0x4c,0x44}}; /* empty picker, close, race */
            static const unsigned char counts[]={6,5,5,6};
            platform.key_tail=0;
            for(unsigned i=0;i<counts[tracks_test_stage];++i) platform.keys[i]=steps[tracks_test_stage][i];
            platform.key_head=counts[tracks_test_stage++];
        }
        if(track_lists_test && argv[6]=='M' && tracks_test_stage<3 && platform.key_head==platform.key_tail) {
            static const unsigned char steps[3][4]={
                {0x4e,0x44,0x1d,0x45}, /* Lists, End, cancel */
                {0x44,0x1d,0x44},      /* reopen, End, load last */
                {0x45,0x4c,0x4c,0x44}}; /* close Tracks, GO */
            static const unsigned char counts[]={4,3,4};
            platform.key_tail=0;
            for(unsigned i=0;i<counts[tracks_test_stage];++i) platform.keys[i]=steps[tracks_test_stage][i];
            platform.key_head=counts[tracks_test_stage++];
        }
        if(track_lists_test && argv[6]=='N' && tracks_test_stage<4 && platform.key_head==platform.key_tail) {
            static const unsigned char steps[4][4]={
                {0x4e,0x44,0x45}, /* Lists allocation warning, dismiss */
                {0x44,0x45},      /* retry names allocation warning, dismiss */
                {0x44,0x1d,0x44}, /* retry, last list, load */
                {0x45,0x4c,0x4c,0x44}};
            static const unsigned char counts[]={3,2,3,4};
            g_slicks_diag_list_alloc_fault=tracks_test_stage<2?(unsigned char)(tracks_test_stage+1):0;
            platform.key_tail=0;
            for(unsigned i=0;i<counts[tracks_test_stage];++i) platform.keys[i]=steps[tracks_test_stage][i];
            platform.key_head=counts[tracks_test_stage++];
        }
        if(track_lists_test && argv[6]!='D' && argv[6]!='M' && argv[6]!='N' && tracks_test_stage<2 && platform.key_head==platform.key_tail) {
            static const unsigned char save_keys[]={0x4d,0x44,0x4e,0x44,0x42,0x44,0x20,0x44};
            static const unsigned char load_keys[]={0x4e,0x44,0x44};
            static const unsigned char race_keys[]={0x45,0x4c,0x4c,0x44};
            static const unsigned char failure_return_keys[]={0x44,0x45,0x4c,0x4c,0x44};
            unsigned char save=(unsigned char)(argv[6]=='L' || argv[6]=='F');
            const unsigned char *keys=tracks_test_stage?(argv[6]=='F'?failure_return_keys:race_keys):(save?save_keys:load_keys);
            unsigned count=tracks_test_stage?(argv[6]=='F'?sizeof failure_return_keys:sizeof race_keys):(save?sizeof save_keys:sizeof load_keys);
            platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=keys[i];
            platform.key_head=(unsigned char)count; ++tracks_test_stage;
        }
        if(track_info_fault_test && track_info_fault_stage<=5 && g_slicks_track_menu &&
           g_slicks_track_menu->message && !g_slicks_track_menu->track_info && platform.key_head==platform.key_tail) {
            ++track_info_fault_stage;
            g_slicks_diag_track_info_fault=track_info_fault_stage<=5?track_info_fault_stage:0;
            platform.key_tail=0; platform.keys[0]=0x45; platform.keys[1]=0x51; platform.key_head=2;
        }
        if(track_info_failure_test && track_info_failure_stage<2 && g_slicks_track_menu && platform.key_head==platform.key_tail) {
            if(!track_info_failure_stage) {
                unsigned target;
                for(target=0;target<track_count;++target) {
                    static const char wanted[]="RAILROAD"; unsigned j=0;
                    while(j<8 && track_names[target][j]==wanted[j]) ++j;
                    if(j==8 && track_names[target][8]=='.') break;
                }
                if(target==track_count) goto cleanup;
                platform.key_tail=0; platform.key_head=0;
                if(g_slicks_track_state.cursor==(short)target) {
                    platform.keys[platform.key_head++]=0x51; track_info_failure_stage=1;
                } else {
                    unsigned count=target-(unsigned)g_slicks_track_state.cursor;
                    if(count>15) count=15;
                    while(count--) platform.keys[platform.key_head++]=0x4d;
                }
            } else if(g_slicks_track_menu->message && !g_slicks_track_menu->track_info) {
                platform.key_tail=0; platform.keys[0]=0x45; platform.keys[1]=0x3d; platform.keys[2]=0x51;
                platform.key_head=3; track_info_failure_stage=2;
            }
        }
        if(track_info_test && tracks_test_stage<2 && g_slicks_track_menu && g_slicks_track_menu->track_info &&
            g_slicks_track_menu->track_info->updates>=256 && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.keys[0]=0x45;
            platform.keys[1]=tracks_test_stage?0x45:0x51;
            platform.key_head=2; ++tracks_test_stage;
        }
        if(tracks_test && !track_lists_test && ((!track_info_test && !tracks_test_stage) ||
            (track_info_test && tracks_test_stage==2)) && !g_slicks_track_menu && platform.key_head==platform.key_tail) {
            static const unsigned char keys[]={0x4c,0x4c,0x44};
            platform.key_tail=0;
            for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
            platform.key_head=sizeof keys; tracks_test_stage=track_info_test?3:1;
        }
        if(joystick_test && options_test_stage<2 && g_slicks_options_menu &&
           platform.key_head==platform.key_tail) {
            static const unsigned char steps[2][14]={
                {0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x44},
                {0x4d,0x4d,0x44,0x45,0x45,0x4c,0x4c,0x4c,0x44}};
            unsigned count=options_test_stage?9:14;
            platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=steps[options_test_stage][i];
            platform.key_head=(unsigned char)count; ++options_test_stage;
        }
        if(controllers_test && options_test_stage<4 && g_slicks_options_menu &&
           platform.key_head==platform.key_tail) {
            static const unsigned char steps[4][16]={
                {0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x44},
                {0x44,0x4e,0x44,0x11,0x44,0x52,0x45,0x44},
                {0x4d,0x4d,0x4d,0x4d,0x44,0x45,0x44},
                {0x4e,0x44,0x15,0x45}};
            static const unsigned char counts[]={14,8,7,4};
            unsigned count=counts[options_test_stage];
            platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=steps[options_test_stage][i];
            platform.key_head=(unsigned char)count; ++options_test_stage;
        }
        if(clear_test && clear_test_stage<2 && g_slicks_options_menu && platform.key_head==platform.key_tail) {
            static const unsigned char steps[2][15]={
                {0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x4d},
                {0x4d,0x4d,0x4d,0x4d,0x4d,0x4d,0x44,0x36,0x44,0x15,0x40,0x45,0x45}};
            /* Custom skips rows 13/14, so fourteen Down events reach row 16. */
            unsigned count=clear_test_stage?13:8; platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=steps[clear_test_stage][i];
            if(clear_test_stage && argv[7]=='S') {
                /* A failed clear has a second acknowledgement for its path. */
                platform.keys[count++]=0x45;
            }
            platform.key_head=count; ++clear_test_stage;
        }
        if(title_help_failure_test && title_help_failure_stage<10 && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.key_head=1;
            if(title_help_failure_stage==0) { help_fail_archive=1; platform.keys[0]=0x50; }
            else if(title_help_failure_stage==2) { help_fail_surface=1; platform.keys[0]=0x50; }
            else if(title_help_failure_stage==4) { g_slicks_diag_help_fail_allocation=1; platform.keys[0]=0x50; }
            else if(title_help_failure_stage==6) platform.keys[0]=0x50;
            else if(title_help_failure_stage==7) {
                if(!g_slicks_title_help || !g_slicks_title_help->help || !g_slicks_title_help->help->chapter_length) goto cleanup;
                /* Malformed chapter byte: real renderer validation must fail
                 * on the next link-selection redraw, not a forced result. */
                g_slicks_title_help->help->chapter[0]=0;
                platform.keys[0]=0x4d;
            } else if(title_help_failure_stage==9) {
                platform.keys[0]=0x50; platform.keys[1]=0x45; platform.keys[2]=0x45; platform.key_head=3;
            } else {
                if(!g_slicks_title_help_warning && (!g_slicks_title_help || !g_slicks_title_help->help_warning)) goto cleanup;
                platform.keys[0]=0x44;
            }
            ++title_help_failure_stage;
        }
        struct SlicksAmigaPlayerMenu *help_failure_menu=g_slicks_options_menu?g_slicks_options_menu:
            g_slicks_player_menu?g_slicks_player_menu:g_slicks_track_menu;
        if(help_failure_test && help_failure_stage<5 && help_failure_menu && platform.key_head==platform.key_tail) {
            platform.key_tail=0; platform.key_head=1;
            if(help_failure_stage==0) { slicks_diag_help_test_ready(); help_fail_archive=1; platform.keys[0]=0x50; }
            else if(help_failure_stage==2) { g_slicks_diag_help_fail_allocation=1; platform.keys[0]=0x50; }
            else if(help_failure_stage==4) {
                platform.keys[0]=0x50; platform.keys[1]=0x45;
                platform.keys[2]=0x45; platform.keys[3]=0x45; platform.key_head=4;
            } else {
                if(!help_failure_menu->help_warning) goto cleanup;
                platform.keys[0]=0x44;
            }
            ++help_failure_stage;
        }
        if((collisions_test || weapons_test) && options_test_stage==1 && g_slicks_options_menu && platform.key_head==platform.key_tail) {
            platform.key_head=platform.key_tail=0;
            if(g_slicks_options_state.row<(weapons_test?7:11)) platform.keys[platform.key_head++]=0x4d;
            else {
                static const unsigned char keys[]={0x3d,0x4e,0x45,0x4c,0x4c,0x4c,0x44};
                for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
                if(!weapons_test || argv[7]=='F') platform.keys[1]=0x3d;
                platform.key_head=sizeof keys; options_test_stage=2;
            }
        }
        if(volume_test && !options_test_stage && g_slicks_options_menu && platform.key_head==platform.key_tail) {
            static const unsigned char keys[]={0x4d,0x3d,0x4e,0x4e,0x45,0x4c,0x4c,0x4c,0x44};
            static const unsigned char save_keys[]={0x4d,0x3d,0x4e,0x4e,0x45,0x4d,0x4d,0x44};
            unsigned count=volume_save_test?sizeof save_keys:sizeof keys;
            platform.key_tail=0;
            for(unsigned i=0;i<count;++i) platform.keys[i]=volume_save_test?save_keys[i]:keys[i];
            platform.key_head=(unsigned char)count; options_test_stage=1;
        }
        if(options_test && !controllers_test && !joystick_test && !help_test && !clear_test && !volume_test && !options_test_stage && g_slicks_options_menu &&
           platform.key_head==platform.key_tail) {
            static const unsigned char keys[]={0x45,0x4c,0x4c,0x4c,0x44};
            static const unsigned char sequence_keys[]={0x4d,0x3d,0x4e,0x45,0x4c,0x44};
            static const unsigned char arcade_save_keys[]={0x4d,0x3d,0x4e,0x45,0x4d,0x4d,0x44};
            platform.key_tail=0;
            const unsigned char *next=arcade_save_test?arcade_save_keys:sequence_test?sequence_keys:keys;
            unsigned count=arcade_save_test?sizeof arcade_save_keys:sequence_test?sizeof sequence_keys:sizeof keys;
            if(mode_transition_test) { next=sequence_keys+3; count=3; }
            for(unsigned i=0;i<count;++i) platform.keys[i]=next[i];
            platform.key_head=(unsigned char)count; options_test_stage=1;
        }
        if(sequence_test && options_test_stage==1 && g_slicks_track_menu &&
            platform.key_head==platform.key_tail) {
            /* Initial selection is All: use the native Clear action first. */
            static const unsigned char keys[]={0x4e,0x4d,0x4d,0x4d,0x44,
                0x4f,0x3d};
            platform.key_tail=0;
            for(unsigned i=0;i<sizeof keys;++i) platform.keys[i]=keys[i];
            platform.key_head=sizeof keys; options_test_stage=2;
        }
        if(sequence_test && options_test_stage>=2 && options_test_stage<4 &&
            g_slicks_track_menu && platform.key_head==platform.key_tail) {
            const char *wanted=options_test_stage==2?"BASIC":"BASICTRK";
            unsigned target=0;
            for(;target<track_count;++target) {
                unsigned j=0;
                while(wanted[j] && track_names[target][j]==wanted[j]) ++j;
                if(!wanted[j] && track_names[target][j]=='.') break;
            }
            if(target==track_count) goto cleanup;
            platform.key_head=platform.key_tail=0;
            if((unsigned)g_slicks_track_state.cursor==target) {
                platform.keys[platform.key_head++]=0x40;
                if(options_test_stage==3) {
                    platform.keys[platform.key_head++]=0x45;
                    platform.keys[platform.key_head++]=0x4c;
                    platform.keys[platform.key_head++]=0x4c;
                    platform.keys[platform.key_head++]=0x44;
                }
                ++options_test_stage;
            } else {
                int delta=(int)target-g_slicks_track_state.cursor;
                unsigned count=delta<0?(unsigned)-delta:(unsigned)delta;
                if(count>15) count=15;
                while(count--) platform.keys[platform.key_head++]=delta<0?0x4c:0x4d;
            }
        }
        /* Original title input (36ce0) reads keyboard scans, not mouse
         * buttons. Do not introduce a second, incomplete activation path. */
        if(!save_prompt && g_slicks_track_menu && g_slicks_track_menu->track_info) {
            slicks_amiga_track_info_tick(g_slicks_track_menu,&g_slicks_setup_session.random_state);
            present_menu_surface(&platform,g_slicks_track_menu);
        }
        if(!g_slicks_diag_ingame && !save_prompt &&
           !race_load_prompt && !service_menu_open && !g_slicks_player_menu &&
           !g_slicks_options_menu && !g_slicks_track_menu && !g_slicks_title_help &&
           !g_slicks_diag_saved_menu) {
            if(configuration.options[0]!=5) {
                slicks_tick_title_colours(logical,source_palette);
                /* The shared original tail advances even without a name. */
                slicks_tick_title_registration(logical,registration.name,source_palette);
                redraw_title_configuration(&platform,logical,chunky,source_palette,
                    menu_selection,selected_vehicle,track_path,0);
            } else {
                slicks_tick_title_registration(logical,registration.name,source_palette);
                redraw_title_configuration(&platform,logical,chunky,source_palette,
                    menu_selection,selected_vehicle,track_path,0);
            }
        }
        if(!save_prompt && g_slicks_track_menu && g_slicks_track_menu->track_lists) {
            if(g_slicks_track_menu->name_dialog) {
                if(slicks_amiga_name_dialog_tick(g_slicks_track_menu,platform.vblank_count)) goto cleanup;
            } else if(g_slicks_track_menu->picker) {
                slicks_status_clock_advance(&picker_clock,platform.vblank_count-picker_clock_vblank);
                picker_clock_vblank=platform.vblank_count;
                if(slicks_amiga_profile_picker_draw(g_slicks_track_menu,picker_clock.ticks)) goto cleanup;
            }
            present_menu_surface(&platform,g_slicks_track_menu);
        }
        if(!save_prompt && g_slicks_player_menu && g_slicks_player_menu->name_dialog) {
            if(slicks_amiga_name_dialog_tick(g_slicks_player_menu,platform.vblank_count)) goto cleanup;
            present_player_menu(&platform);
        }
        if(!save_prompt && g_slicks_player_menu && (g_slicks_player_menu->picker || g_slicks_player_menu->colour_dialog)) {
            unsigned long now=platform.vblank_count;
            slicks_status_clock_advance(&picker_clock,now-picker_clock_vblank);
            picker_clock_vblank=now;
            if(g_slicks_player_menu->colour_dialog) {
                if(slicks_amiga_colour_dialog_draw(g_slicks_player_menu,picker_clock.ticks)) goto cleanup;
            } else if(slicks_amiga_profile_picker_draw(g_slicks_player_menu,picker_clock.ticks)) goto cleanup;
            present_player_menu(&platform);
        }
        if (g_slicks_diag_ingame && g_slicks_diag_scanout_only) {
            slicks_amiga_platform_wait_vblank(&platform);
            if (g_slicks_diag_scanout_only == 2) {
                /* SCANOUTC: unchanged source and scene, repeated production
                 * conversion. Sweep 64-pixel rectangles through all rows
                 * to expose display/write interactions without game logic. */
                unsigned short top =
                    (unsigned short)(g_slicks_diag_scanout_frames % 200);
                unsigned short left = (unsigned short)(
                    ((g_slicks_diag_scanout_frames / 200) % 5) * 64);
                unsigned short bottom = top + 8;
                if (bottom > 200)
                    bottom = 200;
                slicks_amiga_platform_wait_display_blank(&platform);
                slicks_chunky_rect_to_amiga(
                    chunky, platform.views[1].bitmap,
                    left, top, left + 64, bottom, title_frame);
            }
            if (g_slicks_diag_scanout_only == 3) {
                /* SCANOUTF: stress visible DMA with identical full-screen
                 * stores; unlike SCANOUTC, do not wait for display blank. */
                slicks_chunky_rect_to_amiga(
                    chunky, platform.views[1].bitmap,
                    0, 0, 320, 200, title_frame);
            }
            if (++g_slicks_diag_scanout_frames ==
                (g_slicks_diag_scanout_only == 3 ? 50UL : 500UL)) {
                g_slicks_diag_display_checksum =
                    checksum_bitmap(platform.views[1].bitmap);
                slicks_diag_gameplay_ready();
            }
            continue;
        }
        if (g_slicks_diag_ingame) {
            unsigned short dirty;
            unsigned short sound;
            unsigned long audio_blank_at = 0;
            unsigned char was_complete = race->race_complete;
            unsigned char completed_now;
            unsigned char profile =
                (unsigned char)(g_slicks_diag_profile_all || race->frame_count + 1 ==
                                g_slicks_diag_target_frame);
            unsigned long profile_at = platform.vblank_count;
            unsigned long profile_line_at =
                profile ? slicks_diag_profile_raster_time() : 0;
            unsigned long frame_start = profile_line_at;
            unsigned char bench_racing = race->racing;
            if (g_slicks_pc_sampling && bench_racing) {
                pc_sampler_start();
                if (!g_slicks_diag_bench_frames)
                    g_slicks_pc_sample_first_frame=race->frame_count+1;
                /* Match this update's WORK_SAMPLE index. The completed-frame
                 * counter increments only after its work has finished. */
                g_slicks_pc_sample_frame=(unsigned short)g_slicks_diag_bench_frames;
            }
            if (g_slicks_diag_profile_all && bench_racing) {
                if (g_slicks_diag_bench_previous) {
                    g_slicks_diag_bench_cadence_sum +=
                        frame_start - g_slicks_diag_bench_previous;
                    ++g_slicks_diag_bench_cadence_count;
                }
                g_slicks_diag_bench_previous = frame_start;
            }
            if (g_slicks_diag_audit_bitmap)
                snapshot_display(title_asset, platform.views[1].bitmap);
            if (shadow_fixture && race->frame_count == 0) {
                unsigned short car;
                race->racing = 1;
                race->actor_page = 0;
                race->cars[0].special_drive_state = 12000;
                race->cars[0].special_drive_target = 0;
                for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car)
                    race->cars[car].finished = 1;
            } else if (shadow_fixture && race->frame_count == 1) {
                race->cars[0].special_drive_state = 0;
                race->cars[0].special_drive_target = 0;
            }
            if(weapon_hud_fixture) set_weapon_hud_fixture(race);
            if(g_slicks_diag_profile_all) {
                race->profile_frame=g_slicks_diag_profile_all!=2?race->frame_count+1:0;
                race->profile_scope=g_slicks_diag_profile_all>=3?g_slicks_diag_profile_all-2:0;
            }
#ifdef SLICKS_RETENTION_CHECK
            static struct SlicksRetentionSnapshot *saved_race;
            static unsigned char *saved_chunky;
            /* RETCHECK=1: run each racing update first without retention
             * from a snapshot, then for real; the chunky surfaces must match. */
            {
                static struct SlicksRetentionState saved_retention;
                static struct SlicksTrailParticle reference_particles[SLICKS_TRAIL_PARTICLE_MAX];
                if (!saved_race) saved_race = AllocMem(sizeof *saved_race, MEMF_ANY);
                if (!saved_chunky) saved_chunky = AllocMem(64000, MEMF_ANY);
                if (race->racing && saved_race && saved_chunky) {
                    unsigned long reference;
                    slicks_retention_capture(saved_race,race);
                    __builtin_memcpy(saved_chunky, chunky, 64000);
                    saved_retention = slicks_retention;
                    slicks_race_invalidate_retention(race);
                    slicks_race_disable_retention = 1;
                    slicks_race_step(race, logical);
                    reference = retention_check_hash(chunky);
                    unsigned short reference_count=race->trail_particle_count;
                    __builtin_memcpy(reference_particles,race->trail_particles,
                        reference_count*sizeof *reference_particles);
                    if(!slicks_retention_restore(race,saved_race))
                        ++g_slicks_retention_immutable_mismatches;
                    __builtin_memcpy(chunky, saved_chunky, 64000);
                    slicks_retention = saved_retention;
                    slicks_race_disable_retention = 0;
                    slicks_race_step(race, logical);
                    ++g_slicks_retention_checks;
                    if(!slicks_retention_maps_match(saved_race,race))
                        ++g_slicks_retention_immutable_mismatches;
                    unsigned particle_difference=race->trail_particle_count!=reference_count;
                    const volatile unsigned char *expected_points=(const unsigned char *)reference_particles;
                    const volatile unsigned char *actual_points=(const unsigned char *)race->trail_particles;
                    for(unsigned i=0;!particle_difference && i<reference_count*sizeof *reference_particles;++i)
                        particle_difference=expected_points[i]!=actual_points[i];
                    if(particle_difference) ++g_slicks_retention_particle_mismatches;
                    if (retention_check_hash(chunky) != reference &&
                        !g_slicks_retention_mismatches++)
                        g_slicks_retention_first_mismatch = race->frame_count;
                } else slicks_race_step(race, logical);
            }
#else
            slicks_race_step(race, logical);
#endif
            /* Completion is a simulation edge, independent of whether
             * engine playback is enabled or currently owns a channel. */
            completed_now = (unsigned char)(!was_complete && race->race_complete);
            if (race->collision_error) {
                g_slicks_diag_race_error = 10;
                slicks_diag_collision_failed();
                slicks_diag_frame_ready();
                goto cleanup;
            }
            {
                unsigned long now=platform.vblank_count;
                slicks_status_clock_advance(&status_clock,now-status_clock_vblank);
                status_clock_vblank=now;
                int status_result;
#ifdef SLICKS_RETENTION_CHECK
                /* Reuse the snapshot buffers: no extra Chip allocation.
                 * Compare cached bar updates with a forced full repaint,
                 * including damage left by the just-completed race step. */
                if(race->chunky_authoritative && saved_race && saved_chunky) {
                    slicks_retention_capture(saved_race,race);
                    __builtin_memcpy(saved_chunky,chunky,64000);
                    race->status_bar_cache.valid=0;
                    int expected_status=slicks_race_draw_status(race,logical,status_clock.ticks);
                    unsigned long expected_pixels=retention_check_hash(chunky);
                    if(!slicks_retention_restore(race,saved_race))
                        ++g_slicks_retention_immutable_mismatches;
                    __builtin_memcpy(chunky,saved_chunky,64000);
                    status_result=slicks_race_draw_status(race,logical,status_clock.ticks);
                    ++g_slicks_status_cache_checks;
                    if(!slicks_retention_maps_match(saved_race,race))
                        ++g_slicks_retention_immutable_mismatches;
                    if((status_result!=expected_status || retention_check_hash(chunky)!=expected_pixels) &&
                        !g_slicks_status_cache_mismatches++)
                        g_slicks_status_cache_first_mismatch=race->frame_count;
                } else
#endif
                    status_result=slicks_race_draw_status(race,logical,status_clock.ticks);
                if (status_result<0) {
                    g_slicks_diag_race_error=9;
                    goto cleanup;
                }
                audit_weapon_hud(race);
            }
            if (jump_track_test) {
                unsigned short car;
                for (car = 0; car < SLICKS_RACE_CAR_COUNT; ++car) {
                    short state = race->cars[car].special_drive_state;
                    if (state > 0 && previous_jump_state[car] <= 0)
                        ++g_slicks_diag_jump_takeoffs;
                    if (!state && previous_jump_state[car] > 0)
                        ++g_slicks_diag_jump_landings;
                    if (state > (short)g_slicks_diag_jump_peak)
                        g_slicks_diag_jump_peak = (unsigned short)state;
                    if (race->shadows[car].saved_valid)
                        ++g_slicks_diag_jump_shadow_frames;
                    previous_jump_state[car] = state;
                }
            }
            if (shadow_fixture && race->frame_count == 1) {
                const struct SlicksCarShadow *shadow = &race->shadows[0];
                shadow_at = mult320[shadow->old_y] + shadow->old_x + 1;
                shadow_background = shadow->saved_under[1];
                g_slicks_diag_shadow_check = shadow->state == 3 &&
                    shadow->lifetime == 2 && shadow->saved_valid &&
                    chunky[shadow_at] == 37 ? 1 : 10;
            } else if (shadow_fixture && race->frame_count == 5 &&
                       g_slicks_diag_shadow_check == 1) {
                g_slicks_diag_shadow_check = race->shadows[0].state == -3 &&
                    !race->shadows[0].saved_valid &&
                    chunky[shadow_at] == shadow_background ? 2 : 11;
            }
            if (profile) {
                unsigned long now = slicks_diag_profile_raster_time();
                g_slicks_diag_profile_step_vblanks =
                    platform.vblank_count - profile_at;
                g_slicks_diag_profile_step_lines = now - profile_line_at;
                profile_at = platform.vblank_count;
                profile_line_at = now;
            }
            {
                /* Prepare first, publish at one fresh display-end edge.
                 * Do not also pace simulation at VBlank or wait a second
                 * time between audio and C2P. Missed slots are not queued. */
                slicks_amiga_platform_wait_display_end(&platform);
                if (continuous_diagnostics || profile)
                    audio_blank_at = slicks_diag_profile_raster_time();
                if (profile) {
                    profile_at = platform.vblank_count;
                    profile_line_at = audio_blank_at;
                }
            }
            if(race->boundary_palette_pending) {
                for(unsigned colour=0;colour<15;++colour)
                    race_palette[199*3+colour]=race->boundary_colours[colour];
                if(slicks_amiga_platform_update_palette(&platform,1,199,5,race_palette+199*3))
                    goto cleanup;
                race->boundary_palette_pending=0;
            }
            update_race_engines(&audio,race);
            for (sound = 0; sound < race->sound_event_count; ++sound) {
                const struct SlicksSoundEvent *event =
                    &race->sound_events[sound];
                slicks_amiga_audio_play_effect(
                    &audio, event->sample_block, event->flags,
                    event->priority);
            }
            if (completed_now) {
                /* Finish the race batch before replacing it with results.
                 * Original 254fb stops race voices after the race loop;
                 * dispatching this frame's effects afterwards resurrects
                 * race sounds beneath results on otherwise idle channels. */
                slicks_amiga_audio_stop(&audio);
                slicks_amiga_audio_start_music(&audio);
            }
            if (g_slicks_diag_audio_in_blank && (continuous_diagnostics || profile)) {
                unsigned long duration =
                    slicks_diag_profile_raster_time() - audio_blank_at;
                unsigned long remaining = PAL_RASTER_LINES -
                    (audio_blank_at % PAL_RASTER_LINES) + 56;
                if (duration > g_slicks_diag_audio_blank_max_lines)
                    g_slicks_diag_audio_blank_max_lines = duration;
                if (duration >= remaining)
                    ++g_slicks_diag_audio_blank_spills;
            }
            if (profile) {
                unsigned long now = slicks_diag_profile_raster_time();
                g_slicks_diag_profile_audio_vblanks =
                    platform.vblank_count - profile_at;
                g_slicks_diag_profile_audio_lines = now - profile_line_at;
                profile_at = platform.vblank_count;
                profile_line_at = now;
            }
            if (g_slicks_diag_live_stats)
                collect_presentation_snapshot(race,&audio);
            if (profile) {
                profile_at = platform.vblank_count;
                profile_line_at = slicks_diag_profile_raster_time();
            }
            /* Both paths consume the same authoritative chunky surface.
             * Sparse updates include particle restoration/expiry and HUD
             * changes; a later rectangle conversion must preserve them. */
            slicks_race_prune_dirty_pixels(race);
            slicks_chunky_pixels_to_amiga(
                chunky, platform.views[1].bitmap,
                race->dirty_pixels, race->dirty_pixel_count);
            for (dirty = 0; dirty < race->dirty_row_count; ++dirty) {
                const struct SlicksDirtyRows *rows = &race->dirty_rows[dirty];
                slicks_chunky_rect_to_amiga(
                    chunky, platform.views[1].bitmap,
                    rows->left, rows->top, rows->right, rows->bottom,
                    title_frame);
            }
            if (g_slicks_diag_live_stats)
                collect_conversion_statistics(race);
            if (profile) {
                unsigned long now = slicks_diag_profile_raster_time();
                g_slicks_diag_profile_c2p_vblanks =
                    platform.vblank_count - profile_at;
                g_slicks_diag_profile_c2p_lines = now - profile_line_at;
                profile_at = platform.vblank_count;
                profile_line_at = now;
            }
            /* BITMAPFAULT is the audit's positive control: flip a static
             * pixel outside the countdown's declared updates. */
            if (g_slicks_diag_audit_bitmap == 2 && race->frame_count == 1)
                platform.views[1].bitmap->Planes[0][100UL * 320UL] ^= 0x80;
            /* Positive control for a missing update, not an extra write. */
            if (g_slicks_diag_audit_bitmap == 3 && race->frame_count == 1)
                race->chunky[mult320[100]] ^= 1;
            if (g_slicks_diag_audit_bitmap &&
                audit_display(title_asset, platform.views[1].bitmap, race) != 0)
                goto cleanup;
            if (shadow_fixture && (race->frame_count == 1 || race->frame_count == 5) &&
                g_slicks_diag_shadow_check < 10) {
                const struct BitMap *bitmap = platform.views[1].bitmap;
                unsigned long offset = (shadow_at / 320UL) * bitmap->BytesPerRow +
                                       (shadow_at % 320UL) / 8;
                unsigned char mask = (unsigned char)(0x80U >> (shadow_at & 7));
                unsigned short plane;
                unsigned short colour = 0;
                for (plane = 0; plane < 8; ++plane)
                    if (bitmap->Planes[plane][offset] & mask)
                        colour |= 1U << plane;
                if (colour != (race->frame_count == 1 ? 37 : shadow_background))
                    g_slicks_diag_shadow_check = 12;
            }
            slicks_race_clear_dirty_rows(race);
            unsigned char damage_visible = 0;
            if (service_menu_test)
                for (unsigned short car = 0; car < 4; ++car)
                    if (race->cars[car].damage[0] >= 40)
                        damage_visible = 1;
            if ((fuel_race_test || service_menu_test) && (completed_now ||
                (damage_visible && !g_slicks_diag_damage_status_checks) ||
                !(status_checked_phases & (1U << (status_clock.ticks & 1))))) {
                const struct BitMap *bitmap=platform.views[1].bitmap;
                unsigned short car, x, y, plane;
                for (car=0;car<4;++car) {
                    struct SlicksStatusRect rects[3];
                    int count=slicks_race_status_rects(race,car,status_clock.ticks,rects);
                    if (count<0) { ++g_slicks_diag_status_failures; continue; }
                    for (y=187;y<190;++y)
                    for (x=106+car*60;x<126+car*60;++x) {
                        unsigned char expected=race->status_colours[0], actual=0;
                        unsigned long offset=(unsigned long)y*bitmap->BytesPerRow+x/8;
                        for (int i=0;i<count;++i)
                            if (x>=rects[i].left && x<rects[i].right &&
                                y>=rects[i].top && y<rects[i].bottom)
                                expected=race->status_colours[rects[i].colour];
                        for (plane=0;plane<8;++plane)
                            if (bitmap->Planes[plane][offset] & (0x80U >> (x&7))) actual|=1U<<plane;
                        if (actual!=expected || chunky[mult320[y]+x]!=expected)
                            ++g_slicks_diag_status_failures;
                    }
                }
                status_checked_phases|=1U << (status_clock.ticks & 1);
                ++g_slicks_diag_status_checks;
                if (damage_visible)
                    ++g_slicks_diag_damage_status_checks;
            }
            if(continuous_diagnostics || completed_now ||
               race->frame_count==g_slicks_diag_target_frame)
                update_race_diagnostics(race);
            if (profile) {
                unsigned long now = slicks_diag_profile_raster_time();
                g_slicks_diag_profile_diag_vblanks =
                    platform.vblank_count - profile_at;
                g_slicks_diag_profile_diag_lines = now - profile_line_at;
                /* Exclude benchmark aggregation and between-update polling
                 * from the current update's CPU profile. */
                if (g_slicks_pc_sampling) g_slicks_pc_sample_frame=0xffff;
                g_slicks_diag_profile_total_vblanks =
                    g_slicks_diag_profile_step_vblanks +
                    g_slicks_diag_profile_audio_vblanks +
                    g_slicks_diag_profile_c2p_vblanks +
                    g_slicks_diag_profile_diag_vblanks;
                g_slicks_diag_profile_total_lines =
                    g_slicks_diag_profile_step_lines +
                    g_slicks_diag_profile_audio_lines +
                    g_slicks_diag_profile_c2p_lines +
                    g_slicks_diag_profile_diag_lines;
                if (g_slicks_diag_profile_all && bench_racing) {
                    unsigned long wall = now - frame_start;
                    unsigned long work = g_slicks_diag_profile_total_lines;
                    if(g_slicks_diag_bench_frames < 704) {
                        g_slicks_diag_bench_work_samples[g_slicks_diag_bench_frames]=work;
                        g_slicks_diag_bench_particle_samples[g_slicks_diag_bench_frames]=race->trail_particle_count;
                    }
                    ++g_slicks_diag_bench_frames;
                    /* Aggregate the same measured intervals as the worst-frame
                     * snapshot. Bookkeeping is outside the work timer and runs
                     * only in the diagnostic benchmark, never normal play. */
                    g_slicks_diag_bench_work_sum += work;
                    for(unsigned i=0;i<4;++i) {
                        g_slicks_diag_bench_actor_sum[i]+=g_slicks_diag_profile_actor_lines[1+i];
                        g_slicks_diag_bench_actor_sum[4+i]+=g_slicks_diag_profile_actor_lines[11+i];
                    }
                    for(unsigned i=0;i<2;++i)
                        g_slicks_diag_bench_car_sum[i]+=g_slicks_diag_profile_car_draw[i];
                    for(unsigned i=0;i<3;++i)
                        g_slicks_diag_bench_motion_sum[i]+=g_slicks_diag_profile_motion[i];
                    g_slicks_diag_bench_stage_sum[0]+=g_slicks_diag_profile_restore_lines;
                    g_slicks_diag_bench_stage_sum[1]+=g_slicks_diag_profile_advance_lines;
                    g_slicks_diag_bench_stage_sum[2]+=g_slicks_diag_profile_update_lines;
                    g_slicks_diag_bench_stage_sum[3]+=g_slicks_diag_profile_hud_lines;
                    g_slicks_diag_bench_stage_sum[4]+=g_slicks_diag_profile_draw_lines;
                    g_slicks_diag_bench_stage_sum[5]+=g_slicks_diag_profile_audio_lines;
                    g_slicks_diag_bench_stage_sum[6]+=g_slicks_diag_profile_c2p_lines;
                    g_slicks_diag_bench_stage_sum[7]+=g_slicks_diag_profile_diag_lines;
                    for(unsigned i=0;i<4;++i)
                        g_slicks_diag_bench_tail_sum[i]+=g_slicks_diag_profile_tail[i];
                    for(unsigned i=0;i<3;++i)
                        g_slicks_diag_bench_simulation_sum[i]+=g_slicks_diag_profile_actor_lines[21+i];
                    if (work > g_slicks_diag_bench_work_max) {
                        g_slicks_diag_bench_work_max = work;
                        g_slicks_diag_bench_work_max_frame = race->frame_count;
                        g_slicks_diag_bench_max_stages[0]=g_slicks_diag_profile_restore_lines;
                        g_slicks_diag_bench_max_stages[1]=g_slicks_diag_profile_advance_lines;
                        g_slicks_diag_bench_max_stages[2]=g_slicks_diag_profile_update_lines;
                        g_slicks_diag_bench_max_stages[3]=g_slicks_diag_profile_hud_lines;
                        g_slicks_diag_bench_max_stages[4]=g_slicks_diag_profile_draw_lines;
                        g_slicks_diag_bench_max_stages[5]=g_slicks_diag_profile_audio_lines;
                        g_slicks_diag_bench_max_stages[6]=g_slicks_diag_profile_c2p_lines;
                        g_slicks_diag_bench_max_stages[7]=g_slicks_diag_profile_diag_lines;
                        g_slicks_diag_bench_max_particles=race->trail_particle_count;
                        for(unsigned i=0;i<3;++i)
                            g_slicks_diag_bench_max_simulation[i]=g_slicks_diag_profile_actor_lines[21+i];
                        for(unsigned i=0;i<4;++i)
                            g_slicks_diag_bench_max_tail[i]=g_slicks_diag_profile_tail[i];
                        for(unsigned i=0;i<4;++i) {
                            g_slicks_diag_bench_max_actors[i]=g_slicks_diag_profile_actor_lines[1+i];
                            g_slicks_diag_bench_max_actors[4+i]=g_slicks_diag_profile_actor_lines[11+i];
                        }
                        for(unsigned i=0;i<2;++i)
                            g_slicks_diag_bench_max_car_draw[i]=g_slicks_diag_profile_car_draw[i];
                        g_slicks_diag_bench_max_sparse=g_slicks_diag_sparse_converted;
                        for(unsigned i=0;i<3;++i)
                            g_slicks_diag_bench_max_sprite[i]=g_slicks_diag_profile_sprite[i];
                        g_slicks_diag_bench_max_rect_pixels=g_slicks_diag_profile_rect_pixels;
                    }
                    if (work > PAL_RASTER_LINES)
                        ++g_slicks_diag_bench_work_over;
                    if (wall > g_slicks_diag_bench_wall_max)
                        g_slicks_diag_bench_wall_max = wall;
                }
            }
            if (completed_now) {
                sync_chunky_to_logical(chunky, logical);
                g_slicks_diag_checksum = checksum_planes(logical);
                g_slicks_diag_display_checksum =
                    checksum_bitmap(platform.views[1].bitmap);
                slicks_diag_results_ready();
            }
            if (race->frame_count == g_slicks_diag_target_frame) {
                pc_sampler_stop();
                sync_chunky_to_logical(chunky, logical);
                g_slicks_diag_checksum = checksum_planes(logical);
                g_slicks_diag_display_checksum =
                    checksum_bitmap(platform.views[1].bitmap);
                slicks_diag_gameplay_ready();
            }
            if(g_slicks_diag_audio_hold_frames) {
                slicks_amiga_platform_wait_display_blank(&platform);
                slicks_amiga_audio_play_effect(&audio,13,0,127);
                while(g_slicks_diag_audio_hold_frames) {
                    slicks_amiga_platform_wait_vblank(&platform);
                    --g_slicks_diag_audio_hold_frames;
                }
            }
            slicks_diag_frame_ready();
        }
    }

cleanup:
    if(!result && registration_presentation && !g_slicks_diag_force_exit) {
        slicks_amiga_audio_stop(&audio);
        unsigned short timer=slicks_speed_timer_argument(configuration.field_05de);
        if(registration_screen(&platform,chunky,1,timer) ||
           (!registration.name[0] && registration_screen(&platform,chunky,2,timer))) result=20;
    }
    pc_sampler_release();
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
    slicks_amiga_player_menu_destroy(g_slicks_player_menu); g_slicks_player_menu=0;
    slicks_amiga_player_menu_destroy(g_slicks_options_menu); g_slicks_options_menu=0;
    slicks_amiga_player_menu_destroy(g_slicks_track_menu); g_slicks_track_menu=0;
    g_slicks_track_renderer.surface=0;
    slicks_amiga_player_menu_destroy(g_slicks_title_help); g_slicks_title_help=0;
    g_slicks_options_renderer.surface=0;
    g_slicks_options_configuration=0;
    slicks_amiga_audio_destroy(&audio);
    slicks_resource_cache_destroy(menu_cache); menu_cache=0;
    slicks_amiga_track_list_cache_free(&track_list_cache);
    if (sample_resource)
        FreeMem(sample_resource, 131691UL);
    if (chunky)
        FreeMem(chunky, CHUNKY_ALLOCATION_BYTES);
    if (race)
        FreeMem(race, sizeof(*race));
    if (logical)
        FreeMem(logical, 0x40000UL);
    g_slicks_diag_logical = 0;
    slicks_title_background=0;
    if (title_frame)
        FreeMem(title_frame, TITLE_FRAME_ALLOCATION_BYTES);
    if (title_asset)
        FreeMem(title_asset, 64003UL);
    if(slicks_title_font) { FreeMem(slicks_title_font,title_font_sizes[0]); slicks_title_font=0; }
    if(slicks_title_small_font) { FreeMem(slicks_title_small_font,title_font_sizes[1]); slicks_title_small_font=0; }
    if(title_arcade_font) {FreeMem(title_arcade_font,title_font_sizes[2]);title_arcade_font=0;}
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    GfxBase = 0;
    DOSBase = 0;
    return result;
}
