#ifndef SLICKS_MENU_RESOURCES_H
#define SLICKS_MENU_RESOURCES_H

/* Startup-resident encoded resources. The decoded mainmenu and partII palette
 * already have permanent owners. Large resources come first to avoid many
 * small allocations splitting the space needed by modal menu workspaces. */
static const char *const slicks_menu_resources[]={
    "tuning.@I", "trckmenu.@I", "players.bmp", "HELP.TXT",
    "kirj.@f", "iso.@f", "pieni.@f",
    "trckmenu.@p", "peli.@p", "tuning.@p",
    "val1.@I", "val2.@I", "pel_on.@I", "pel_ei.@I", "pel_t.@I",
    "computer.@16", "carimage16", "auto01.@16", "auto02.@16",
    "auto03.@16", "auto04.@16", "auto05.@16", "auto06.@16",
    "auto07.@16", "auto08.@16", "auto09.@16", "top10cc.@16",
    "lang1.txt", "lang2.txt", "lang3.txt", "lang4.txt",
    "lang5.txt", "lang6.txt", "lang7.txt", "lang8.txt",
    "ohj_key.@I", "ohj_joy.@I", "ohj_lptc.@I",
    "keys_m1.@16", "keys_m2.@16", "keys_m3.@16", "keys_m4.@16", "keys_m5.@16",
    "vir00.@16", "vir01.@16", "vir02.@16", "vir03.@16", "vir04.@16",
    "vir05.@16", "vir06.@16", "vir07.@16", "vir08.@16", "vir09.@16",
    "vir10.@16", "vir11.@16", "vir12.@16", "clock.@16"
};
#define SLICKS_MENU_RESOURCE_COUNT (sizeof slicks_menu_resources/sizeof *slicks_menu_resources)
#endif
