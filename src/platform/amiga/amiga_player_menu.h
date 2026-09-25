#ifndef SLICKS_AMIGA_PLAYER_MENU_H
#define SLICKS_AMIGA_PLAYER_MENU_H
#include "resource_archive.h"
#include "../../ui/player_menu_renderer.h"
#include "../../ui/list_renderer.h"
#include "../../ui/profile_editor_renderer.h"
#include "../../ui/name_dialog.h"
#include "../../ui/colour_dialog.h"
#include "../../ui/options_menu_renderer.h"
#include "../../ui/track_menu_renderer.h"
#include "../../ui/controllers_dialog_renderer.h"
#include "../../ui/help_renderer.h"
#include "../../ui/help_viewer.h"
#include "../../ui/message_dialog.h"
#include "../../ui/track_list_dialog.h"
#include "../../ui/race_menu_renderer.h"
#include "../../ui/speed_dialog.h"
#include "../../ui/change_cars_renderer.h"
#include "../../ui/intermission_renderer.h"
#include "amiga_setup_storage.h"
#include "../../ui/championship_standings_draw.h"
struct SlicksAmigaPlayerMenu;
void slicks_amiga_standings_draw(struct SlicksAmigaPlayerMenu *,
    const struct SlicksChampionshipStandings *,const unsigned char [4][6],
    const unsigned char *const [4]);
struct SlicksAmigaTrackLists {
    unsigned char *bytes;
    struct SlicksTrackLists catalogue;
    unsigned char name[21];
    short pending_delete;
};
/* One-shot diagnostic allocation boundaries; zero in normal execution. */
extern unsigned char g_slicks_diag_list_alloc_fault;
struct SlicksAmigaTrackInfo {
    unsigned char saved[64000],preview[64*40],palette[768],font_colours[2];
    unsigned long updates;
    unsigned short phase;
};
struct SlicksAmigaMessageDialog {
    struct SlicksMessageDialog renderer;
    unsigned char saved[8192];
};

struct SlicksAmigaControllersDialog {
    struct SlicksControllersRenderer renderer;
    struct SlicksControllersDialog state;
    struct SlicksMenuIcon icons[8];
    unsigned char pixels[8][256],original[16800],tinted[16000];
};

struct SlicksAmigaColourDialog {
    struct SlicksColourDialog renderer;
    unsigned char saved[2400];
};

struct SlicksAmigaNameDialog {
    struct SlicksNameDialog renderer;
    unsigned char original[8192],field[8192],cursor[256];
    unsigned long blink_bucket;
    unsigned char hidden;
};

struct SlicksAmigaProfilePicker {
    struct SlicksListRenderer renderer;
    unsigned char original[15200],tinted[14896],caption[4096];
    unsigned char *owned_names;
    unsigned long owned_names_size;
    struct SlicksSavedRectangle scrollbar_saved;
    unsigned char scrollbar_background[800];
};

struct SlicksMenuRows { unsigned short top,bottom; };
struct SlicksAmigaIntermission {
    struct SlicksIntermissionRenderer renderer;
    struct SlicksRecordsRenderer surface;
    struct SlicksIntermissionMenu state;
    struct SlicksIntermissionContent content;
    unsigned char buttons[3024],cars[320],names[4][21],labels[4][128],track_name[128],slash[8];
    unsigned char old_colour;
};
struct SlicksAmigaChangeCars {
    struct SlicksChangeCarsRenderer renderer;
    struct SlicksChangeCarsDialog state;
    struct SlicksPlayerMenuRenderer surface;
    struct SlicksProfileSelection *players;
    struct SlicksMenuIcon icons[11];
    unsigned char pixels[10][192],original[4000],decorated[4000];
    short vehicle_count;
};
struct SlicksAmigaRaceMenu {
    struct SlicksRaceMenu state;
    struct SlicksRaceMenuRenderer renderer;
    struct SlicksRecordsRenderer surface;
    unsigned char tinted[7500],language[1000];
    const unsigned char *labels[6];
    struct SlicksSpeedDialog speed;
    unsigned char speed_active;
};
struct SlicksAmigaPlayerMenu {
    struct SlicksPlayerMenuRenderer renderer;
    struct SlicksPlayerMenuLabels labels;
    unsigned char palette[768],saved[64000],fonts[3][6000],pixels[11][192];
    struct SlicksMenuIcon icons[11];
    struct SlicksMenuRows dirty[16];
    unsigned short dirty_count;
    int error;
    struct SlicksAmigaProfilePicker *picker;
    short delete_index;
    unsigned char delete_pending,delete_old_colour;
    struct SlicksProfileEditor editor;
    short editor_index;
    unsigned char editor_active,editor_new,editor_old_colour,editor_pending;
    unsigned char editor_name[21];
    struct SlicksAmigaNameDialog *name_dialog;
    struct SlicksAmigaColourDialog *colour_dialog;
    struct SlicksAmigaControllersDialog *controllers_dialog;
    struct SlicksHelpViewer *help;
    unsigned char help_warning;
    struct SlicksAmigaMessageDialog *message;
    struct SlicksAmigaTrackLists *track_lists;
    struct SlicksAmigaTrackInfo *track_info;
    struct SlicksAmigaRaceMenu *race_menu;
    struct SlicksAmigaChangeCars *change_cars;
    struct SlicksAmigaIntermission *intermission;
    unsigned char key_characters[8][128],key_modifiers;
};
/* OS available for open/close. Drawing and key dispatch need no OS calls.
 * Use a dedicated help-surface object: its saved[] belongs to the race page. */
int slicks_amiga_race_menu_open(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *,
    const char *,const unsigned char [6][64],unsigned char,unsigned char);
int slicks_amiga_race_menu_draw(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_race_menu_close(struct SlicksAmigaPlayerMenu *);
struct SlicksAmigaPlayerMenu *slicks_amiga_race_surface_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *);
/* Retains the caller's page; loads fonts, ten cars and the original clock
 * marker. records_icon ID -1 maps to that marker on this dedicated owner. */
struct SlicksAmigaPlayerMenu *slicks_amiga_intermission_surface_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *);
/* Dedicated intermission surface only. Inputs/strings copied on open; track
 * bytes and decode arena are needed only during open, with OS available.
 * Labels are already resolved through the original language lookup. */
int slicks_amiga_intermission_open(struct SlicksAmigaPlayerMenu *,const struct SlicksIntermissionContent *,
    const unsigned char *,const unsigned char *,unsigned long,const unsigned char *,unsigned long);
int slicks_amiga_intermission_key(struct SlicksAmigaPlayerMenu *,unsigned char);
int slicks_amiga_intermission_refresh_cars(struct SlicksAmigaPlayerMenu *,const signed char [4]);
int slicks_amiga_intermission_close(struct SlicksAmigaPlayerMenu *);
extern unsigned char g_slicks_diag_intermission_fault;
int slicks_amiga_race_speed_open(struct SlicksAmigaPlayerMenu *,struct SlicksConfiguration *,unsigned char *);
int slicks_amiga_race_speed_key(struct SlicksAmigaPlayerMenu *,struct SlicksConfiguration *,unsigned char);
int slicks_amiga_race_speed_close(struct SlicksAmigaPlayerMenu *,const struct SlicksConfiguration *,unsigned short *);
extern unsigned char g_slicks_diag_pause_fault;
/* Original prepare/RNG runs before allocation. Returns 1 for an open modal,
 * 0 if original show flag remains zero, -1 on failure. Open/close require OS;
 * key handles DOS scans under takeover and returns 1 when ready to close.
 * Caller keeps the validated profile selection alive throughout the modal. */
int slicks_amiga_change_cars_open(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *,
    struct SlicksProfileSelection *,const struct SlicksSetupProfile *,short,
    const unsigned char *,unsigned long *,signed char,const unsigned char *);
int slicks_amiga_change_cars_key(struct SlicksAmigaPlayerMenu *,unsigned char);
int slicks_amiga_change_cars_close(struct SlicksAmigaPlayerMenu *);
extern unsigned char g_slicks_diag_change_cars_fault;
/* Call only with the OS available, before hardware takeover. Strings are
 * original initialized-DS resources, kept alive for the menu lifetime.
 * Chunky storage (with C2P lookahead) belongs to the caller. */
struct SlicksAmigaPlayerMenu *slicks_amiga_player_menu_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *,
    const unsigned char *,unsigned char,const struct SlicksPlayerMenuLabels *);
/* Reserved save-under storage permits a warning even when Help allocation fails. */
int slicks_amiga_warning_open(struct SlicksAmigaPlayerMenu *,const unsigned char *message);
int slicks_amiga_emergency_warning_open(unsigned char *,const unsigned char *,unsigned char *,const unsigned char *);
int slicks_amiga_emergency_warning_close(void);
extern unsigned char g_slicks_diag_controllers_fault;
int slicks_amiga_help_warning_open(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_help_warning_close(struct SlicksAmigaPlayerMenu *);
extern unsigned char g_slicks_diag_help_fail_allocation;
extern unsigned char g_slicks_diag_track_info_fault;
/* Shared menu surface and ownership, without players.bmp or vehicle icons. */
struct SlicksAmigaPlayerMenu *slicks_amiga_options_menu_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *,
    const unsigned char *,struct SlicksOptionsRenderer *);
struct SlicksAmigaPlayerMenu *slicks_amiga_track_menu_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *,
    const unsigned char *,short,unsigned char,struct SlicksTrackRenderer *,
    const unsigned char *(*)(void *,unsigned),void *);
struct SlicksAmigaPlayerMenu *slicks_amiga_help_surface_create(
    struct SlicksResourceArchive *,unsigned char *,const unsigned char *);
void slicks_amiga_player_menu_destroy(struct SlicksAmigaPlayerMenu *);
struct SlicksSetupLoadReport slicks_amiga_track_lists_open(
    struct SlicksAmigaPlayerMenu *,const unsigned char *,unsigned char);
int slicks_amiga_track_lists_picker(struct SlicksAmigaPlayerMenu *,const unsigned char *,unsigned char);
struct SlicksTrackListChoice slicks_amiga_track_lists_choice(struct SlicksAmigaPlayerMenu *,short);
void slicks_amiga_track_lists_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_player_menu_draw(struct SlicksAmigaPlayerMenu *,unsigned,
    const short [4],const signed char [4],const struct SlicksPlayerProfiles *);
void slicks_amiga_player_menu_clear_dirty(struct SlicksAmigaPlayerMenu *);
/* Open/close allocate/free: call only while AmigaOS is available. */
/* Original .SSS picker geometry, copied nine-byte records, maximum 40.
 * OS available; existing picker draw/key/close APIs apply. */
int slicks_amiga_saved_files_picker(struct SlicksAmigaPlayerMenu *,const unsigned char [][9],
    unsigned,unsigned char,const unsigned char *,unsigned char);
int slicks_amiga_saved_filename_open(struct SlicksAmigaPlayerMenu *,unsigned char [9],
    const unsigned char *,unsigned char);
int slicks_amiga_profile_picker_open(struct SlicksAmigaPlayerMenu *,unsigned,short,
    const struct SlicksPlayerProfiles *,const unsigned char *,unsigned char);
short slicks_amiga_profile_picker_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_profile_picker_draw(struct SlicksAmigaPlayerMenu *,unsigned long);
int slicks_amiga_profile_delete_prompt(struct SlicksAmigaPlayerMenu *,short,
    const struct SlicksPlayerProfiles *,const unsigned char *,unsigned char);
void slicks_amiga_player_menu_restore(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_profile_editor_open(struct SlicksAmigaPlayerMenu *,struct SlicksPlayerProfiles *,
    short,unsigned char,unsigned long *);
int slicks_amiga_profile_editor_draw(struct SlicksAmigaPlayerMenu *,struct SlicksPlayerProfiles *,
    const struct SlicksProfileEditorLabels *,unsigned char);
int slicks_amiga_profile_editor_close(struct SlicksAmigaPlayerMenu *,struct SlicksPlayerProfiles *);
int slicks_amiga_name_dialog_open(struct SlicksAmigaPlayerMenu *,const unsigned char *,unsigned char);
struct SlicksTrackRecords;
int slicks_amiga_track_info_open(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *,
    const unsigned char *,unsigned long,const unsigned char *,unsigned long,
    const unsigned char *,unsigned char,unsigned char,signed char);
void slicks_amiga_track_info_close(struct SlicksAmigaPlayerMenu *);
void slicks_amiga_track_info_tick(struct SlicksAmigaPlayerMenu *,unsigned long *);
int slicks_amiga_records_icons_load(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *);
int slicks_amiga_records_draw(struct SlicksAmigaPlayerMenu *,const struct SlicksTrackRecords *,
    const signed char [4],short,short,unsigned char,signed char);
int slicks_amiga_message_open_font(struct SlicksAmigaPlayerMenu *,const unsigned char *,unsigned char,unsigned);
int slicks_amiga_name_dialog_open_at(struct SlicksAmigaPlayerMenu *,unsigned char [21],
    const unsigned char *,short,short,unsigned char);
int slicks_amiga_name_dialog_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_name_dialog_tick(struct SlicksAmigaPlayerMenu *,unsigned long);
unsigned char slicks_amiga_menu_character(struct SlicksAmigaPlayerMenu *,unsigned char);
int slicks_amiga_colour_dialog_open(struct SlicksAmigaPlayerMenu *,struct SlicksPlayerProfiles *,const unsigned char *);
int slicks_amiga_colour_dialog_draw(struct SlicksAmigaPlayerMenu *,unsigned long);
int slicks_amiga_colour_dialog_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_controllers_open(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *);
int slicks_amiga_controllers_open_at(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *,short,short);
int slicks_amiga_controllers_draw(struct SlicksAmigaPlayerMenu *,const struct SlicksConfiguration *,const struct SlicksControllersLabels *);
int slicks_amiga_controllers_capture_prompt(struct SlicksAmigaPlayerMenu *,const unsigned char *);
int slicks_amiga_controllers_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_help_renderer_init(struct SlicksAmigaPlayerMenu *,struct SlicksHelpRenderer *);
int slicks_amiga_help_open(struct SlicksAmigaPlayerMenu *,struct SlicksResourceArchive *,const unsigned char *);
int slicks_amiga_help_close(struct SlicksAmigaPlayerMenu *);
int slicks_amiga_message_open(struct SlicksAmigaPlayerMenu *,const unsigned char *,unsigned char);
int slicks_amiga_message_close(struct SlicksAmigaPlayerMenu *);
#endif
